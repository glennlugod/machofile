//
//  macho.cpp
//  Easter Bunny
//
//  Created by Glenn on 12/29/12.
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
    const void* MachOFile::macho_offset (macho_input_t *input, const void *address, size_t offset, size_t length) {
        void *result = ((uint8_t *) address) + offset;
        return macho_read(input, result, length);
    }
    
    /* return a human readable formatted version number. the result must be free()'d. */
    char* MachOFile::macho_format_dylib_version(uint32_t version) {
        char *result = NULL;
        asprintf(&result, "%u.%u.%u", (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
        return result;
    }
    
    /* Some byteswap wrappers */
    static uint32_t macho_swap32 (uint32_t input) {
        return OSSwapInt32(input);
    }
    
    static uint32_t macho_nswap32(uint32_t input) {
        return input;
    }
    
    /* Parse a Mach-O header */
    bool MachOFile::parse_macho(macho_input_t *input) {
        /* Read the file type. */
        const uint32_t* magic = (const uint32_t*)macho_read(input, input->data, sizeof(uint32_t));
        if (magic == NULL)
            return false;
        
        /* Parse the Mach-O header */
        uint32_t (*swap32)(uint32_t) = macho_nswap32;
                
        switch (*magic) {
            case MH_CIGAM:
                m_is_need_byteswap = true;
                swap32 = macho_swap32;
                // Fall-through
                
            case MH_MAGIC:
                m_header_size = sizeof(*m_header);
                m_header = (const struct mach_header*)macho_read(input, input->data, m_header_size);
                if (m_header == NULL) {
                    return false;
                }
                //printf("Type: Mach-O 32-bit\n");
                break;
                
            case MH_CIGAM_64:
                m_is_need_byteswap = true;
                swap32 = macho_swap32;
                // Fall-through
                
            case MH_MAGIC_64:
                m_header_size = sizeof(*m_header64);
                m_header64 = (const struct mach_header_64*)macho_read(input, input->data, m_header_size);
                if (m_header64 == NULL)
                    return false;
                
                /* The 64-bit header is a direct superset of the 32-bit header */
                m_header = (struct mach_header *)m_header64;
                
                //printf("Type: Mach-O 64-bit\n");
                m_is64 = true;
                break;
                
            case FAT_CIGAM:
            case FAT_MAGIC:
                m_fat_header = (const struct fat_header*)macho_read(input, input->data, sizeof(*m_fat_header));
                m_is_universal = true;
                //printf("Type: Universal\n");
                break;
                
            default:
                //warnx("Unknown Mach-O magic: 0x%x", *magic);
                return false;
        }
        
        /* Parse universal file. */
        if (m_is_universal) {
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
                
                /*
                / * Parse the architecture's Mach-O header * /
                printf("\n");
                if (!parse_macho(&arch_input))
                    return false;
                */
            }
            
            return true;
        }
        
        /* Fetch the arch name */
        m_archInfo = NXGetArchInfoFromCpuType(swap32(m_header->cputype), swap32(m_header->cpusubtype));
        /*
        if (m_archInfo != NULL) {
            printf("Architecture: %s\n", m_archInfo->name);
        }
        */
        
        /* Parse the Mach-O load commands */
        const struct load_command* cmd = (const struct load_command*)macho_offset(input, m_header, m_header_size, sizeof(struct load_command));
        if (cmd == NULL)
            return false;
        uint32_t ncmds = swap32(m_header->ncmds);
        
        /* Iterate over the load commands */
        for (uint32_t i = 0; i < ncmds; i++) {
            /* Load the full command */
            uint32_t cmdsize = swap32(cmd->cmdsize);
            cmd = (const struct load_command*)macho_read(input, cmd, cmdsize);
            if (cmd == NULL)
                return false;
            
            /* Handle known types */
            uint32_t cmd_type = swap32(cmd->cmd);
            
            switch (cmd_type) {
                case LC_RPATH: {
                    if (cmdsize < sizeof(struct rpath_command)) {
                        warnx("Incorrect cmd size");
                        return false;
                    }
                    
                    /* Fetch the path */
                    size_t pathlen = cmdsize - sizeof(struct rpath_command);
                    const char* pathptr = (const char*)macho_offset(input, cmd, sizeof(struct rpath_command), pathlen);
                    if (pathptr == NULL)
                        return false;
                    
                    /*
                    char* path = (char*)malloc(pathlen);
                    strlcpy(path, pathptr, pathlen);
                    printf("[rpath] path=%s\n", path);
                    free(path);
                    */
                    
                    struct runpath_additions_info info;
                    info.cmd_type = cmd_type;
                    info.cmd = (struct rpath_command*)cmd;
                    info.path = pathptr;
                    info.pathlen = pathlen;
                    
                    m_runpath_additions_infos.push_back(info);
                    break;
                }
                
                case LC_ID_DYLIB:
                case LC_LOAD_WEAK_DYLIB:
                case LC_REEXPORT_DYLIB:
                case LC_LOAD_DYLIB: {
                    if (cmdsize < sizeof(struct dylib_command)) {
                        warnx("Incorrect name size");
                        return false;
                    }
                    
                    const struct dylib_command *dylib_cmd = (const struct dylib_command *)cmd;
                    
                    /* Extract the install name */
                    size_t namelen = cmdsize - sizeof(struct dylib_command);
                    const char* nameptr = (const char*)macho_offset(input, cmd, sizeof(struct dylib_command), namelen);
                    if (nameptr == NULL)
                        return false;
                    
                    /* Print the dylib info */
                    char *current_version = macho_format_dylib_version(swap32(dylib_cmd->dylib.current_version));
                    char *compat_version = macho_format_dylib_version(swap32(dylib_cmd->dylib.compatibility_version));
                    
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
                    break;
                }
                    
                default:
                    break;
            }
            
            /* Load the next command */
            cmd = (const struct load_command*)macho_offset(input, cmd, cmdsize, sizeof(struct load_command));
            if (cmd == NULL)
                return false;
        }
        
        return true;
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

        printf("Parsing: %s\n", path);
        return parse_macho(&input_file);
    }

}

