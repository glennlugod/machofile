//
//  main.cpp
//  machofile
//
//  Created by Glenn Lugod on 12/31/12.
//  Copyright (c) 2012 Glenn. All rights reserved.
//

#include <iostream>

#include <math.h>

#include "machofile.h"

using namespace rotg;

static const char* getCPUTypeString(cpu_type_t cputype)
{
    const char* strcputype = "Unknown";
    
    switch (cputype) {
        case CPU_TYPE_ANY:
            strcputype = "CPU_TYPE_ANY";
            break;
            
        case CPU_TYPE_VAX:
            strcputype = "CPU_TYPE_VAX";
            break;
            
        case CPU_TYPE_MC680x0:
            strcputype = "CPU_TYPE_MC680x0";
            break;
            
        case CPU_TYPE_X86:
            strcputype = "CPU_TYPE_X86";
            break;
            
        case CPU_TYPE_X86_64:
            strcputype = "CPU_TYPE_X86_64";
            break;
            
        case CPU_TYPE_MC98000:
            strcputype = "CPU_TYPE_MC98000";
            break;
            
        case CPU_TYPE_HPPA:
            strcputype = "CPU_TYPE_HPPA";
            break;
            
        case CPU_TYPE_ARM:
            strcputype = "CPU_TYPE_ARM";
            break;
            
        case CPU_TYPE_MC88000:
            strcputype = "CPU_TYPE_MC88000";
            break;
            
        case CPU_TYPE_SPARC:
            strcputype = "CPU_TYPE_SPARC";
            break;
            
        case CPU_TYPE_I860:
            strcputype = "CPU_TYPE_I860";
            break;
            
        case CPU_TYPE_POWERPC:
            strcputype = "CPU_TYPE_POWERPC";
            break;
            
        case CPU_TYPE_POWERPC64:
            strcputype = "CPU_TYPE_POWERPC64";
            break;
    }
    
    return strcputype;
}

static void printHeader(MachOFile& machoFile)
{
    if (machoFile.is32bit()) {
        printf("Type: Mach-O 32-bit\n");
    } else if (machoFile.is64bit()) {
        printf("Type: Mach-O 64-bit\n");
    } else if (machoFile.isUniversal()) {
        printf("Type: Universal\n");
    }
    
    const NXArchInfo* archInfo = machoFile.getArchInfo();
    if (archInfo) {
        printf("Architecture: %s\n\n", archInfo->name);
    }
    
    printf("\n***** Header *****\n");
    
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
    printf("\tValue : %s", getCPUTypeString(header->cputype));
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
    
    if (machoFile.is64bit()) {
        const struct mach_header_64* header64 = machoFile.getHeader64();
        
        printf("Reserved\n");
        printf("\tOffset: 0x%08llx\n", machoFile.getOffset((void*)&header64->reserved));
        printf("\tData  : 0x%X\n", header64->reserved);
    }
    
    printf("\n");
}

static void printSegmentCommand64(MachOFile& machofile, const segment_command_64_info_t* info)
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
    
    uint32_t nsect;
    for (nsect=0; nsect < info->cmd->nsects; nsect++) {
        const struct section_64* section = info->section_64s[nsect];
        printf("\n\tSection64 Header (%s)\n", section->sectname);
        
        printf("\t\tSection Name\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->sectname));
        printf("\t\t\tData  : 0x%X\n", info->cmd->flags);
        printf("\t\t\tValue : %s\n", section->sectname);
        
        printf("\t\tSegment Name\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->segname));
        printf("\t\t\tValue : %s\n", section->segname);
        
        printf("\t\tAddress\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->addr));
        printf("\t\t\tData  : 0x%llX\n", section->addr);
        printf("\t\t\tValue : %llu\n", section->addr);
        
        printf("\t\tSize\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->size));
        printf("\t\t\tData  : 0x%llX\n", section->size);
        printf("\t\t\tValue : %llu\n", section->size);
        
        printf("\t\tOffset\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->offset));
        printf("\t\t\tData  : 0x%X\n", section->offset);
        printf("\t\t\tValue : %u\n", section->offset);
        
        printf("\t\tAlignment\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->align));
        printf("\t\t\tData  : 0x%X\n", section->align);
        printf("\t\t\tValue : %u\n", section->align);
        
        printf("\t\tRelocations Offset\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->reloff));
        printf("\t\t\tData  : 0x%X\n", section->reloff);
        printf("\t\t\tValue : %u\n", section->reloff);
        
        printf("\t\tNumber of Relocations\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->nreloc));
        printf("\t\t\tData  : 0x%X\n", section->nreloc);
        printf("\t\t\tValue : %u\n", section->nreloc);
        
        printf("\t\tFlags\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->flags));
        printf("\t\t\tData  : 0x%X\n", section->flags);
        printf("\t\t\tValue : %u\n", section->flags);
        
        printf("\t\tReserved1\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->reserved1));
        printf("\t\t\tData  : 0x%X\n", section->reserved1);
        printf("\t\t\tValue : %u\n", section->reserved1);
        
        printf("\t\tReserved2\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->reserved2));
        printf("\t\t\tData  : 0x%X\n", section->reserved2);
        printf("\t\t\tValue : %u\n", section->reserved2);
        
        printf("\t\tReserved3\n");
        printf("\t\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&section->reserved3));
        printf("\t\t\tData  : 0x%X\n", section->reserved3);
        printf("\t\t\tValue : %u\n", section->reserved3);
    }
    
    printf("\n");
}

static void printDyldInfo(MachOFile& machofile, const dyld_info_command_info_t* info)
{
    const char* cmd_name = "";
    
    switch (info->cmd_type) {
        case LC_DYLD_INFO:
            cmd_name = "LC_DYLD_INFO";
            break;
        case LC_DYLD_INFO_ONLY:
            cmd_name = "LC_DYLD_INFO_ONLY";
            break;
    }

    printf("%s\n", cmd_name);

    printf("\tCommand\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmd));
    printf("\t\tData  : 0x%X\n", info->cmd->cmd);
    printf("\t\tValue : %s\n", cmd_name);
    
    printf("\tCommand Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmdsize));
    printf("\t\tData  : 0x%X\n", info->cmd->cmdsize);
    printf("\t\tValue : %d\n", info->cmd->cmdsize);
    
    printf("\tRebase Info Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->rebase_off));
    printf("\t\tData  : 0x%X\n", info->cmd->rebase_off);
    printf("\t\tValue : %d\n", info->cmd->rebase_off);
    
    printf("\tRebase Info Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->rebase_size));
    printf("\t\tData  : 0x%X\n", info->cmd->rebase_size);
    printf("\t\tValue : %d\n", info->cmd->rebase_size);
    
    printf("\tBinding Info Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->bind_off));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tBinding Info Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->bind_off));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tWeak Binding Info Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->weak_bind_off));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tWeak Binding Info Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->weak_bind_size));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tLazy Binding Info Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->lazy_bind_off));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tLazy Binding Info Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->lazy_bind_size));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tExport Info Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->export_off));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\tExport Info Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->export_size));
    printf("\t\tData  : 0x%X\n", info->cmd->bind_off);
    printf("\t\tValue : %d\n", info->cmd->bind_off);
    
    printf("\n");
}

static void printThreadCommand(MachOFile& machofile, const thread_command_info_t* info)
{
    const char* cmd_name = "";
    switch (info->cmd_type) {
        case LC_THREAD:
            cmd_name = "LC_THREAD";
            break;
            
        case LC_UNIXTHREAD:
            cmd_name = "LC_UNIXTHREAD";
            break;
    }
    
    printf("%s\n", cmd_name);
    
    printf("\tCommand\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmd));
    printf("\t\tData  : 0x%X\n", info->cmd->cmd);
    printf("\t\tValue : %s\n", cmd_name);
    
    printf("\tCommand Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmdsize));
    printf("\t\tData  : 0x%X\n", info->cmd->cmdsize);
    printf("\t\tValue : %d\n", info->cmd->cmdsize);
    
    const struct mach_header* header = machofile.getHeader();
    if (header->cputype == CPU_TYPE_X86 || header->cputype == CPU_TYPE_X86_64) {
        const x86_thread_state* state = (const x86_thread_state*)((uint8_t*)info->cmd + sizeof(struct thread_command));
        
        printf("\tFlavor\n");
        printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->tsh.flavor));
        printf("\t\tData  : 0x%X\n", state->tsh.flavor);
        printf("\t\tValue : ");
        switch (state->tsh.flavor) {
            case x86_THREAD_STATE32:
                printf("x86_THREAD_STATE32");
                break;
                
            case x86_FLOAT_STATE32:
                printf("x86_FLOAT_STATE32");
                break;
                
            case x86_EXCEPTION_STATE32:
                printf("x86_EXCEPTION_STATE32");
                break;
                
            case x86_THREAD_STATE64:
                printf("x86_THREAD_STATE64");
                break;
                
            case x86_FLOAT_STATE64:
                printf("x86_FLOAT_STATE64");
                break;
                
            case x86_EXCEPTION_STATE64:
                printf("x86_EXCEPTION_STATE64");
                break;
                
            case x86_THREAD_STATE:
                printf("x86_THREAD_STATE");
                break;
                
            case x86_FLOAT_STATE:
                printf("x86_FLOAT_STATE");
                break;
                
            case x86_EXCEPTION_STATE:
                printf("x86_EXCEPTION_STATE");
                break;
                
            case x86_DEBUG_STATE32:
                printf("x86_DEBUG_STATE32");
                break;
                
            case x86_DEBUG_STATE64:
                printf("x86_DEBUG_STATE64");
                break;
                
            case x86_DEBUG_STATE:
                printf("x86_DEBUG_STATE");
                break;
                
            case THREAD_STATE_NONE:
                printf("THREAD_STATE_NONE");
                break;
        }
        printf("\n");

        printf("\tCount\n");
        printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->tsh.count));
        printf("\t\tData  : 0x%X\n", state->tsh.count);
        printf("\t\tValue : %d\n", state->tsh.count);

        switch (state->tsh.flavor) {
            case x86_THREAD_STATE32:
                printf("TODO: Display Info - x86_THREAD_STATE32");
                break;
                
            case x86_FLOAT_STATE32:
                printf("TODO: Display Info - x86_FLOAT_STATE32");
                break;
                
            case x86_EXCEPTION_STATE32:
                printf("TODO: Display Info - x86_EXCEPTION_STATE32");
                break;
                
            case x86_THREAD_STATE64:
                printf("\trax\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rax));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rax);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rax);
                
                printf("\trbx\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rbx));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rbx);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rbx);
                
                printf("\trcx\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rcx));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rcx);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rcx);
                
                printf("\trdx\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rdx));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rdx);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rdx);
                
                printf("\trdi\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rdi));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rdi);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rdi);
                
                printf("\trsi\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rsi));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rsi);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rsi);
                
                printf("\trbp\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rbp));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rbp);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rbp);
                
                printf("\trsp\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rsp));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rsp);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rsp);
                
                printf("\tr8\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r8));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r8);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r8);
                
                printf("\tr9\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r9));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r9);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r9);
                
                printf("\tr10\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r10));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r10);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r10);
                
                printf("\tr11\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r11));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r11);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r11);
                
                printf("\tr12\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r12));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r12);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r12);
                
                printf("\tr13\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r13));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r13);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r13);
                
                printf("\tr14\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r14));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r14);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r14);
                
                printf("\tr15\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__r15));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__r15);
                printf("\t\tValue : %lld\n", state->uts.ts64.__r15);
                
                printf("\trip\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rip));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rip);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rip);
                
                printf("\trflags\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__rflags));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__rflags);
                printf("\t\tValue : %lld\n", state->uts.ts64.__rflags);
                
                printf("\tcs\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__cs));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__cs);
                printf("\t\tValue : %lld\n", state->uts.ts64.__cs);
                
                printf("\tfs\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__fs));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__fs);
                printf("\t\tValue : %lld\n", state->uts.ts64.__fs);
                
                printf("\tgs\n");
                printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&state->uts.ts64.__gs));
                printf("\t\tData  : 0x%llX\n", state->uts.ts64.__gs);
                printf("\t\tValue : %lld\n", state->uts.ts64.__gs);
                
                break;
                
            case x86_FLOAT_STATE64:
                printf("TODO: Display Info - x86_FLOAT_STATE64");
                break;
                
            case x86_EXCEPTION_STATE64:
                printf("TODO: Display Info - x86_EXCEPTION_STATE64");
                break;
                
            case x86_THREAD_STATE:
                printf("TODO: Display Info - x86_THREAD_STATE");
                break;
                
            case x86_FLOAT_STATE:
                printf("TODO: Display Info - x86_FLOAT_STATE");
                break;
                
            case x86_EXCEPTION_STATE:
                printf("TODO: Display Info - x86_EXCEPTION_STATE");
                break;
                
            case x86_DEBUG_STATE32:
                printf("TODO: Display Info - x86_DEBUG_STATE32");
                break;
                
            case x86_DEBUG_STATE64:
                printf("TODO: Display Info - x86_DEBUG_STATE64");
                break;
                
            case x86_DEBUG_STATE:
                printf("TODO: Display Info - x86_DEBUG_STATE");
                break;
                
            case THREAD_STATE_NONE:
                printf("TODO: Display Info - THREAD_STATE_NONE");
                break;
        }

    }
    
    printf("\n");
}

static void printLoadDylib(MachOFile& machofile, const dylib_command_info_t* info)
{
    const char* cmd_name = "";
    
    switch (info->cmd_type) {
        case LC_ID_DYLIB:
            cmd_name = "LC_ID_DYLIB";
            break;
        case LC_LOAD_WEAK_DYLIB:
            cmd_name = "LC_LOAD_WEAK_DYLIB";
            break;
        case LC_LOAD_DYLIB:
            cmd_name = "LC_LOAD_DYLIB";
            break;
        case LC_REEXPORT_DYLIB:
            cmd_name = "LC_REEXPORT_DYLIB";
            break;
    }
    
    printf("%s\n", cmd_name);
    
    printf("\tCommand\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmd));
    printf("\t\tData  : 0x%X\n", info->cmd->cmd);
    printf("\t\tValue : %s\n", cmd_name);
    
    printf("\tCommand Size\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->cmdsize));
    printf("\t\tData  : 0x%X\n", info->cmd->cmdsize);
    printf("\t\tValue : %d\n", info->cmd->cmdsize);
    
    printf("\tStr Offset\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->dylib.name.offset));
    printf("\t\tData  : 0x%X\n", info->cmd->dylib.name.offset);
    printf("\t\tValue : %d\n", info->cmd->dylib.name.offset);
    
    printf("\tTime Stamp\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->dylib.timestamp));
    printf("\t\tData  : 0x%X\n", info->cmd->dylib.timestamp);
    time_t time = (time_t)info->cmd->dylib.timestamp;
    printf("\t\tValue : %s", ctime(&time));
    
    printf("\tCurrent Version\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->dylib.current_version));
    printf("\t\tData  : 0x%X\n", info->cmd->dylib.current_version);
    printf("\t\tValue : %u.%u.%u\n", (info->cmd->dylib.current_version >> 16), ((info->cmd->dylib.current_version >> 8) & 0xff), (info->cmd->dylib.current_version & 0xff));
    
    printf("\tCompatibility Version\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)&info->cmd->dylib.compatibility_version));
    printf("\t\tData  : 0x%X\n", info->cmd->dylib.compatibility_version);
    printf("\t\tValue : %u.%u.%u\n", (info->cmd->dylib.compatibility_version >> 16), ((info->cmd->dylib.compatibility_version >> 8) & 0xff), (info->cmd->dylib.compatibility_version & 0xff));
    
    printf("\tName\n");
    printf("\t\tOffset: 0x%08llx\n", machofile.getOffset((void*)info->libname));
    printf("\t\tValue : %s\n", info->libname);
    
    printf("\n");
}

static void printLoadCommands(MachOFile& machofile)
{
    printf("\n***** Load Commands *****\n");
    const load_command_infos_t& load_cmd_infos = machofile.getLoadCommandInfos();
    
    load_command_infos_t::const_iterator iter;
    for (iter = load_cmd_infos.begin(); iter != load_cmd_infos.end(); iter++) {
        const load_command_info_t& info = *iter;
        
        switch (info.cmd_type) {
            case LC_SEGMENT:
                printf("LC_SEGMENT (TODO: Details)\n\n");
                break;
                
            case LC_SEGMENT_64:
                printSegmentCommand64(machofile, (const segment_command_64_info_t*)info.cmd_info);
                break;
                
            case LC_SYMTAB:
                printf("LC_SYMTAB (TODO: Details)\n\n");
                break;
                
            case LC_DYSYMTAB:
                printf("LC_DYSYMTAB (TODO: Details)\n\n");
                break;
                
            case LC_TWOLEVEL_HINTS:
                printf("LC_TWOLEVEL_HINTS (TODO: Details)\n\n");
                break;
                
            case LC_ID_DYLINKER:
                printf("LC_ID_DYLINKER (TODO: Details)\n\n");
                break;
                
            case LC_LOAD_DYLINKER:
                printf("LC_LOAD_DYLINKER (TODO: Details)\n\n");
                break;
                
            case LC_PREBIND_CKSUM:
                printf("LC_PREBIND_CKSUM (TODO: Details)\n\n");
                break;
                
            case LC_UUID:
                printf("LC_UUID (TODO: Details)\n\n");
                break;
                
            case LC_THREAD:
            case LC_UNIXTHREAD:
                printThreadCommand(machofile, (const thread_command_info_t *)info.cmd_info);
                break;
                
            case LC_ID_DYLIB:
            case LC_LOAD_DYLIB:
            case LC_LOAD_WEAK_DYLIB:
            case LC_REEXPORT_DYLIB:
                printLoadDylib(machofile, (const dylib_command_info_t*)info.cmd_info);
                break;
                
            case LC_LAZY_LOAD_DYLIB:
                printf("LC_LAZY_LOAD_DYLIB (TODO: Details)\n\n");
                break;
                
#ifdef __MAC_10_7
            case LC_LOAD_UPWARD_DYLIB:
                printf("LC_LOAD_UPWARD_DYLIB (TODO: Details)\n\n");
                break;
#endif
                
            case LC_CODE_SIGNATURE:
                printf("LC_CODE_SIGNATURE (TODO: Details)\n\n");
                break;
                
            case LC_SEGMENT_SPLIT_INFO:
                printf("LC_SEGMENT_SPLIT_INFO (TODO: Details)\n\n");
                break;

#ifdef __MAC_10_7
            case LC_FUNCTION_STARTS:
                printf("LC_FUNCTION_STARTS (TODO: Details)\n\n");
                break;
#endif
                
            case LC_ENCRYPTION_INFO:
                printf("LC_ENCRYPTION_INFO (TODO: Details)\n\n");
                break;
                
            case LC_RPATH:
                printf("LC_RPATH (TODO: Details)\n\n");
                break;
                
            case LC_ROUTINES:
                printf("LC_ROUTINES (TODO: Details)\n\n");
                break;
                
            case LC_ROUTINES_64:
                printf("LC_ROUTINES_64 (TODO: Details)\n\n");
                break;
                
            case LC_SUB_FRAMEWORK:
                printf("LC_SUB_FRAMEWORK (TODO: Details)\n\n");
                break;
                
            case LC_SUB_UMBRELLA:
                printf("LC_SUB_UMBRELLA (TODO: Details)\n\n");
                break;
                
            case LC_SUB_CLIENT:
                printf("LC_SUB_CLIENT (TODO: Details)\n\n");
                break;
                
            case LC_SUB_LIBRARY:
                printf("LC_SUB_LIBRARY (TODO: Details)\n\n");
                break;
                
            case LC_DYLD_INFO:
            case LC_DYLD_INFO_ONLY:
                printDyldInfo(machofile, (const dyld_info_command_info_t*)info.cmd_info);
                break;
                
#ifdef __MAC_10_7
            case LC_VERSION_MIN_MACOSX:
                printf("LC_VERSION_MIN_MACOSX (TODO: Details)\n\n");
                break;
                
            case LC_VERSION_MIN_IPHONEOS:
                printf("LC_VERSION_MIN_IPHONEOS (TODO: Details)\n\n");
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

static void parseUniversal(MachOFile& machoFile)
{
    printf("Type: Universal file\n");
    
    const fat_arch_infos_t& infos = machoFile.getFatArchInfos();
    
    int archNum = 1;
    fat_arch_infos_t::const_iterator iter;
    for (iter=infos.begin(); iter!=infos.end(); iter++) {
        const struct fat_arch& arch = iter->arch;
        
        printf("\tArch %d\n", archNum);
        printf("\t\tCPU Type: %s\n", getCPUTypeString(arch.cputype));
        printf("\t\tCPU SubType: 0x%X\n", arch.cpusubtype);
        printf("\t\tOffset: %d\n", arch.offset);
        printf("\t\tSize: %u\n", arch.size);
        printf("\t\tAlign: %f\n", pow(2, arch.align));

        archNum++;
    }
    
    printf("\n");
    
    archNum = 1;
    for (iter=infos.begin(); iter!=infos.end(); iter++) {
        printf("********** Arch %d **********\n", archNum++);
        
        const fat_arch_info_t& fat_arch_info = *iter;
        
        MachOFile machoFile;
        
        if (machoFile.parse_macho(&(fat_arch_info.input))) {
            if (machoFile.isUniversal()) {
                parseUniversal(machoFile);
            }
            else if (machoFile.is64bit())
            {
                printHeader(machoFile);
                printLoadCommands(machoFile);
            }
            else if (machoFile.is32bit())
            {
                printf("TODO: support 32-bit mach-o format\n\n");
            }
        }
    }
}

int main(int argc, const char * argv[])
{
    MachOFile machoFile;
    
    if (machoFile.parse_file(argv[1])) {
        printf("File: %s\n", argv[1]);
        
        if (machoFile.isUniversal()) {
            parseUniversal(machoFile);
        }
        else if (machoFile.is64bit())
        {
            printHeader(machoFile);
            printLoadCommands(machoFile);
        }
        else if (machoFile.is32bit())
        {
            printf("TODO: support 32-bit mach-o format\n");
        }
    }
    else
    {
        printf("error parsing %s", argv[1]);
    }

    return 0;
}

