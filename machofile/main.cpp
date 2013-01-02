//
//  main.cpp
//  machofile
//
//  Created by Glenn Lugod on 12/31/12.
//  Copyright (c) 2012 Glenn. All rights reserved.
//

#include <iostream>

#include "machofile.h"

using namespace rotg;

static void printHeader(MachOFile& machoFile)
{
    if (machoFile.is32()) {
        printf("Type: Mach-O 32-bit\n");
    } else if (machoFile.is64()) {
        printf("Type: Mach-O 64-bit\n");
    } else if (machoFile.isUniversal()) {
        printf("Type: Universal\n");
    }
    
    const NXArchInfo* archInfo = machoFile.getArchInfo();
    if (archInfo) {
        printf("Architecture: %s\n\n", archInfo->name);
    }
    
    printf("***** Header *****\n");
    
    const struct mach_header* header = machoFile.getHeader();
    
    printf("Magic Number\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->magic));
    printf("\tData  : 0x%X\n", header->magic);
    printf("\tValue : ");
    switch (header->magic) {            
        case MH_CIGAM:
            printf("MH_CIGAM");
            break;
            
        case MH_MAGIC:
            printf("MH_MAGIC");
            break;
            
        case MH_CIGAM_64:
            printf("MH_CIGAM_64");
            break;
            
        case MH_MAGIC_64:
            printf("MH_MAGIC_64");
            break;
            
        default:
            printf("Unknown");
            break;
    }
    printf("\n");
    
    printf("CPU Type\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->cputype));
    printf("\tData  : 0x%X\n", header->cputype);
    printf("\tValue : ");
    switch (header->cputype) {
        case CPU_TYPE_ANY:
            printf("CPU_TYPE_ANY");
            break;
            
        case CPU_TYPE_VAX:
            printf("CPU_TYPE_VAX");
            break;
            
        case CPU_TYPE_MC680x0:
            printf("CPU_TYPE_MC680x0");
            break;
            
        case CPU_TYPE_X86:
            printf("CPU_TYPE_X86");
            break;
            
        case CPU_TYPE_X86_64:
            printf("CPU_TYPE_X86_64");
            break;
            
        case CPU_TYPE_MC98000:
            printf("CPU_TYPE_MC98000");
            break;
            
        case CPU_TYPE_HPPA:
            printf("CPU_TYPE_HPPA");
            break;
            
        case CPU_TYPE_ARM:
            printf("CPU_TYPE_ARM");
            break;
            
        case CPU_TYPE_MC88000:
            printf("CPU_TYPE_MC88000");
            break;
            
        case CPU_TYPE_SPARC:
            printf("CPU_TYPE_SPARC");
            break;
            
        case CPU_TYPE_I860:
            printf("CPU_TYPE_I860");
            break;
            
        case CPU_TYPE_POWERPC:
            printf("CPU_TYPE_POWERPC");
            break;
            
        case CPU_TYPE_POWERPC64:
            printf("CPU_TYPE_POWERPC64");
            break;
            
        default:
            printf("Unknown");
            break;
    }
    printf("\n");
    
    printf("CPU SubType\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->cpusubtype));
    printf("\tData  : 0x%X\n", header->cpusubtype);
    
    printf("File Type\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->filetype));
    printf("\tData  : 0x%X\n", header->filetype);
    printf("\tValue : ");
    switch (header->filetype) {
        case MH_OBJECT:
            printf("MH_OBJECT");
            break;
            
        case MH_EXECUTE:
            printf("MH_EXECUTE");
            break;
            
        case MH_FVMLIB:
            printf("MH_FVMLIB");
            break;
            
        case MH_CORE:
            printf("MH_CORE");
            break;
            
        case MH_PRELOAD:
            printf("MH_PRELOAD");
            break;
            
        case MH_DYLIB:
            printf("MH_DYLIB");
            break;
            
        case MH_DYLINKER:
            printf("MH_DYLINKER");
            break;
            
        case MH_BUNDLE:
            printf("MH_BUNDLE");
            break;
            
        case MH_DYLIB_STUB:
            printf("MH_DYLIB_STUB");
            break;
            
        case MH_DSYM:
            printf("MH_DSYM");
            break;
            
        case MH_KEXT_BUNDLE:
            printf("MH_KEXT_BUNDLE");
            break;
            
        default:
            printf("Unknown");
            break;
    }
    printf("\n");
    
    printf("Number of Load Commands\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->ncmds));
    printf("\tData  : 0x%X\n", header->ncmds);
    printf("\tValue : %d\n", header->ncmds);
    
    printf("Size of Load Commands\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->sizeofcmds));
    printf("\tData  : 0x%X\n", header->sizeofcmds);
    printf("\tValue : %d\n", header->sizeofcmds);
    
    printf("Flags\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->flags));
    printf("\tData  : 0x%X\n", header->flags);
    
    if (machoFile.is64()) {
        const struct mach_header_64* header64 = machoFile.getHeader64();
        
        printf("Reserved\n");
        printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header64->reserved));
        printf("\tData  : 0x%X\n", header64->reserved);
    }
    
    printf("\n");
}

static void printSegment64(MachOFile& machofile, const segment_64_info_t* info)
{
    printf("LC_SEGMENT_64 (%s)\n", info->cmd->segname);
    
    printf("\tCommand\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmd));
    printf("\t\tData  : 0x%X\n", info->cmd->cmd);
    printf("\t\tValue : LC_SEGMENT_64\n");
    
    printf("\tCommand Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmdsize));
    printf("\t\tData  : 0x%X\n", info->cmd->cmdsize);
    printf("\t\tValue : %d\n", info->cmd->cmdsize);
    
    printf("\tSegment Name\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->segname));
    printf("\t\tValue : %s\n", info->cmd->segname);
    
    printf("\tVM Address\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->vmaddr));
    printf("\t\tData  : 0x%llX\n", info->cmd->vmaddr);
    printf("\t\tValue : %lld\n", info->cmd->vmaddr);
    
    printf("\tVM Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->vmsize));
    printf("\t\tData  : 0x%llX\n", info->cmd->vmsize);
    printf("\t\tValue : %lld\n", info->cmd->vmsize);
    
    printf("\tFile Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->fileoff));
    printf("\t\tData  : 0x%llX\n", info->cmd->fileoff);
    printf("\t\tValue : %lld\n", info->cmd->fileoff);
    
    printf("\tFile Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->filesize));
    printf("\t\tData  : 0x%llX\n", info->cmd->filesize);
    printf("\t\tValue : %lld\n", info->cmd->filesize);
    
    printf("\tMaximum VM Protection\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->maxprot));
    printf("\t\tData  : 0x%X\n", info->cmd->maxprot);
    printf("\t\tValue : 0x%X\n", info->cmd->maxprot);
    
    printf("\tInitial VM Protection\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->initprot));
    printf("\t\tData  : 0x%X\n", info->cmd->initprot);
    printf("\t\tValue : 0x%X\n", info->cmd->initprot);
    
    printf("\tNumber of Sections\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->nsects));
    printf("\t\tData  : 0x%X\n", info->cmd->nsects);
    printf("\t\tValue : %d\n", info->cmd->nsects);
    
    printf("\tFlags\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->flags));
    printf("\t\tData  : 0x%X\n", info->cmd->flags);
    printf("\t\tValue : 0x%X\n", info->cmd->flags);
    
    printf("\n");
}

static void printLoadCommands(MachOFile& machofile)
{
    printf("***** Load Commands *****\n");
    const load_command_infos_t& load_cmd_infos = machofile.getLoadCommandInfos();
    
    load_command_infos_t::const_iterator iter;
    for (iter = load_cmd_infos.begin(); iter != load_cmd_infos.end(); iter++) {
        const load_command_info_t& info = *iter;
        
        switch (info.cmd_type) {
            case LC_SEGMENT:
                printf("LC_SEGMENT (TODO: Details)\n");
                break;
                
            case LC_SEGMENT_64:
                printSegment64(machofile, (const segment_64_info_t*)info.cmd_info);
                break;
                
            case LC_SYMTAB:
                printf("LC_SYMTAB (TODO: Details)\n");
                break;
                
            case LC_DYSYMTAB:
                printf("LC_DYSYMTAB (TODO: Details)\n");
                break;
                
            case LC_TWOLEVEL_HINTS:
                printf("LC_TWOLEVEL_HINTS (TODO: Details)\n");
                break;
                
            case LC_ID_DYLINKER:
                printf("LC_ID_DYLINKER (TODO: Details)\n");
                break;
                
            case LC_LOAD_DYLINKER:
                printf("LC_LOAD_DYLINKER (TODO: Details)\n");
                break;
                
            case LC_PREBIND_CKSUM:
                printf("LC_PREBIND_CKSUM (TODO: Details)\n");
                break;
                
            case LC_UUID:
                printf("LC_UUID (TODO: Details)\n");
                break;
                
            case LC_THREAD:
                printf("LC_THREAD (TODO: Details)\n");
                break;
                
            case LC_UNIXTHREAD:
                printf("LC_UNIXTHREAD (TODO: Details)\n");
                break;
                
            case LC_ID_DYLIB:
                printf("LC_ID_DYLIB (TODO: Details)\n");
                break;
                
            case LC_LOAD_DYLIB:
                printf("LC_LOAD_DYLIB (TODO: Details)\n");
                break;
                
            case LC_LOAD_WEAK_DYLIB:
                printf("LC_LOAD_WEAK_DYLIB (TODO: Details)\n");
                break;

            case LC_REEXPORT_DYLIB:
                printf("LC_REEXPORT_DYLIB (TODO: Details)\n");
                break;
                
            case LC_LAZY_LOAD_DYLIB:
                printf("LC_LAZY_LOAD_DYLIB (TODO: Details)\n");
                break;
                
#ifdef __MAC_10_7
            case LC_LOAD_UPWARD_DYLIB:
                printf("LC_LOAD_UPWARD_DYLIB (TODO: Details)\n");
                break;
#endif
                
            case LC_CODE_SIGNATURE:
                printf("LC_CODE_SIGNATURE (TODO: Details)\n");
                break;
                
            case LC_SEGMENT_SPLIT_INFO:
                printf("LC_SEGMENT_SPLIT_INFO (TODO: Details)\n");
                break;

#ifdef __MAC_10_7
            case LC_FUNCTION_STARTS:
                printf("LC_FUNCTION_STARTS (TODO: Details)\n");
                break;
#endif
                
            case LC_ENCRYPTION_INFO:
                printf("LC_ENCRYPTION_INFO (TODO: Details)\n");
                break;
                
            case LC_RPATH:
                printf("LC_RPATH (TODO: Details)\n");
                break;
                
            case LC_ROUTINES:
                printf("LC_ROUTINES (TODO: Details)\n");
                break;
                
            case LC_ROUTINES_64:
                printf("LC_ROUTINES_64 (TODO: Details)\n");
                break;
                
            case LC_SUB_FRAMEWORK:
                printf("LC_SUB_FRAMEWORK (TODO: Details)\n");
                break;
                
            case LC_SUB_UMBRELLA:
                printf("LC_SUB_UMBRELLA (TODO: Details)\n");
                break;
                
            case LC_SUB_CLIENT:
                printf("LC_SUB_CLIENT (TODO: Details)\n");
                break;
                
            case LC_SUB_LIBRARY:
                printf("LC_SUB_LIBRARY (TODO: Details)\n");
                break;
                
            case LC_DYLD_INFO:
                printf("LC_DYLD_INFO (TODO: Details)\n");
                break;
                
            case LC_DYLD_INFO_ONLY:
                printf("LC_DYLD_INFO_ONLY (TODO: Details)\n");
                break;
                
#ifdef __MAC_10_7
            case LC_VERSION_MIN_MACOSX:
                printf("LC_VERSION_MIN_MACOSX (TODO: Details)\n");
                break;
                
            case LC_VERSION_MIN_IPHONEOS:
                printf("LC_VERSION_MIN_IPHONEOS (TODO: Details)\n");
                break;
#endif
            
            default:
                printf("Unsupported/Unknown command\n");
                printf("\tType        : 0x%X\n", info.cmd_type);
                printf("\tCommand Size: %d\n", info.cmd->cmdsize);
                break;
        }
    }
    
    printf("\n");
}

static void printDylibs(MachOFile& machoFile)
{
    const dylib_infos_t& dylib_infos = machoFile.getDylibInfos();
    
    dylib_infos_t::const_iterator iter;
    for (iter=dylib_infos.begin(); iter!=dylib_infos.end(); iter++) {
        const struct dylib_info& info = *iter;
        
        switch (info.cmd_type) {
            case LC_ID_DYLIB:
                printf("[dylib] ");
                break;
            case LC_LOAD_WEAK_DYLIB:
                printf("[weak] ");
                break;
            case LC_LOAD_DYLIB:
                printf("[load] ");
                break;
            case LC_REEXPORT_DYLIB:
                printf("[reexport] ");
                break;
            default:
                printf("[%d] ", info.cmd_type);
                break;
        }
        
        /* This is a dyld library identifier */
        printf("install_name=%s (compatibility_version=%s, version=%s)\n", info.name, info.compat_version.c_str(), info.current_version.c_str());
    }
}

int main(int argc, const char * argv[])
{
    MachOFile machoFile;
    
    if (machoFile.parse_file(argv[1])) {
        printHeader(machoFile);
        printLoadCommands(machoFile);
        printDylibs(machoFile);
    }
    else
    {
        printf("error parsing %s", argv[1]);
    }

    return 0;
}

