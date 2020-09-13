#ifndef __ELF_H
#define __ELF_H

#pragma pack(push,1)
#pragma pack(pop)

#define EI_NIDENT 16
#define EI_CLASS  4

/* 32-bit ELF base types. */
typedef unsigned int        Elf32_Addr;
typedef unsigned short      Elf32_Half;
typedef unsigned int        Elf32_Off;
typedef signed int          Elf32_Sword;
typedef unsigned int        Elf32_Word;

/* 64-bit ELF base types. */
typedef unsigned long long  Elf64_Addr;
typedef unsigned short      Elf64_Half;
typedef signed short        Elf64_SHalf;
typedef unsigned long long  Elf64_Off;
typedef signed int          Elf64_Sword;
typedef unsigned int        Elf64_Word;
typedef unsigned long long  Elf64_Xword;
typedef signed long long    Elf64_Sxword;

typedef struct elf32_hdr{
    unsigned char           e_ident[EI_NIDENT];
    Elf32_Half              e_type;
    Elf32_Half              e_machine;
    Elf32_Word              e_version;
    Elf32_Addr              e_entry;
    Elf32_Off               e_phoff;
    Elf32_Off               e_shoff;
    Elf32_Word              e_flags;
    Elf32_Half              e_ehsize;
    Elf32_Half              e_phentsize;
    Elf32_Half              e_phnum;
    Elf32_Half              e_shentsize;
    Elf32_Half              e_shnum;
    Elf32_Half              e_shstrndx;
} Elf32_Ehdr;

typedef struct elf32_shdr {
    Elf32_Word              sh_name;
    Elf32_Word              sh_type;
    Elf32_Word              sh_flags;
    Elf32_Addr              sh_addr;
    Elf32_Off               sh_offset;
    Elf32_Word              sh_size;
    Elf32_Word              sh_link;
    Elf32_Word              sh_info;
    Elf32_Word              sh_addralign;
    Elf32_Word              sh_entsize;
} Elf32_Shdr;

typedef struct elf64_hdr {
    unsigned char           e_ident[EI_NIDENT];
    Elf64_Half              e_type;
    Elf64_Half              e_machine;
    Elf64_Word              e_version;
    Elf64_Addr              e_entry;
    Elf64_Off               e_phoff;
    Elf64_Off               e_shoff;
    Elf64_Word              e_flags;
    Elf64_Half              e_ehsize;
    Elf64_Half              e_phentsize;
    Elf64_Half              e_phnum;
    Elf64_Half              e_shentsize;
    Elf64_Half              e_shnum;
    Elf64_Half              e_shstrndx;
} Elf64_Ehdr;

typedef struct elf64_shdr {
    Elf64_Word              sh_name;
    Elf64_Word              sh_type;
    Elf64_Xword             sh_flags;
    Elf64_Addr              sh_addr;
    Elf64_Off               sh_offset;
    Elf64_Xword             sh_size;
    Elf64_Word              sh_link;
    Elf64_Word              sh_info;
    Elf64_Xword             sh_addralign;
    Elf64_Xword             sh_entsize;
} Elf64_Shdr;

#endif // __ELF_H
