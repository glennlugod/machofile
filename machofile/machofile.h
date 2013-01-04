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
    
    typedef std::vector<const struct section_64*> section_64s_t;
    
    typedef struct segment_64_info {
        uint32_t                            cmd_type;
        const struct segment_command_64*    cmd;
        section_64s_t                       section_64s;
    } segment_64_info_t;
    
    typedef std::vector<segment_64_info_t*> segment_64_infos_t;
    
    typedef struct bind_opcode {
        uint8_t         opcode;
        uint8_t         immediate;
        const uint8_t*  ptr;
    } bind_opcode_t;
    
    enum BindNodeType {NodeTypeBind, NodeTypeWeakBind, NodeTypeLazyBind};
    
    typedef struct bind_action {
        uint64_t        address;
        const char*     symbolName;
        uint32_t        symbolFlags;
        int64_t         addend;
        uint64_t        libOrdinal;
        BindNodeType    nodeType;
        uint64_t        location;
    } bind_action_t;
    
    typedef struct binding_info {
        std::vector<bind_opcode_t> opcodes;
    } binding_info_t;
    
    typedef struct lazy_binding_info {
        std::vector<bind_opcode_t> opcodes;
    } lazy_binding_info_t;
    
    typedef struct export_info {
        std::vector<bind_opcode_t> opcodes;
    } export_info_t;
    
    typedef struct dynamic_loader_info {
        binding_info_t      binding_info;
        lazy_binding_info_t lazy_binding_info;
        export_info_t       export_info;
    } dynamic_loader_info_t;
    
    typedef struct dylib_info_command_info {
        uint32_t                        cmd_type;
        const struct dyld_info_command* cmd;
        dynamic_loader_info_t           dl_info;
    } dyld_info_command_info_t;
    
    typedef std::vector<dyld_info_command_info_t*> dyld_info_command_infos_t;
    
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
        
        const section_64s_t& getSection64s() const {
            return m_section_64s;
        }
        
    private:
        const void* macho_read(macho_input_t* input, const void *address, size_t length);
        const void* macho_offset(macho_input_t *input, const void *address, size_t offset, size_t length);
        const void* read_sleb128(const void *address, int64_t& result);
        const void* read_uleb128(const void *address, uint64_t& result);
        
        char* macho_format_dylib_version (uint32_t version);
        
        bool parse_universal(macho_input_t *input);
        bool parse_load_commands(macho_input_t *input);
        
        bool parse_LC_SEGMENT_64(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_RPATH(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_DYLIBS(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_DYLD_INFOS(macho_input_t *input, uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        
        // dylib related parsing
        bool parse_rebase_node(macho_input_t *input, const struct dyld_info_command* dyld_info_cmd);
        bool parse_binding_node(macho_input_t *input, dyld_info_command_info_t* dyld_info_cmd_info, BindNodeType nodeType);
        
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
        dyld_info_command_infos_t       m_dyld_info_command_infos;
        
        dylib_infos_t                   m_dylib_infos;
        runpath_additions_infos_t       m_runpath_additions_infos;
        fat_arch_infos_t                m_fat_arch_infos;
        
        section_64s_t                   m_section_64s;
        
        SegmentInfoMap                  m_segmentInfo;      // segment info lookup table by offset
    };
    
}

#endif
