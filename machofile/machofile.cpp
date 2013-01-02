//
//  machofile.cpp
//  
//
//  Created by Glenn Lugod on 12/29/12.
//
//

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <err.h>
#include <string.h>

#include <libkern/OSAtomic.h>

#include "machofile.h"

namespace rotg {
    
    MachOFile::MachOFile()
        : m_fd(-1)
        , m_data(NULL)
        , m_baseAddress(NULL)
        , m_header(NULL)
        , m_header64(NULL)
        , m_header_size(0)
        , m_fat_header(NULL)
        , m_is64(false)
        , m_is_universal(false)
        , m_archInfo(NULL)
        , m_is_need_byteswap(false)
    {
    }
    
    MachOFile::~MachOFile()
    {
        segment_64_infos_t::iterator iter;
        for (iter = m_segment_64_infos.begin(); iter != m_segment_64_infos.end(); iter++) {
            delete *iter;
        }
        
        if ((m_data) && (m_data != MAP_FAILED)) {
            munmap(m_data, m_stbuf.st_size);
            m_data = NULL;
        }
        
        if (m_fd > 0) {
            close(m_fd);
            m_fd = -1;
        }
    }
    
    /* Verify that the given range is within bounds. */
    const void* MachOFile::macho_read(macho_input_t* input, const void *address, size_t length) {
        if ((((uint8_t *) address) - ((uint8_t *) input->data)) + length > input->length) {
            warnx("Short read parsing Mach-O input");
            return NULL;
        }
        
        return address;
    }
    
    /* Verify that address + offset + length is within bounds. */
    const void* MachOFile::macho_offset(macho_input_t *input, const void *address, size_t offset, size_t length) {
        void *result = ((uint8_t *) address) + offset;
        return macho_read(input, result, length);
    }
    
    /* return a human readable formatted version number. the result must be free()'d. */
    char* MachOFile::macho_format_dylib_version(uint32_t version) {
        char *result = NULL;
        asprintf(&result, "%u.%u.%u", (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
        return result;
    }
    
    bool MachOFile::parse_universal(macho_input_t *input)
    {
        uint32_t nfat = OSSwapBigToHostInt32(m_fat_header->nfat_arch);
        const struct fat_arch* archs = (const struct fat_arch*)macho_offset(input, m_fat_header, sizeof(struct fat_header), sizeof(struct fat_arch));
        if (archs == NULL)
            return false;
        
        //printf("Architecture Count: %d\n", nfat);
        for (uint32_t i = 0; i < nfat; i++) {
            const struct fat_arch* arch = (const struct fat_arch*)macho_read(input, archs + i, sizeof(struct fat_arch));
            if (arch == NULL)
                return false;
            
            /* Fetch a pointer to the architecture's Mach-O header. */
            size_t length = OSSwapBigToHostInt32(arch->size);
            const void *data = macho_offset(input, input->data, OSSwapBigToHostInt32(arch->offset), length);
            if (data == NULL)
                return false;
            
            fat_arch_info_t fat_arch_info;
            fat_arch_info.arch = arch;
            fat_arch_info.input.length = length;
            fat_arch_info.input.data = data;
            
            m_fat_arch_infos.push_back(fat_arch_info);
        }
        
        return true;
    }
    
    bool MachOFile::parse_LC_SEGMENT_64(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct segment_command_64)) {
            warnx("Incorrect cmd size");
            return false;
        }
        
        const struct segment_command_64* segment_cmd_64 = (const struct segment_command_64*)load_cmd_info->cmd;
        
        // preserve segment RVA/size for offset lookup
        m_segmentInfo[segment_cmd_64->fileoff] = std::make_pair(segment_cmd_64->vmaddr, segment_cmd_64->vmsize);
        
        // Section Headers
        for (uint32_t nsect = 0; nsect < segment_cmd_64->nsects; ++nsect)
        {
        /*
             uint32_t sectionloc = location + sizeof(struct segment_command_64) + nsect * sizeof(struct section_64);
             MATCH_STRUCT(section_64,sectionloc)
             [self createSection64Node:node
             caption:[NSString stringWithFormat:@"Section64 Header (%s)",
             string(section_64->sectname,16).c_str()]
             location:sectionloc
             section_64:section_64];
             
             // preserv section fileOffset/sectName for RVA lookup
             NSDictionary * userInfo = [self userInfoForSection64:section_64];
             insertChild[section_64->addr] = make_pair(section_64->offset + imageOffset, userInfo);
             
             // preserv header info for latter use
             sections_64.push_back(section_64);
         */
        }

        segment_64_info_t* info = new segment_64_info_t();
        if (info == NULL) {
            return false;
        }
        
        info->cmd_type = cmd_type;
        info->cmd = segment_cmd_64;
        
        load_cmd_info->cmd_info = info;
        m_segment_64_infos.push_back(info);
        
        return true;
    }
    
    bool MachOFile::parse_LC_RPATH(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct rpath_command)) {
            warnx("Incorrect cmd size");
            return false;
        }
        
        /* Fetch the path */
        size_t pathlen = cmdsize - sizeof(struct rpath_command);
        const char* pathptr = (const char*)macho_offset(input, load_cmd_info->cmd, sizeof(struct rpath_command), pathlen);
        if (pathptr == NULL)
            return false;
        
        struct runpath_additions_info info;
        info.cmd_type = cmd_type;
        info.cmd = (struct rpath_command*)load_cmd_info->cmd;
        info.path = pathptr;
        info.pathlen = pathlen;
        
        m_runpath_additions_infos.push_back(info);

        return true;
    }
    
    bool MachOFile::parse_LC_DYLIBS(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct dylib_command)) {
            warnx("Incorrect name size");
            return false;
        }
        
        const struct dylib_command *dylib_cmd = (const struct dylib_command *)load_cmd_info->cmd;
        
        /* Extract the install name */
        size_t namelen = cmdsize - sizeof(struct dylib_command);
        const char* nameptr = (const char*)macho_offset(input, load_cmd_info->cmd, sizeof(struct dylib_command), namelen);
        if (nameptr == NULL)
            return false;
        
        /* Print the dylib info */
        char *current_version = macho_format_dylib_version(read32(dylib_cmd->dylib.current_version));
        char *compat_version = macho_format_dylib_version(read32(dylib_cmd->dylib.compatibility_version));
        
        struct dylib_info dl_info;
        dl_info.cmd_type = cmd_type;
        dl_info.cmd = dylib_cmd;
        dl_info.name = nameptr;
        dl_info.namelen = namelen;
        dl_info.current_version = current_version;
        dl_info.compat_version = compat_version;
        
        m_dylib_infos.push_back(dl_info);
        
        free(current_version);
        free(compat_version);
        
        return true;
    }
    
    bool MachOFile::parse_binding_node(macho_input_t *input, const struct dyld_info_command* dyld_info_cmd)
    {
        const uint8_t* baseAddress = (const uint8_t*)macho_offset(input, input->data, dyld_info_cmd->bind_off, dyld_info_cmd->bind_size);
        if (baseAddress == NULL) {
            return false;
        }
        
        const uint8_t* ptr = baseAddress;
        const uint8_t* endAddress = baseAddress + dyld_info_cmd->bind_size;
        
        //const uint8_t* address = baseAddress;
        bool isDone = false;
        
        while ((ptr < endAddress) && !isDone) {
            uint8_t opcode = *ptr & REBASE_OPCODE_MASK;
            //uint8_t immediate = *ptr & REBASE_IMMEDIATE_MASK;
            
            switch (opcode) {
                case REBASE_OPCODE_DONE:
                    isDone = true;
                    break;
                    
                case REBASE_OPCODE_SET_TYPE_IMM:
                    /*
                    type = immediate;
                    
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_SET_TYPE_IMM"
                                           :[NSString stringWithFormat:@"type (%i)", type]];
                    */
                    break;
                    
                case REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
                {
                    /*
                    uint32_t segmentIndex = immediate;
                    
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB"
                                           :[NSString stringWithFormat:@"segment (%u)", segmentIndex]];
                    
                    uint64_t offset = [self read_uleb128:range lastReadHex:&lastReadHex];
                    
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"uleb128"
                                           :[NSString stringWithFormat:@"offset (%qi)",offset]];
                    
                    if (([self is64bit] == NO && segmentIndex >= segments.size()) ||
                        ([self is64bit] == YES && segmentIndex >= segments_64.size()))
                    {
                        [NSException raise:@"Segment"
                                    format:@"index is out of range %u", segmentIndex];
                    }
                    
                    address = ([self is64bit] == NO ? segments.at(segmentIndex)->vmaddr
                               : segments_64.at(segmentIndex)->vmaddr) + offset;
                    */
                } break;
                    
                case REBASE_OPCODE_ADD_ADDR_ULEB:
                {
                    /*
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_ADD_ADDR_ULEB"
                                           :@""];
                    
                    uint64_t offset = [self read_uleb128:range lastReadHex:&lastReadHex];
                    
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"uleb128"
                                           :[NSString stringWithFormat:@"offset (%qi)",offset]];
                    
                    address += offset;
                    */
                } break;
                    
                case REBASE_OPCODE_ADD_ADDR_IMM_SCALED:
                {
                    /*
                    uint32_t scale = immediate;
                    
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_ADD_ADDR_IMM_SCALED"
                                           :[NSString stringWithFormat:@"scale (%u)",scale]];
                    
                    address += scale * ptrSize;
                    */
                } break;
                    
                case REBASE_OPCODE_DO_REBASE_IMM_TIMES:
                {
                    /*
                    uint32_t count = immediate;
                    
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_DO_REBASE_IMM_TIMES"
                                           :[NSString stringWithFormat:@"count (%u)",count]];
                    
                    [node.details setAttributes:MVUnderlineAttributeName,@"YES",nil];
                    
                    for (uint32_t index = 0; index < count; index++)
                    {
                        [self rebaseAddress:address type:type node:actionNode location:doRebaseLocation];
                        address += ptrSize;
                    }
                    
                    doRebaseLocation = NSMaxRange(range);
                    */
                    
                } break;
                    
                case REBASE_OPCODE_DO_REBASE_ULEB_TIMES:
                {
                    /*
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_DO_REBASE_ULEB_TIMES"
                                           :@""];
                    
                    uint32_t startNextRebase = NSMaxRange(range);
                    
                    uint64_t count = [self read_uleb128:range lastReadHex:&lastReadHex];
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"uleb128"
                                           :[NSString stringWithFormat:@"count (%qu)",count]];
                    
                    [node.details setAttributes:MVUnderlineAttributeName,@"YES",nil];
                    
                    for (uint64_t index = 0; index < count; index++)
                    {
                        [self rebaseAddress:address type:type node:actionNode location:doRebaseLocation];
                        address += ptrSize;
                    }
                    
                    doRebaseLocation = startNextRebase;
                    */
                    
                } break;
                    
                case REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB:
                {
                    /*
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB"
                                           :@""];
                    
                    uint32_t startNextRebase = NSMaxRange(range);
                    
                    uint64_t offset = [self read_uleb128:range lastReadHex:&lastReadHex];
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"uleb128"
                                           :[NSString stringWithFormat:@"offset (%qi)",offset]];
                    
                    [node.details setAttributes:MVUnderlineAttributeName,@"YES",nil];
                    
                    [self rebaseAddress:address type:type node:actionNode location:doRebaseLocation];
                    address += ptrSize + offset;
                    
                    doRebaseLocation = startNextRebase;
                    */
                    
                } break;
                    
                case REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB: 
                {
                    /*
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB"
                                           :@""];
                    
                    uint32_t startNextRebase = NSMaxRange(range);
                    
                    uint64_t count = [self read_uleb128:range lastReadHex:&lastReadHex];
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"uleb128"
                                           :[NSString stringWithFormat:@"count (%qu)",count]];
                    
                    uint64_t skip = [self read_uleb128:range lastReadHex:&lastReadHex];
                    [node.details appendRow:[NSString stringWithFormat:@"%.8lX", range.location]
                                           :lastReadHex
                                           :@"uleb128"
                                           :[NSString stringWithFormat:@"skip (%qu)",skip]];
                    
                    [node.details setAttributes:MVUnderlineAttributeName,@"YES",nil];
                    
                    for (uint64_t index = 0; index < count; index++) 
                    {
                        [self rebaseAddress:address type:type node:actionNode location:doRebaseLocation];
                        address += ptrSize + skip;
                    }
                    
                    doRebaseLocation = startNextRebase;
                    */
                    
                } break;
                    
                default:
                    return false;
                    break;
            }
            
            ptr++;
        }

        return isDone;
    }
    
    bool MachOFile::parse_LC_DYLD_INFOS(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct dyld_info_command)) {
            warnx("Incorrect name size");
            return false;
        }
        
        const struct dyld_info_command* dyld_info_cmd = (const struct dyld_info_command*)load_cmd_info->cmd;
        
        if (dyld_info_cmd->rebase_off * dyld_info_cmd->rebase_size > 0)
        {
            // TODO: createRebaseNode
        }
        
        if (dyld_info_cmd->bind_off * dyld_info_cmd->bind_size > 0)
        {
            if (!parse_binding_node(input, dyld_info_cmd)) {
                return false;
            }
        }
        
        if (dyld_info_cmd->weak_bind_off * dyld_info_cmd->weak_bind_size > 0)
        {
            // TODO: createBindingNode
        }
        
        if (dyld_info_cmd->lazy_bind_off * dyld_info_cmd->lazy_bind_size > 0)
        {
            // TODO: createBindingNode
        }
        
        if (dyld_info_cmd->export_off * dyld_info_cmd->export_size > 0)
        {
            // TODO: createExportNode
        }

        return true;
    }
    
    bool MachOFile::parse_load_commands(macho_input_t *input)
    {
        const struct load_command* cmd = (const struct load_command*)macho_offset(input, m_header, m_header_size, sizeof(struct load_command));
        if (cmd == NULL)
            return false;
        
        uint32_t ncmds = read32(m_header->ncmds);
        
        /* Iterate over the load commands */
        for (uint32_t i = 0; i < ncmds; i++) {
            /* Load the full command */
            uint32_t cmdsize = read32(cmd->cmdsize);
            cmd = (const struct load_command*)macho_read(input, cmd, cmdsize);
            if (cmd == NULL) {
                return false;
            }
            
            /* Handle known types */
            uint32_t cmd_type = read32(cmd->cmd);
            
            load_command_info_t load_cmd_info;
            load_cmd_info.cmd_type = cmd_type;
            load_cmd_info.cmd = cmd;
            load_cmd_info.cmd_info = NULL;
            
            switch (cmd_type) {
                case LC_SEGMENT:
                {
                    /*
                    MATCH_STRUCT(segment_command,location)
                    node = [self createLCSegmentNode:parent
                                             caption:[NSString stringWithFormat:@"%@ (%s)",
                                                      caption, string(segment_command->segname,16).c_str()]
                                            location:location
                                     segment_command:segment_command];
                    
                    // preserv segment RVA/size for offset lookup
                    segmentInfo[segment_command->fileoff + imageOffset] = make_pair(segment_command->vmaddr, segment_command->vmsize);
                    
                    // preserv load segment command info for latter use
                    segments.push_back(segment_command);
                    
                    // Section Headers
                    for (uint32_t nsect = 0; nsect < segment_command->nsects; ++nsect)
                    {
                        uint32_t sectionloc = location + sizeof(struct segment_command) + nsect * sizeof(struct section);
                        MATCH_STRUCT(section,sectionloc)
                        [self createSectionNode:node
                                        caption:[NSString stringWithFormat:@"Section Header (%s)",
                                                 string(section->sectname,16).c_str()]
                                       location:sectionloc
                                        section:section];
                        
                        // preserv section fileOffset/sectName for RVA lookup
                        NSDictionary * userInfo = [self userInfoForSection:section];
                        insertChild[section->addr] = make_pair(section->offset + imageOffset, userInfo);
                        
                        // preserv header info for latter use
                        sections.push_back(section);
                    }
                    */
                } break;
                    
                case LC_SEGMENT_64:
                {
                    if (!parse_LC_SEGMENT_64(input, cmd_type, cmdsize, &load_cmd_info)) {
                        return false;
                    }
                } break;
                    
                case LC_SYMTAB:
                {
                    /*
                    MATCH_STRUCT(symtab_command,location)
                    
                    node = [self createLCSymtabNode:parent
                                            caption:caption
                                           location:location
                                     symtab_command:symtab_command];
                    
                    strtab = (char *)((uint8_t *)[dataController.fileData bytes] + imageOffset + symtab_command->stroff);
                    
                    for (uint32_t nsym = 0; nsym < symtab_command->nsyms; ++nsym)
                    {
                        if ([self is64bit] == NO)
                        {
                            MATCH_STRUCT(nlist,imageOffset + symtab_command->symoff + nsym * sizeof(struct nlist))
                            symbols.push_back (nlist);
                        }
                        else // 64bit
                        {
                            MATCH_STRUCT(nlist_64,imageOffset + symtab_command->symoff + nsym * sizeof(struct nlist_64))
                            symbols_64.push_back (nlist_64);
                        }
                        
                    }
                    */
                } break;
                    
                case LC_DYSYMTAB:
                {
                    /*
                    MATCH_STRUCT(dysymtab_command,location)
                    node = [self createLCDysymtabNode:parent
                                              caption:caption
                                             location:location
                                     dysymtab_command:dysymtab_command];
                    */
                } break;
                    
                case LC_TWOLEVEL_HINTS:
                {
                    /*
                    MATCH_STRUCT(twolevel_hints_command,location)
                    node = [self createLCTwolevelHintsNode:parent
                                                   caption:caption
                                                  location:location
                                    twolevel_hints_command:twolevel_hints_command];
                    */
                } break;
                    
                case LC_ID_DYLINKER:
                case LC_LOAD_DYLINKER:
                {
                    /*
                    MATCH_STRUCT(dylinker_command,location)
                    node = [self createLCDylinkerNode:parent
                                              caption:caption
                                             location:location
                                     dylinker_command:dylinker_command];
                    */
                } break;
                
                case LC_PREBIND_CKSUM:
                {
                    /*
                    MATCH_STRUCT(prebind_cksum_command,location)
                    node = [self createLCPrebindChksumNode:parent
                                                   caption:caption
                                                  location:location
                                     prebind_cksum_command:prebind_cksum_command];
                    */
                } break;
                
                case LC_UUID:
                {
                    /*
                    MATCH_STRUCT(uuid_command,location)
                    node = [self createLCUUIDNode:parent
                                          caption:caption
                                         location:location
                                     uuid_command:uuid_command];
                    */
                } break;
                
                case LC_THREAD:
                case LC_UNIXTHREAD:
                {
                    /*
                    MATCH_STRUCT(thread_command,location)
                    node = [self createLCThreadNode:parent
                                            caption:caption
                                           location:location
                                     thread_command:thread_command];
                    */
                } break;
                
                case LC_ID_DYLIB:
                case LC_LOAD_DYLIB:
                case LC_LOAD_WEAK_DYLIB:
                case LC_REEXPORT_DYLIB:
                case LC_LAZY_LOAD_DYLIB:
#ifdef __MAC_10_7
                case LC_LOAD_UPWARD_DYLIB:
#endif
                {
                    if (!parse_LC_DYLIBS(input, cmd_type, cmdsize, &load_cmd_info)) {
                        return false;
                    }
                } break;
                
                case LC_CODE_SIGNATURE:
                case LC_SEGMENT_SPLIT_INFO:
#ifdef __MAC_10_7
                case LC_FUNCTION_STARTS:
#endif
                {
                    /*
                    MATCH_STRUCT(linkedit_data_command,location)
                    node = [self createLCLinkeditDataNode:parent
                                                  caption:caption
                                                 location:location
                                    linkedit_data_command:linkedit_data_command];
                    */
                } break;
                    
                case LC_ENCRYPTION_INFO:
                {
                    /*
                    MATCH_STRUCT(encryption_info_command, location)
                    node = [self createLCEncryptionInfoNode:parent
                                                    caption:caption
                                                   location:location
                                    encryption_info_command:encryption_info_command];
                    */
                } break;
                    
                case LC_RPATH:
                {
                    if (!parse_LC_RPATH(input, cmd_type, cmdsize, &load_cmd_info)) {
                        return false;
                    }
                } break;
                    
                case LC_ROUTINES:
                {
                    /*
                    MATCH_STRUCT(routines_command,location)
                    node = [self createLCRoutinesNode:parent 
                                              caption:caption
                                             location:location
                                     routines_command:routines_command];
                    */
                } break; 
                    
                case LC_ROUTINES_64:
                {
                    /*
                    MATCH_STRUCT(routines_command_64,location)
                    node = [self createLCRoutines64Node:parent 
                                                caption:caption
                                               location:location
                                    routines_command_64:routines_command_64];
                    */
                } break;   
                    
                case LC_SUB_FRAMEWORK:
                {
                    /*
                    MATCH_STRUCT(sub_framework_command,location)
                    node = [self createLCSubFrameworkNode:parent 
                                                  caption:caption
                                                 location:location
                                    sub_framework_command:sub_framework_command];
                    */
                } break; 
                    
                case LC_SUB_UMBRELLA:
                {
                    /*
                    MATCH_STRUCT(sub_umbrella_command,location)
                    node = [self createLCSubUmbrellaNode:parent 
                                                 caption:caption
                                                location:location
                                    sub_umbrella_command:sub_umbrella_command];
                    */
                } break; 
                    
                case LC_SUB_CLIENT:
                {
                    /*
                    MATCH_STRUCT(sub_client_command,location)
                    node = [self createLCSubClientNode:parent 
                                               caption:caption
                                              location:location
                                    sub_client_command:sub_client_command];
                     */
                } break; 
                    
                case LC_SUB_LIBRARY:
                {
                    /*
                    MATCH_STRUCT(sub_library_command,location)
                    node = [self createLCSubLibraryNode:parent 
                                                caption:caption
                                               location:location
                                    sub_library_command:sub_library_command];
                    */
                } break; 
                
                case LC_DYLD_INFO:
                case LC_DYLD_INFO_ONLY:
                {
                    if (!parse_LC_DYLD_INFOS(input, cmd_type, cmdsize, &load_cmd_info)) {
                        return false;
                    }
                } break;   
                
#ifdef __MAC_10_7
                case LC_VERSION_MIN_MACOSX:
                case LC_VERSION_MIN_IPHONEOS:
                {
                    /*
                    MATCH_STRUCT(version_min_command,location)
                    node = [self createLCVersionMinNode:parent 
                                                caption:caption
                                               location:location
                                    version_min_command:version_min_command];
                    */
                } break;
#endif
                
                default:
                    break;
            }
            
            m_load_command_infos.push_back(load_cmd_info);

            /* Load the next command */
            cmd = (const struct load_command*)macho_offset(input, cmd, cmdsize, sizeof(struct load_command));
            if (cmd == NULL) {
                return false;
            }
        }
        
        return true;
    }
    
    uint64_t MachOFile::getOffset(void* address)
    {
        return (uint8_t*)address - (uint8_t*)m_baseAddress;
    }
    
    /* Parse a Mach-O header */
    bool MachOFile::parse_macho(macho_input_t *input) {
        m_baseAddress = input->data;
        
        /* Read the file type. */
        const uint32_t* magic = (const uint32_t*)macho_read(input, input->data, sizeof(uint32_t));
        if (magic == NULL)
            return false;
        
        switch (*magic) {
            case MH_CIGAM:
                m_is_need_byteswap = true;
                // Fall-through
                
            case MH_MAGIC:
                m_header_size = sizeof(*m_header);
                m_header = (const struct mach_header*)macho_read(input, input->data, m_header_size);
                if (m_header == NULL) {
                    return false;
                }
                break;
                
            case MH_CIGAM_64:
                m_is_need_byteswap = true;
                // Fall-through
                
            case MH_MAGIC_64:
                m_header_size = sizeof(*m_header64);
                m_header64 = (const struct mach_header_64*)macho_read(input, input->data, m_header_size);
                if (m_header64 == NULL)
                    return false;
                
                /* The 64-bit header is a direct superset of the 32-bit header */
                m_header = (struct mach_header *)m_header64;
                
                m_is64 = true;
                break;
                
            case FAT_CIGAM:
            case FAT_MAGIC:
                m_fat_header = (const struct fat_header*)macho_read(input, input->data, sizeof(*m_fat_header));
                m_is_universal = true;
                break;
                
            default:
                warnx("Unknown Mach-O magic: 0x%x", *magic);
                return false;
        }
        
        /* Parse universal file. */
        if (m_is_universal) {
            return parse_universal(input);
        }
        
        /* Fetch the arch name */
        m_archInfo = NXGetArchInfoFromCpuType(read32(m_header->cputype), read32(m_header->cpusubtype));
        
        /* Parse the Mach-O load commands */
        return parse_load_commands(input);
    }

    bool MachOFile::parse_file(const char* path)
    {
        m_fd = open(path, O_RDONLY);
        if (m_fd < 0) {
            return false;
        }
        
        if (fstat(m_fd, &m_stbuf) != 0) {
            return false;
        }
        
        /* mmap */
        m_data = mmap(NULL, m_stbuf.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE, m_fd, 0);
        if (m_data == MAP_FAILED) {
            return false;
        }
        
        /* Parse */
        macho_input_t input_file;
        input_file.data = m_data;
        input_file.length = m_stbuf.st_size;

        return parse_macho(&input_file);
    }

}

