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
#include <mach-o/nlist.h>

#include <sys/stat.h>

#include <unistd.h>

#include <vector>
#include <map>
#include <string>

namespace rotg {
    
    typedef struct load_command_info {
        uint32_t                    cmd_type;		/* type of load command */
        const struct load_command*  cmd;
        void*                       cmd_info;
    } load_command_info_t;
    
    typedef std::vector<load_command_info_t> load_command_infos_t;
    
    typedef struct dylib_command_info {
        uint32_t                    cmd_type;
        const struct dylib_command* cmd;
        const char*                 libname;
        size_t                      libnamelen;
    } dylib_command_info_t;
    
    typedef std::vector<dylib_command_info_t*> dylib_command_infos_t;
    
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
        uint64_t    baseOffset;
    } macho_input_t;
    
    typedef struct fat_arch_info {
        struct fat_arch arch;
        const void*     ptr;
        macho_input_t   input;
    } fat_arch_info_t;
    
    typedef std::vector<fat_arch_info_t> fat_arch_infos_t;
    
    typedef std::vector<const struct section_64*> section_64s_t;
    
    typedef struct segment_command_64_info {
        uint32_t                            cmd_type;
        const struct segment_command_64*    cmd;
        section_64s_t                       section_64s;
    } segment_command_64_info_t;
    
    typedef std::vector<segment_command_64_info_t*> segment_command_64_infos_t;
    
    typedef struct bind_opcode {
        uint8_t         opcode;
        uint8_t         immediate;
        const uint8_t*  ptr;
    } bind_opcode_t;
    
    typedef std::vector<bind_opcode_t> bind_opcodes_t;

    enum BindNodeType {NodeTypeBind, NodeTypeWeakBind, NodeTypeLazyBind};
    
    typedef struct bind_action {
        uint64_t        address;
        uint32_t        type;
        const char*     symbolName;
        uint32_t        flags;
        int64_t         addend;
        uint64_t        libOrdinal;
        BindNodeType    nodeType;
        uint64_t        location;
        uint64_t        ptrSize;
    } bind_action_t;
    
    typedef std::vector<bind_action_t> bind_actions_t;
    
    typedef struct binding_info {
        bind_opcodes_t opcodes;
        bind_actions_t actions;
    } binding_info_t;
    
    typedef struct export_node {
        const char* label;
        uint64_t    skip;
    } export_node_t;
    
    typedef std::vector<export_node_t> export_nodes_t;
    
    typedef struct export_opcode {
        uint8_t         terminalSize;
        uint8_t         childCount;
        export_nodes_t  nodes;
        const uint8_t*  ptr;
    } export_opcode_t;
    
    typedef std::vector<export_opcode_t> export_opcodes_t;
    
    typedef struct export_action {
        uint64_t        flags;
        uint64_t        offset;
        std::string     symbolName;
        uint64_t        address;
        const uint8_t*  ptr;
    } export_action_t;
    
    typedef std::vector<export_action_t> export_actions_t;
    
    typedef struct export_info {
        export_opcodes_t    opcodes;
        export_actions_t    actions;
    } export_info_t;
    
    typedef struct dynamic_loader_info {
        binding_info_t  binding_info;
        binding_info_t  weak_binding_info;
        binding_info_t  lazy_binding_info;
        export_info_t   export_info;
    } dynamic_loader_info_t;
    
    typedef struct dylib_info_command_info {
        uint32_t                        cmd_type;
        const struct dyld_info_command* cmd;
        dynamic_loader_info_t           loader_info;
    } dyld_info_command_info_t;
        
    typedef struct thread_command_info {
        uint32_t                        cmd_type;
        const struct thread_command*    cmd;
    } thread_command_info_t;
    
    typedef std::vector<thread_command_info_t*> thread_command_infos_t;
    
    typedef struct nlist_info {
        void *          nlist; // cast to 'struct nlist' or 'struct nlist_64' based on architecture
        const char *    name;
    } nlist_info_t;
    
    typedef std::vector<nlist_info_t> nlist_infos_t;
    
    typedef struct symtab_command_info {
        uint32_t                        cmd_type;
        const struct symtab_command*    cmd;
        nlist_infos_t                   nlist_infos;
    } symtab_command_info_t;
    
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
        
        bool parse_macho(const macho_input_t *input);
        bool parse_file(const char* path);
        
        uint64_t getOffset(const void* address);
        
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
        
        bool is32bit() const {
            return !m_is64bit && !m_is_universal;
        }
        
        bool is64bit() const {
            return !is32bit();
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
        
        const segment_command_64_infos_t& getSegmentCommand64Infos() const {
            return m_segment_command_64_infos;
        }
        
        const dylib_command_infos_t& getDylibCommandInfos() const {
            return m_dylib_command_infos;
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
        
        const dyld_info_command_info_t& getDyldInfoCommandInfo() const {
            return m_dyld_info_command_info;
        }
        
        const symtab_command_info_t& getSymtabCommandInfo() const {
            return m_symtab_command_info;
        }
        
        const char * getStringTable() const {
            return m_string_table;
        }
        
    private:
        MachOFile operator=(MachOFile&);    // declare only, do not allow assign
        MachOFile(MachOFile&);              // declare only, do not allow copy
        
        const void* macho_read(const void *address, size_t length);
        const void* macho_offset(const void *address, size_t offset, size_t length);
        const void* read_sleb128(const void *address, int64_t& result);
        const void* read_uleb128(const void *address, uint64_t& result);
        
        bool parse_universal();
        bool parse_load_commands();
        
        bool parse_LC_SEGMENT_64(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_RPATH(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_DYLIB(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_DYLD_INFO(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_THREAD(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);
        bool parse_LC_SYMTAB(uint32_t cmd_type, uint32_t cmdsize, load_command_info_t* load_cmd_info);

        // dylib related parsing
        bool parse_rebase_node(const struct dyld_info_command* dyld_info_cmd, uint64_t baseAddress);
        bool parse_binding_node(binding_info_t* binding_info, uint64_t location, uint32_t length, BindNodeType nodeType, uint64_t baseAddress);
        bool parse_export_node(export_info_t* exportInfo, const char* prefix, uint64_t location, uint64_t length, uint64_t skipBytes, uint64_t baseAddress);
        
        int                             m_fd;
        bool                            m_isInputOwned;
        macho_input_t                   m_input;
        
        const struct mach_header*       m_header;
        const struct mach_header_64*    m_header64;
        size_t                          m_header_size;
        const struct fat_header*        m_fat_header;
        bool                            m_is64bit;
        bool                            m_is_universal;
        const NXArchInfo*               m_archInfo;
        bool                            m_is_need_byteswap;
        
        load_command_infos_t            m_load_command_infos;
        segment_command_64_infos_t      m_segment_command_64_infos;
        dyld_info_command_info_t        m_dyld_info_command_info;
        thread_command_infos_t          m_thread_command_infos;
        dylib_command_infos_t           m_dylib_command_infos;
        runpath_additions_infos_t       m_runpath_additions_infos;
        fat_arch_infos_t                m_fat_arch_infos;
        symtab_command_info_t           m_symtab_command_info;
        const char *                    m_string_table;
        
        section_64s_t                   m_section_64s;
        
        SegmentInfoMap                  m_segmentInfo;      // segment info lookup table by offset
    };
    
}

#endif
