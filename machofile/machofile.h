//
//  machofile.h
//  
//
//  Created by Glenn Lugod on 12/29/12.
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

#include <string>
#include <vector>
#include <map>

namespace rotg {
    
    typedef struct load_command_info {
        uint32_t                    cmd_type;		/* type of load command */
        const struct load_command*  cmd;
        void*                       cmd_info;
    } load_command_info_t;
    
    typedef std::vector<load_command_info_t> load_command_infos_t;
    
    typedef struct dylib_info {
        uint32_t                    cmd_type;
        const struct dylib_command* cmd;
        const char*                 name;
        size_t                      namelen;
        std::string                 current_version;
        std::string                 compat_version;
    } dylib_info_t;
    
    typedef std::vector<dylib_info_t> dylib_infos_t;
    
    typedef struct runpath_additions_info {
        uint32_t                    cmd_type;
        const struct rpath_command* cmd;
        const char*                 path;
        size_t                      pathlen;
    } runpath_additions_info_t;
    
    typedef std::vector<runpath_additions_info_t> runpath_additions_infos_t;
    
    typedef struct macho_input {
        const void* data;
        size_t      length;
    } macho_input_t;
    
    typedef struct fat_arch_info {
        const struct fat_arch*  arch;
        macho_input_t           input;
    } fat_arch_info_t;
    
    typedef std::vector<fat_arch_info_t> fat_arch_infos_t;
    
    typedef struct segment_64_info {
        uint32_t                            cmd_type;
        const struct segment_command_64*    cmd;
    } segment_64_info_t;
    
    typedef std::vector<segment_64_info_t*> segment_64_infos_t;
    
    ////////////////////////////////////////////////////////////////////////////////
    
    /*
    typedef std::vector<struct load_command const *>          CommandVector;
    typedef std::vector<struct segment_command const *>       SegmentVector;
    typedef std::vector<struct segment_command_64 const *>    Segment64Vector;
    typedef std::vector<struct section const *>               SectionVector;
    typedef std::vector<struct section_64 const *>            Section64Vector;
    typedef std::vector<struct nlist const *>                 NListVector;
    typedef std::vector<struct nlist_64 const *>              NList64Vector;
    typedef std::vector<struct dylib const *>                 DylibVector;
    typedef std::vector<struct dylib_module const *>          ModuleVector;
    typedef std::vector<struct dylib_module_64 const *>       Module64Vector;
    
    typedef std::map<uint32_t,std::pair<uint32_t,uint64_t> >    RelocMap;           // fileOffset --> <length,value>
    */
    
    typedef std::map<uint64_t, std::pair<uint64_t, uint64_t> >  SegmentInfoMap;     // fileOffset --> <address,size>
    
    //typedef std::map<uint64_t,std::pair<uint32_t,NSDictionary *> >  SectionInfoMap;     // address    --> <fileOffset,sectionUserInfo>
    //typedef std::map<uint64_t,uint64_t>                             ExceptionFrameMap;  // LSDA_addr  --> PCBegin_addr

    ////////////////////////////////////////////////////////////////////////////////
    
    class MachOFile
    {
    public:
        MachOFile();
        ~MachOFile();
        
        bool parse_macho(macho_input_t *input);
        bool parse_file(const char* path);
        
        uint64_t getOffset(void* address); 
        
        uint32_t read32(uint32_t input) const {
            if (isNeedByteSwap()) {
                return OSSwapInt32(input);
            }
            
            return input;
        }
        
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
        
        const segment_64_infos_t& getSegment64Infos() const {
            return m_segment_64_infos;
        }
        
        const dylib_infos_t& getDylibInfos() const {
            return m_dylib_infos;
        }
        
        const fat_arch_infos_t& getFatArchInfos() const {
            return m_fat_arch_infos;
        }
        
        const load_command_infos_t& getLoadCommandInfos() const {
            return m_load_command_infos;
        }
        
    private:
        const void* macho_read(macho_input_t* input, const void *address, size_t length);
        const void* macho_offset(macho_input_t *input, const void *address, size_t offset, size_t length);
        char* macho_format_dylib_version (uint32_t version);
        
        bool parse_universal(macho_input_t *input);
        bool parse_load_commands(macho_input_t *input);
        
        bool parse_LC_SEGMENT_64(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_RPATH(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_DYLIBS(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_DYLD_INFOS(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        
        // dylib related parsing
        bool parse_binding_node(macho_input_t *input, const struct dyld_info_command* dyld_info_cmd);
        
        int                             m_fd;
        struct stat                     m_stbuf;
        void*                           m_data;
        const void*                     m_baseAddress;
        
        const struct mach_header*       m_header;
        const struct mach_header_64*    m_header64;
        size_t                          m_header_size;
        const struct fat_header*        m_fat_header;
        bool                            m_is64;
        bool                            m_is_universal;
        const NXArchInfo*               m_archInfo;
        bool                            m_is_need_byteswap;
        
        load_command_infos_t            m_load_command_infos;
        segment_64_infos_t              m_segment_64_infos;
        dylib_infos_t                   m_dylib_infos;
        runpath_additions_infos_t       m_runpath_additions_infos;
        fat_arch_infos_t                m_fat_arch_infos;
        
        SegmentInfoMap                  m_segmentInfo;      // segment info lookup table by offset
    };
    
}

#endif
