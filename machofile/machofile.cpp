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
#include <mach-o/swap.h>

#include "machofile.h"

namespace rotg {
    
    MachOFile::MachOFile()
        : m_fd(-1)
        , m_isInputOwned(false)
        , m_header(NULL)
        , m_header64(NULL)
        , m_header_size(0)
        , m_fat_header(NULL)
        , m_is64bit(false)
        , m_is_universal(false)
        , m_archInfo(NULL)
        , m_is_need_byteswap(false)
    {
        memset(&m_input, 0, sizeof(macho_input_t));
    }
    
    MachOFile::~MachOFile()
    {
        segment_command_64_infos_t::iterator s64i_iter;
        for (s64i_iter = m_segment_command_64_infos.begin(); s64i_iter != m_segment_command_64_infos.end(); s64i_iter++) {
            delete *s64i_iter;
        }
        
        dyld_info_command_infos_t::iterator dici_iter;
        for (dici_iter = m_dyld_info_command_infos.begin(); dici_iter != m_dyld_info_command_infos.end(); dici_iter++) {
            delete *dici_iter;
        }
        
        thread_command_infos_t::iterator tci_iter;
        for (tci_iter = m_thread_command_infos.begin(); tci_iter != m_thread_command_infos.end(); tci_iter++) {
            delete *tci_iter;
        }
        
        dylib_command_infos_t::iterator dli_iter;
        for (dli_iter = m_dylib_command_infos.begin(); dli_iter != m_dylib_command_infos.end(); dli_iter++) {
            delete *dli_iter;
        }
        
        if (m_isInputOwned && (m_input.data) && (m_input.data != MAP_FAILED)) {
            munmap((void*)m_input.data, m_input.length);
            m_input.data = NULL;
        }
        
        if (m_fd > 0) {
            close(m_fd);
            m_fd = -1;
        }
    }
    
    /* Verify that the given range is within bounds. */
    const void* MachOFile::macho_read(const void *address, size_t length) {
        if ((((uint8_t *) address) - ((uint8_t *) m_input.data)) + length > m_input.length) {
            warnx("Short read parsing Mach-O input");
            return NULL;
        }
        
        return address;
    }
    
    /* Verify that address + offset + length is within bounds. */
    const void* MachOFile::macho_offset(const void *address, size_t offset, size_t length) {
        void *result = ((uint8_t *) address) + offset;
        return macho_read(result, length);
    }
    
    const void* MachOFile::read_sleb128(const void *address, int64_t& result)
    {
        uint8_t *p = ((uint8_t *) address);
        result = 0;
        
        int bit = 0;
        uint8_t byte;
        
        do {
            byte = *p++;
            result |= ((byte & 0x7f) << bit);
            bit += 7;
        } while (byte & 0x80);
        
        // sign extend negative numbers
        if ( (byte & 0x40) != 0 )
        {
            result |= (-1LL) << bit;
        }
        
        return p;
    }
    
    const void* MachOFile::read_uleb128(const void *address, uint64_t& result)
    {
        uint8_t *p = ((uint8_t *) address);
        result = 0;
        
        int bit = 0;
        
        do {
            uint64_t slice = *p & 0x7f;
            
            if (bit >= 64 || slice << bit >> bit != slice)
                return NULL;
            else {
                result |= (slice << bit);
                bit += 7;
            }
        } 
        while (*p++ & 0x80);

        return p;
    }
    
    bool MachOFile::parse_universal()
    {
        uint32_t nfat = OSSwapBigToHostInt32(m_fat_header->nfat_arch);
        const struct fat_arch* archs = (const struct fat_arch*)macho_offset(m_fat_header, sizeof(struct fat_header), sizeof(struct fat_arch));
        if (archs == NULL) {
            return false;
        }
        
        //printf("Architecture Count: %d\n", nfat);
        for (uint32_t i = 0; i < nfat; i++) {
            const struct fat_arch* arch = (const struct fat_arch*)macho_read(archs + i, sizeof(struct fat_arch));
            if (arch == NULL)
                return false;
            
            /* Fetch a pointer to the architecture's Mach-O header. */
            size_t length = OSSwapBigToHostInt32(arch->size);
            const void *data = macho_offset(m_input.data, OSSwapBigToHostInt32(arch->offset), length);
            if (data == NULL)
                return false;
            
            fat_arch_info_t fat_arch_info;
            fat_arch_info.arch = *arch;
            swap_fat_arch(&fat_arch_info.arch, 1, NX_LittleEndian);
            fat_arch_info.ptr = arch;
            fat_arch_info.input.length = length;
            fat_arch_info.input.data = data;
            fat_arch_info.input.baseOffset = getOffset(data);
            
            m_fat_arch_infos.push_back(fat_arch_info);
        }
        
        return true;
    }
    
    bool MachOFile::parse_LC_SEGMENT_64(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct segment_command_64)) {
            warnx("Incorrect cmd size");
            return false;
        }
        
        const struct segment_command_64* segment_cmd_64 = (const struct segment_command_64*)load_cmd_info->cmd;
        
        // preserve segment RVA/size for offset lookup
        m_segmentInfo[segment_cmd_64->fileoff] = std::make_pair(segment_cmd_64->vmaddr, segment_cmd_64->vmsize);
        
        segment_command_64_info_t* info = new segment_command_64_info_t();
        if (info == NULL) {
            return false;
        }
        
        info->cmd_type = cmd_type;
        info->cmd = segment_cmd_64;
        
        m_segment_command_64_infos.push_back(info);
        
        // Section Headers
        for (uint32_t nsect = 0; nsect < segment_cmd_64->nsects; ++nsect)
        {
            const struct section_64* section = (const struct section_64*)macho_offset(segment_cmd_64, sizeof(struct segment_command_64) + nsect * sizeof(struct section_64), sizeof(struct section_64));
            if (section == NULL) {
                return false;
            }
            
            info->section_64s.push_back(section);
            m_section_64s.push_back(section);
        }
        
        load_cmd_info->cmd_info = info;
        
        return true;
    }
    
    bool MachOFile::parse_LC_RPATH(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct rpath_command)) {
            warnx("Incorrect cmd size");
            return false;
        }
        
        /* Fetch the path */
        size_t pathlen = cmdsize - sizeof(struct rpath_command);
        const char* pathptr = (const char*)macho_offset(load_cmd_info->cmd, sizeof(struct rpath_command), pathlen);
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
    
    bool MachOFile::parse_LC_DYLIB(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct dylib_command)) {
            warnx("Incorrect name size");
            return false;
        }
        
        const struct dylib_command *dylib_cmd = (const struct dylib_command *)load_cmd_info->cmd;
        
        /* Extract the install name */
        size_t namelen = cmdsize - sizeof(struct dylib_command);
        const char* nameptr = (const char*)macho_offset(load_cmd_info->cmd, sizeof(struct dylib_command), namelen);
        if (nameptr == NULL) {
            return false;
        }
        
        dylib_command_info_t* dl_info = new dylib_command_info_t();
        if (dl_info == NULL) {
            return false;
        }
        
        dl_info->cmd_type = cmd_type;
        dl_info->cmd = dylib_cmd;
        dl_info->libname = nameptr;
        dl_info->libnamelen = namelen;
        
        m_dylib_command_infos.push_back(dl_info);
        load_cmd_info->cmd_info = dl_info;
        
        return true;
    }
    
    bool MachOFile::parse_rebase_node(const struct dyld_info_command* dyld_info_cmd, uint64_t baseAddress)
    {
        const uint8_t* ptr = (const uint8_t*)macho_offset(m_input.data, dyld_info_cmd->bind_off, dyld_info_cmd->bind_size);
        if (ptr == NULL) {
            return false;
        }

        const uint8_t* endAddress = ptr + dyld_info_cmd->bind_size;
        
        //const uint8_t* address = base_addr;
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
    
    bool MachOFile::parse_binding_node(binding_info_t* binding_info, uint64_t location, uint32_t length, BindNodeType nodeType, uint64_t baseAddress)
    {
        uint64_t libOrdinal = 0;
        uint32_t type = 0;
        int64_t addend = 0;
        const char* symbolName = NULL;
        uint32_t symbolFlags = 0;
        
        uint64_t doBindLocation = location;
        
        const uint8_t* ptr = (const uint8_t*)macho_offset(m_input.data, location, length);
        if (ptr == NULL) {
            return false;
        }
        
        const uint8_t* endAddress = ptr + length;
        
        uint64_t ptrSize = (is64bit() ? sizeof(uint64_t) : sizeof(uint32_t));
        uint64_t address = baseAddress;
        bool isDone = false;
        
        while ((ptr < endAddress) && !isDone) {
            uint8_t opcode = *ptr & REBASE_OPCODE_MASK;
            uint8_t immediate = *ptr & REBASE_IMMEDIATE_MASK;
            
            bind_opcode_t bind_op;
            bind_op.opcode = opcode;
            bind_op.immediate = immediate;
            bind_op.ptr = ptr;
            
            ptr++;
            
            switch (opcode)
            {
                case BIND_OPCODE_DONE:                    
                    // The lazy bindings have one of these at the end of each bind.
                    if (nodeType != NodeTypeLazyBind)
                    {
                        isDone = true;
                    }
                    
                    doBindLocation = getOffset((void*)ptr);
                    break;
                    
                case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
                    libOrdinal = immediate;
                    break;
                    
                case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
                    ptr = (const u_int8_t*)read_uleb128(ptr, libOrdinal);
                    if (ptr == NULL) {
                        return false;
                    }
                    break;
                    
                case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
                    // Special means negative
                    if (immediate == 0)
                    {
                        libOrdinal = 0;
                    }
                    else
                    {
                        int8_t signExtended = immediate | BIND_OPCODE_MASK; // This sign extends the value
                        
                        libOrdinal = signExtended;
                    }
                    break;
                    
                case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM: {
                    symbolFlags = immediate;
                    symbolName = (const char*)ptr;
                    ptr += strlen(symbolName) + 1;
                } break;
                    
                case BIND_OPCODE_SET_TYPE_IMM:
                    type = immediate;
                    break;
                    
                case BIND_OPCODE_SET_ADDEND_SLEB:
                    ptr = (const u_int8_t*)read_sleb128(ptr, addend);
                    if (ptr == NULL) {
                        return false;
                    }
                    break;
                    
                case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
                {
                    uint32_t segmentIndex = immediate;

                    uint64_t val;
                    ptr = (const u_int8_t*)read_uleb128(ptr, val);
                    if (ptr == NULL) {
                        return false;
                    }
                    
                    if (is64bit()) {
                        if (segmentIndex >= m_segment_command_64_infos.size()) {
                            return false;
                        }
                        
                        address = m_segment_command_64_infos[segmentIndex]->cmd->vmaddr + val;
                    } else if (is32bit()) {
                        // TODO: index vs. check size
                        
                        // TODO: address = segments.at(segmentIndex)->vmaddr;
                    }
                } break;
                    
                case BIND_OPCODE_ADD_ADDR_ULEB:
                {
                    uint64_t val;
                    ptr = (const u_int8_t*)read_uleb128(ptr, val);
                    if (ptr == NULL) {
                        return false;
                    }

                    address += val;
                } break;
                    
                case BIND_OPCODE_DO_BIND:
                {
                    bind_action_t bindAction;
                    bindAction.address = address;
                    bindAction.type = type;
                    bindAction.symbolName = symbolName;
                    bindAction.flags = symbolFlags;
                    bindAction.addend = addend;
                    bindAction.libOrdinal = libOrdinal;
                    bindAction.nodeType = nodeType;
                    bindAction.location = doBindLocation;
                    bindAction.ptrSize = ptrSize;
                    
                    binding_info->actions.push_back(bindAction);
                    
                    doBindLocation = getOffset((void*)ptr);

                    address += ptrSize;
                } break;
                    
                case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
                {
                    uint64_t startNextBind = getOffset((void*)ptr);
                    
                    uint64_t val;
                    ptr = (const u_int8_t*)read_uleb128(ptr, val);
                    if (ptr == NULL) {
                        return false;
                    }
                    
                    bind_action_t bindAction;
                    bindAction.address = address;
                    bindAction.type = type;
                    bindAction.symbolName = symbolName;
                    bindAction.flags = symbolFlags;
                    bindAction.addend = addend;
                    bindAction.libOrdinal = libOrdinal;
                    bindAction.nodeType = nodeType;
                    bindAction.location = doBindLocation;
                    bindAction.ptrSize = ptrSize;
                    
                    binding_info->actions.push_back(bindAction);
                    
                    doBindLocation = startNextBind;
                    
                    address += ptrSize + val;
                } break;
                    
                case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
                {
                    uint32_t scale = immediate;
                    
                    bind_action_t bindAction;
                    bindAction.address = address;
                    bindAction.type = type;
                    bindAction.symbolName = symbolName;
                    bindAction.flags = symbolFlags;
                    bindAction.addend = addend;
                    bindAction.libOrdinal = libOrdinal;
                    bindAction.nodeType = nodeType;
                    bindAction.location = doBindLocation;
                    bindAction.ptrSize = ptrSize;
                    
                    binding_info->actions.push_back(bindAction);
                    
                    doBindLocation = getOffset((void*)ptr);
                    
                    address += ptrSize + scale * ptrSize;
                } break;
                    
                case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB: 
                {
                    uint64_t startNextBind = getOffset((void*)ptr);
                    
                    uint64_t count;
                    ptr = (const u_int8_t*)read_uleb128(ptr, count);
                    if (ptr == NULL) {
                        return false;
                    }

                    uint64_t skip;
                    ptr = (const u_int8_t*)read_uleb128(ptr, skip);
                    if (ptr == NULL) {
                        return false;
                    }
                    
                    for (uint64_t index = 0; index < count; index++) 
                    {
                        bind_action_t bindAction;
                        bindAction.address = address;
                        bindAction.type = type;
                        bindAction.symbolName = symbolName;
                        bindAction.flags = symbolFlags;
                        bindAction.addend = addend;
                        bindAction.libOrdinal = libOrdinal;
                        bindAction.nodeType = nodeType;
                        bindAction.location = doBindLocation;
                        bindAction.ptrSize = ptrSize;
                        
                        binding_info->actions.push_back(bindAction);
                        
                        doBindLocation = startNextBind;
                        
                        address += ptrSize + skip;
                    }
                } break;
                    
                default:
                    return false;
            }
            
            binding_info->opcodes.push_back(bind_op);
        }
        
        return true;
    }
    
    bool MachOFile::parse_export_node(export_info_t* exportInfo, const char* prefix, uint64_t location, uint64_t length, uint64_t skipBytes, uint64_t baseAddress)
    {
        const uint8_t* ptr = (const uint8_t*)macho_offset(m_input.data, location + skipBytes, length);
        if (ptr == NULL) {
            return false;
        }
        
        export_opcode_t exportOpcode;
        exportOpcode.ptr = ptr;
        
        uint8_t terminalSize = *ptr;
        exportOpcode.terminalSize = terminalSize;
        ptr++;
        
        if (terminalSize != 0) {
            export_action_t exportAction;
            
            ptr = (const uint8_t*)read_uleb128(ptr, exportAction.flags);
            if (ptr == NULL) {
                return false;
            }
            
            uint64_t offset;
            ptr = (const uint8_t*)read_uleb128(ptr, offset);
            if (ptr == NULL) {
                return false;
            }
            exportAction.offset = offset;
            
            exportAction.symbolName = prefix;
            exportAction.address = baseAddress + offset;
            
            exportInfo->actions.push_back(exportAction);
        }
        
        uint8_t childCount = *ptr;
        exportOpcode.childCount = childCount;
        ptr++;
        
        while (childCount-- > 0) {
            export_node_t exportNode;
            
            exportNode.label = (const char*)ptr;
            ptr += strlen(exportNode.label) + 1;
            
            uint64_t skip;
            ptr = (const uint8_t*)read_uleb128(ptr, skip);
            if (ptr == NULL) {
                return false;
            }
            exportNode.skip = skip;
            
            std::string _prefix = prefix;
            _prefix += exportNode.label;
            
            exportOpcode.nodes.push_back(exportNode);
            
            if (!parse_export_node(exportInfo, _prefix.c_str(), location, length - skip, skip, baseAddress)) {
                return false;
            }
        }
        
        exportInfo->opcodes.push_back(exportOpcode);
        
        return true;
    }
    
    bool MachOFile::parse_LC_DYLD_INFO(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        if (cmdsize < sizeof(struct dyld_info_command)) {
            warnx("Incorrect name size");
            return false;
        }
        
        uint64_t base_addr = 0;
        
        /* Iterate over the load commands */
        load_command_infos_t::iterator iter;
        for (iter = m_load_command_infos.begin(); iter != m_load_command_infos.end(); iter++) {
            
            load_command_info_t* load_cmd_info = &(*iter);
            uint32_t cmd_type = load_cmd_info->cmd_type;
            
            switch (cmd_type) {
                case LC_SEGMENT: {
                    struct segment_command const * segment_command = (struct segment_command const *)load_cmd_info->cmd;
                    if (segment_command->fileoff == 0 && segment_command->filesize != 0)
                    {
                        base_addr = segment_command->vmaddr;
                    }
                } break;
                    
                case LC_SEGMENT_64: {
                    struct segment_command_64 const * segment_command_64 = (struct segment_command_64 const *)load_cmd_info->cmd;
                    if (segment_command_64->fileoff == 0 && segment_command_64->filesize != 0)
                    {
                        base_addr = segment_command_64->vmaddr;
                    }
                    
                } break;
            }
        }
        
        dyld_info_command_info_t* cmd_info = new dyld_info_command_info_t();
        if (cmd_info == NULL) {
            return false;
        }
        
        m_dyld_info_command_infos.push_back(cmd_info);
        load_cmd_info->cmd_info = cmd_info;

        const struct dyld_info_command* dyld_info_cmd = (const struct dyld_info_command*)load_cmd_info->cmd;
        cmd_info->cmd = dyld_info_cmd;
        cmd_info->cmd_type = cmd_type;
        
        if (dyld_info_cmd->rebase_off * dyld_info_cmd->rebase_size > 0)
        {
            // TODO: createRebaseNode
            /*
            if (!parse_rebase_node(input, dyld_info_cmd)) {
                return false;
            }
            */
        }
        
        if (dyld_info_cmd->bind_off * dyld_info_cmd->bind_size > 0)
        {
            if (!parse_binding_node(&cmd_info->loader_info.binding_info, dyld_info_cmd->bind_off, dyld_info_cmd->bind_size, NodeTypeBind, base_addr)) {
                return false;
            }
        }
        
        if (dyld_info_cmd->weak_bind_off * dyld_info_cmd->weak_bind_size > 0)
        {
            if (!parse_binding_node(&cmd_info->loader_info.weak_binding_info, dyld_info_cmd->weak_bind_off, dyld_info_cmd->weak_bind_size, NodeTypeWeakBind, base_addr)) {
                return false;
            }
        }
        
        if (dyld_info_cmd->lazy_bind_off * dyld_info_cmd->lazy_bind_size > 0)
        {
            if (!parse_binding_node(&cmd_info->loader_info.lazy_binding_info, dyld_info_cmd->lazy_bind_off, dyld_info_cmd->lazy_bind_size, NodeTypeLazyBind, base_addr)) {
                return false;
            }
        }
        
        if (dyld_info_cmd->export_off * dyld_info_cmd->export_size > 0)
        {
            if (!parse_export_node(&cmd_info->loader_info.export_info, "", dyld_info_cmd->export_off, dyld_info_cmd->export_size, 0, base_addr)) {
                return false;
            }
        }

        return true;
    }
    
    bool MachOFile::parse_LC_THREAD(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info)
    {
        const struct thread_command* cmd = (const struct thread_command*)load_cmd_info->cmd;
        
        thread_command_info_t* cmd_info = new thread_command_info_t();
        if (cmd_info == NULL) {
            return false;
        }
        
        cmd_info->cmd_type = cmd_type;
        cmd_info->cmd = cmd;
        
        m_thread_command_infos.push_back(cmd_info);
        load_cmd_info->cmd_info = cmd_info;
        
        return true;
    }
    
    bool MachOFile::parse_load_commands()
    {
        const struct load_command* cmd = (const struct load_command*)macho_offset(m_header, m_header_size, sizeof(struct load_command));
        if (cmd == NULL) {
            return false;
        }
        
        uint32_t ncmds = read32(m_header->ncmds);
        
        /* Get the load commands */
        for (uint32_t i = 0; i < ncmds; i++) {
            /* Load the full command */
            uint32_t cmdsize = read32(cmd->cmdsize);
            cmd = (const struct load_command*)macho_read(cmd, cmdsize);
            if (cmd == NULL) {
                return false;
            }
            
            /* Handle known types */
            uint32_t cmd_type = read32(cmd->cmd);
            
            load_command_info_t load_cmd_info;
            load_cmd_info.cmd_type = cmd_type;
            load_cmd_info.cmd = cmd;
            load_cmd_info.cmd_info = NULL;
            
            m_load_command_infos.push_back(load_cmd_info);

            /* Load the next command */
            cmd = (const struct load_command*)macho_offset(cmd, cmdsize, sizeof(struct load_command));
            if (cmd == NULL) {
                return false;
            }
        }
        
        /* Iterate over the load commands */
        load_command_infos_t::iterator iter;
        for (iter = m_load_command_infos.begin(); iter != m_load_command_infos.end(); iter++) {
            
            load_command_info_t* load_cmd_info = &(*iter);
            uint32_t cmd_type = load_cmd_info->cmd_type;
            uint32_t cmdsize = read32(load_cmd_info->cmd->cmdsize);
            
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
                    if (!parse_LC_SEGMENT_64(cmd_type, cmdsize, load_cmd_info)) {
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
                    if (!parse_LC_THREAD(cmd_type, cmdsize, load_cmd_info)) {
                        return false;
                    }
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
                    if (!parse_LC_DYLIB(cmd_type, cmdsize, load_cmd_info)) {
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
                    if (!parse_LC_RPATH(cmd_type, cmdsize, load_cmd_info)) {
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
                    if (!parse_LC_DYLD_INFO(cmd_type, cmdsize, load_cmd_info)) {
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
        }
        
        return true;
    }
    
    uint64_t MachOFile::getOffset(const void* address)
    {
        return ((uint8_t*)address - (uint8_t*)m_input.data) + m_input.baseOffset;
    }
    
    /* Parse a Mach-O header */
    bool MachOFile::parse_macho(const macho_input_t *input) {
        if (m_input.data == NULL) {
            m_input.data = input->data;
            m_input.length = input->length;
            m_input.baseOffset = input->baseOffset;
        }
        
        /* Read the file type. */
        const uint32_t* magic = (const uint32_t*)macho_read(input->data, sizeof(uint32_t));
        if (magic == NULL) {
            return false;
        }
        
        switch (*magic) {
            case MH_CIGAM:
                m_is_need_byteswap = true;
                // Fall-through
                
            case MH_MAGIC:
                m_header_size = sizeof(*m_header);
                m_header = (const struct mach_header*)macho_read(input->data, m_header_size);
                if (m_header == NULL) {
                    return false;
                }
                break;
                
            case MH_CIGAM_64:
                m_is_need_byteswap = true;
                // Fall-through
                
            case MH_MAGIC_64:
                m_header_size = sizeof(*m_header64);
                m_header64 = (const struct mach_header_64*)macho_read(input->data, m_header_size);
                if (m_header64 == NULL)
                    return false;
                
                /* The 64-bit header is a direct superset of the 32-bit header */
                m_header = (struct mach_header *)m_header64;
                
                m_is64bit = true;
                break;
                
            case FAT_CIGAM:
            case FAT_MAGIC:
                m_fat_header = (const struct fat_header*)macho_read(input->data, sizeof(struct fat_header));
                m_is_universal = true;
                break;
                
            default:
                warnx("Unknown Mach-O magic: 0x%x", *magic);
                return false;
        }
        
        /* Parse universal file. */
        if (m_is_universal) {
            return parse_universal();
        }
        
        /* Fetch the arch name */
        m_archInfo = NXGetArchInfoFromCpuType(read32(m_header->cputype), read32(m_header->cpusubtype));
        
        /* Parse the Mach-O load commands */
        return parse_load_commands();
    }

    bool MachOFile::parse_file(const char* path)
    {
        m_fd = open(path, O_RDONLY);
        if (m_fd < 0) {
            return false;
        }
        
        struct stat stbuf;
        if (fstat(m_fd, &stbuf) != 0) {
            return false;
        }
        
        /* mmap */
        m_input.data = mmap(NULL, stbuf.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE, m_fd, 0);
        if (m_input.data == MAP_FAILED) {
            return false;
        }
        
        m_input.length = stbuf.st_size;
        m_isInputOwned = true;

        /* Parse */
        return parse_macho(&m_input);
    }

}

