//
//  main.cpp
//  machofile
//
//  Created by Glenn on 12/31/12.
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
    
    printf("** Header **\n");
    
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

static void printSegments64(MachOFile& machoFile)
{
    const segment_64_infos_t& segment_64_infos = machoFile.getSegment64Infos();
    
    segment_64_infos_t::const_iterator iter;
    for (iter=segment_64_infos.begin(); iter!=segment_64_infos.end(); iter++) {
        const segment_64_info_t& info = *iter;
        
        printf("LC_SEGMENT_64 (%s)\n", info.cmd->segname);
    }
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
        printSegments64(machoFile);
        printDylibs(machoFile);
    }
    else
    {
        printf("error parsing %s", argv[1]);
    }

    return 0;
}

