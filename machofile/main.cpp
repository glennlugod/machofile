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

int main(int argc, const char * argv[])
{    
    MachOFile machoFile;
    
    if (machoFile.parse_file(argv[1])) {
        
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
            printf("Architecture: %s\n", archInfo->name);
        }
        
        printSegments64(machoFile);
        printDylibs(machoFile);
    }
    else
    {
        printf("error parsing %s", argv[1]);
    }

    return 0;
}

