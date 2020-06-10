#pragma once
/*
 *  This file contains definitions for compile-time configuration of the kernel's behavior
 *  (Mainly logging toggles)
 */

//  Log VMapping creations/frees
//#define VMM_LOG_VMAPPING

//  Log PMM token freeing
//#define PMM_LOG_TOKEN_FREES

//  Log creating processes from flat binaries
//#define PROCESS_LOG_BIN_CREATION

//  Log creating processes from ELF
//#define PROCESS_LOG_ELF_CREATION

//  Log creating page directories for processes
//#define LOG_PAGEDIR_CREATION

//  Log QuickMapper mappings
//#define QUICKMAP_LOG_MAPS
