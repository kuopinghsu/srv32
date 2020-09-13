// elfreader
// Copyright 2020, Kuoping Hsu, GPL license

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "elf.h"

#define VERBOSE 0

static int elf32_read(FILE *fp, char *imem, char *dmem, int *isize, int *dsize)
{
    int i, size;
    Elf32_Ehdr elf32_header;
    Elf32_Shdr *elf32_sechdr;
    char *secName;

    fseek(fp, 0, SEEK_SET);
    if (!(size = fread(&elf32_header, sizeof(Elf32_Ehdr), 1, fp))) {
        printf("File read fail\n");
        return 0;
    }

    if (!(elf32_sechdr = (Elf32_Shdr*)malloc(sizeof(Elf32_Shdr) * elf32_header.e_shnum))) {
        printf("Malloc fail\n");
        return 0;
    }

    fseek(fp, elf32_header.e_shoff, SEEK_SET);
    if (!(size = fread(elf32_sechdr, sizeof(Elf32_Shdr) * elf32_header.e_shnum, 1, fp))) {
        printf("File read fail\n");
        return 0;
    }

    fseek(fp, elf32_sechdr[elf32_header.e_shstrndx].sh_offset, SEEK_SET);
    if (!(secName = malloc(elf32_sechdr[elf32_header.e_shstrndx].sh_size))) {
        printf("Malloc fail\n");
        return 0;
    }

    if (!(size = fread(secName, elf32_sechdr[elf32_header.e_shstrndx].sh_size, 1, fp))) {
        printf("File read fail\n");
        return 0;
    }

    if (VERBOSE) printf("[Nr] Type\tAddress\t\tOffset\t\tSize\t\tName\n");
    for(i = 0; i < elf32_header.e_shnum; i++) {
        char *names = secName + elf32_sechdr[i].sh_name;

        if (!strncmp(names, ".text", 6)) {
            *isize = (int)elf32_sechdr[i].sh_size;
            fseek(fp, (int)elf32_sechdr[i].sh_offset, SEEK_SET);
            if (imem != NULL && !(size = fread(imem, (int)elf32_sechdr[i].sh_size, 1, fp))) {
                printf("File read fail\n");
                return 0;
            }
        } else if (!strncmp(names, ".data", 6)) {
            *dsize = (int)elf32_sechdr[i].sh_size;
            fseek(fp, (int)elf32_sechdr[i].sh_offset, SEEK_SET);
            if (dmem != NULL && !(size = fread(dmem, (int)elf32_sechdr[i].sh_size, 1, fp))) {
                printf("File read fail\n");
                return 0;
            }
        }

        if (VERBOSE) printf("[%2d] %08x\t%08x\t%08x\t%8d\t%s\n",
                i,
                (int)elf32_sechdr[i].sh_type,
                (int)elf32_sechdr[i].sh_addr,
                (int)elf32_sechdr[i].sh_offset,
                (int)elf32_sechdr[i].sh_size,
                names);
    }

    free(elf32_sechdr);
    free(secName);

    if ((!imem && !*isize) || (!dmem && !*dsize)) {
        return 0;
    }

    return 1;
}

static int elf64_read(FILE *fp, char *imem, char *dmem, int *isize, int *dsize)
{
    int i, size;
    Elf64_Ehdr elf64_header;
    Elf64_Shdr *elf64_sechdr;
    char *secName;

    fseek(fp, 0, SEEK_SET);
    if (!(size = fread(&elf64_header, sizeof(Elf64_Ehdr), 1, fp))) {
        printf("File read fail\n");
        return 0;
    }

    if (!(elf64_sechdr = (Elf64_Shdr*)malloc(sizeof(Elf64_Shdr) * elf64_header.e_shnum))) {
        printf("Malloc fail\n");
        return 0;
    }

    fseek(fp, elf64_header.e_shoff, SEEK_SET);
    if (!(size = fread(elf64_sechdr, sizeof(Elf64_Shdr) * elf64_header.e_shnum, 1, fp))) {
        printf("File read fail\n");
        return 0;
    }

    fseek(fp, elf64_sechdr[elf64_header.e_shstrndx].sh_offset, SEEK_SET);
    if (!(secName = malloc(elf64_sechdr[elf64_header.e_shstrndx].sh_size))) {
        printf("Malloc fail\n");
        return 0;
    }

    if (!(size = fread(secName, elf64_sechdr[elf64_header.e_shstrndx].sh_size, 1, fp))) {
        printf("File read fail\n");
        return 0;
    }

    if (VERBOSE) printf("[Nr] Type\tAddress\t\tOffset\t\tSize\t\tName\n");
    for(i = 0; i < elf64_header.e_shnum; i++) {
        char *names = secName + elf64_sechdr[i].sh_name;

        if (!strncmp(names, ".text", 6)) {
            *isize = (int)elf64_sechdr[i].sh_size;
            fseek(fp, (int)elf64_sechdr[i].sh_offset, SEEK_SET);
            if (imem != NULL && !(size = fread(imem, (int)elf64_sechdr[i].sh_size, 1, fp))) {
                printf("File read fail\n");
                return 0;
            }
        } else if (!strncmp(names, ".data", 6)) {
            *dsize = (int)elf64_sechdr[i].sh_size;
            fseek(fp, (int)elf64_sechdr[i].sh_offset, SEEK_SET);
            if (dmem != NULL && !(size = fread(dmem, (int)elf64_sechdr[i].sh_size, 1, fp))) {
                printf("File read fail\n");
                return 0;
            }
        }

        if (VERBOSE) printf("[%2d] %08x\t%08x\t%08x\t%8d\t%s\n",
                i,
                (int)elf64_sechdr[i].sh_type,
                (int)elf64_sechdr[i].sh_addr,
                (int)elf64_sechdr[i].sh_offset,
                (int)elf64_sechdr[i].sh_size,
                names);
    }

    free(elf64_sechdr);
    free(secName);

    if ((!imem && !*isize) || (!dmem && !*dsize)) {
        return 0;
    }

    return 1;
}

int elfread(char *file, char *imem, char *dmem, int *isize, int *dsize)
{
    FILE *fp;
    char elf_header[EI_NIDENT];
    int size;

    if ((fp = fopen(file, "r")) == NULL) {
        printf("Can not open file %s\n", file);
        return 0;
    }

    if (!(size = fread(&elf_header, sizeof(elf_header), 1, fp))) {
        printf("Can not read file %s\n", file);
        return 0;
    }

    if (elf_header[0] == 0x7F || elf_header[1] == 'E') {
        if (elf_header[EI_CLASS] == 1) { // ELF32
            int result = elf32_read(fp, imem, dmem, isize, dsize);
            fclose(fp);
            return result;
        } else { // ELF64
            int result = elf64_read(fp, imem, dmem, isize, dsize);
            fclose(fp);
            return result;
        }
    } else {
        printf("Can not read file %s\n", file);
        return 0;
    }
    return 1;
}

#if 0
int main(int argc, char **argv)
{
    int result;
    int isize, dsize;

    if (argc != 2) {
        printf("usage: elfread file.elf\n");
        return 1;
    }

    result = elfread(argv[1], NULL, NULL, &isize, &dsize);
    return result ? 0 : 1;
}
#endif

