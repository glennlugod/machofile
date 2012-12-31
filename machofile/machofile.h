//
//  macho.h
//  Easter Bunny
//
//  Created by Glenn on 12/29/12.
//
//

#ifndef rotg_machofile_h
#define rotg_machofile_h

#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <mach-o/fat.h> // for fat structure decoding
#include <mach-o/arch.h> // to know which is local arch

#include <sys/stat.h>

#include <unistd.h>

#include <list>
#include <string>

namespace rotg {
    
    typedef struct dylib_info {
        uint32_t                    cmd_type;
        const struct dylib_command* cmd;
        const char*                 name;
        size_t                      namelen;
        std::string                 current_version;
        std::string                 compat_version;
    } dylib_info_t;
    
    typedef std::list<dylib_info_t> dylib_infos_t;

    typedef struct runpath_additions_info {
        uint32_t                    cmd_type;
        const struct rpath_command* cmd;
        const char*                 path;
        size_t                      pathlen;
    } runpath_additions_info_t;
    
    typedef std::list<runpath_additions_info_t> runpath_additions_infos_t;
    
    typedef struct macho_input {
        const void* data;
        size_t      length;
    } macho_input_t;
    
    typedef struct fat_arch_info {
        const struct fat_arch*  arch;
        macho_input_t           input;
    } fat_arch_info_t;
    
    typedef std::list<fat_arch_info_t> fat_arch_infos_t;
    
    class MachOFile
    {
    public:
        MachOFile();
        ~MachOFile();
        
        bool parse_macho(macho_input_t *input);
        bool parse_file(const char* path);
        
        const struct mach_header* getHeader() const {
            return m_header;
        }
        
        const struct mach_header_64* getHeader64() const {
            return m_header64;
        }
        
        size_t getHeaderSize() const {
            return m_header_size;
        }
        
        const struct fat_header* getFatHeader() const {
            return m_fat_header;
        }
        
        bool is32() const {
            return !m_is64 && !m_is_universal;
        }
        
        bool is64() const {
            return !is32();
        }
        
        bool isUniversal() const {
            return m_is_universal;
        }
        
        bool isNeedByteSwap() const {
            return m_is_need_byteswap;
        }
        
        const NXArchInfo* getArchInfo() const {
            return m_archInfo;
        }
        
        const dylib_infos_t& getDylibInfos() const {
            return m_dylib_infos;
        }
        
        const fat_arch_infos_t& getFatArchInfos() const {
            return m_fat_arch_infos;
        }
        
    private:
        const void* macho_read(macho_input_t* input, const void *address, size_t length);
        const void* macho_offset(macho_input_t *input, const void *address, size_t offset, size_t length);
        char* macho_format_dylib_version (uint32_t version);
        
        int                             m_fd;
        void*                           m_data;
        struct stat                     m_stbuf;
        
        const struct mach_header*       m_header;
        const struct mach_header_64*    m_header64;
        size_t                          m_header_size;
        const struct fat_header*        m_fat_header;
        bool                            m_is64;
        bool                            m_is_universal;
        const NXArchInfo*               m_archInfo;
        bool                            m_is_need_byteswap;
        
        dylib_infos_t                   m_dylib_infos;
        runpath_additions_infos_t       m_runpath_additions_infos;
        fat_arch_infos_t                m_fat_arch_infos;
    };
    
}

#endif
