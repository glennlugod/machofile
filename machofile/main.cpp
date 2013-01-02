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

static void printSegments64(MachOFile& machoFile)
{
    const segment_64_infos_t& segment_64_infos = machoFile.getSegment64Infos();
    
    segment_64_infos_t::const_iterator iter;
    for (iter=segment_64_infos.begin(); iter!=segment_64_infos.end(); iter++) {
        const segment_64_info_t& info = *iter;
        
        printf("LC_SEGMENT_64 (%s)\n", info.cmd->segname);
    }
}

static void printHeader(MachOFile& machoFile)
{
    if (machoFile.is32()) {
        printf("Type: Mach-O 32-bit\n");
    }
    if (machoFile.is64()) {
        printf("Type: Mach-O 64-bit\n");
    }
    if (machoFile.isUniversal()) {
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
    switch (header->magic) {            
        case MH_CIGAM:
            printf("\tValue : MH_CIGAM\n");
            break;
            
        case MH_MAGIC:
            printf("\tValue : MH_MAGIC\n");
            break;
            
        case MH_CIGAM_64:
            printf("\tValue : MH_CIGAM_64\n");
            break;

        case MH_MAGIC_64:
            printf("\tValue : MH_MAGIC_64\n");
            break;
            
        default:
            printf("\tValue : Unknown\n");
            break;
    }
    
    printf("CPU Type\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->cputype));
    printf("\tData  : 0x%X\n", header->cputype);
    switch (header->cputype) {
        case CPU_TYPE_ANY:
            printf("\tValue : CPU_TYPE_ANY\n");
            break;
        
        case CPU_TYPE_VAX:
            printf("\tValue : CPU_TYPE_VAX\n");
            break;

        case CPU_TYPE_MC680x0:
            printf("\tValue : CPU_TYPE_MC680x0\n");
            break;
            
        case CPU_TYPE_X86:
            printf("\tValue : CPU_TYPE_X86\n");
            break;
            
        case CPU_TYPE_X86_64:
            printf("\tValue : CPU_TYPE_X86_64\n");
            break;
            
        case CPU_TYPE_MC98000:
            printf("\tValue : CPU_TYPE_MC98000\n");
            break;
            
        case CPU_TYPE_HPPA:
            printf("\tValue : CPU_TYPE_HPPA\n");
            break;
            
        case CPU_TYPE_ARM:
            printf("\tValue : CPU_TYPE_ARM\n");
            break;
            
        case CPU_TYPE_MC88000:
            printf("\tValue : CPU_TYPE_MC88000\n");
            break;
            
        case CPU_TYPE_SPARC:
            printf("\tValue : CPU_TYPE_SPARC\n");
            break;
            
        case CPU_TYPE_I860:
            printf("\tValue : CPU_TYPE_I860\n");
            break;
            
        case CPU_TYPE_POWERPC:
            printf("\tValue : CPU_TYPE_POWERPC\n");
            break;
            
        case CPU_TYPE_POWERPC64:
            printf("\tValue : CPU_TYPE_POWERPC64\n");
            break;

        default:
            printf("\tValue : Unknown\n");
            break;
    }

    printf("CPU SubType\n");
    printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header->cpusubtype));
    printf("\tData  : 0x%X\n", header->cpusubtype);

    printf("\n");
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

