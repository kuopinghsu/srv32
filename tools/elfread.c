// Copyright © 2020 Kuoping Hsu
// ELF reader
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "elf.h"

#define VERBOSE 0
#define LIBRARY 1

#ifndef USE_LIBBFD
static int elf32_read(FILE *fp, char *imem, char *dmem, int *isize, int *dsize)
{
    int i;
    Elf32_Ehdr elf32_header;
    Elf32_Shdr *elf32_sechdr = NULL;
    char *secName = NULL;

    fseek(fp, 0, SEEK_SET);
    if (!fread(&elf32_header, sizeof(Elf32_Ehdr), 1, fp)) {
        printf("File read fail\n");
        goto fail;
    }

    if (!(elf32_sechdr =
           (Elf32_Shdr*)malloc(sizeof(Elf32_Shdr) * elf32_header.e_shnum))) {
        goto fail;
    }

    fseek(fp, elf32_header.e_shoff, SEEK_SET);
    if (!fread(elf32_sechdr,
               sizeof(Elf32_Shdr) * elf32_header.e_shnum, 1, fp)) {
        printf("File read fail\n");
        goto fail;
    }

    fseek(fp, elf32_sechdr[elf32_header.e_shstrndx].sh_offset, SEEK_SET);
    if (!(secName = malloc(elf32_sechdr[elf32_header.e_shstrndx].sh_size))) {
        printf("Malloc fail\n");
        goto fail;
    }

    if (!fread(secName,
               elf32_sechdr[elf32_header.e_shstrndx].sh_size, 1, fp)) {
        printf("File read fail\n");
        goto fail;
    }

    if (VERBOSE) printf("[Nr] Type\tAddress\t\tOffset\t\tSize\t\tName\n");
    for(i = 0; i < elf32_header.e_shnum; i++) {
        char *names = secName + elf32_sechdr[i].sh_name;

        if (!strncmp(names, ".text", 6)) {
            *isize = (int)elf32_sechdr[i].sh_size;
            fseek(fp, (int)elf32_sechdr[i].sh_offset, SEEK_SET);
            if (imem != NULL &&
                !fread(imem, (int)elf32_sechdr[i].sh_size, 1, fp)) {
                printf("File read fail\n");
                goto fail;
            }
        } else if (!strncmp(names, ".data", 6)) {
            *dsize = (int)elf32_sechdr[i].sh_size;
            fseek(fp, (int)elf32_sechdr[i].sh_offset, SEEK_SET);
            if (dmem != NULL &&
                !fread(dmem, (int)elf32_sechdr[i].sh_size, 1, fp)) {
                printf("File read fail\n");
                goto fail;
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

    if ((!imem && !*isize) || (!dmem && !*dsize)) {
        goto fail;
    }

    if (elf32_sechdr) free(elf32_sechdr);
    if (secName) free(secName);
    return 1;

fail:
    if (elf32_sechdr) free(elf32_sechdr);
    if (secName) free(secName);
    return 0;
}

// LCOV_EXCL_START
static int elf64_read(FILE *fp, char *imem, char *dmem, int *isize, int *dsize)
{
    int i;
    Elf64_Ehdr elf64_header;
    Elf64_Shdr *elf64_sechdr = NULL;
    char *secName = NULL;

    fseek(fp, 0, SEEK_SET);
    if (!fread(&elf64_header, sizeof(Elf64_Ehdr), 1, fp)) {
        printf("File read fail\n");
        goto fail;
    }

    if (!(elf64_sechdr =
           (Elf64_Shdr*)malloc(sizeof(Elf64_Shdr) * elf64_header.e_shnum))) {
        goto fail;
    }

    fseek(fp, elf64_header.e_shoff, SEEK_SET);
    if (!fread(elf64_sechdr,
               sizeof(Elf64_Shdr) * elf64_header.e_shnum, 1, fp)) {
        printf("File read fail\n");
        goto fail;
    }

    fseek(fp, elf64_sechdr[elf64_header.e_shstrndx].sh_offset, SEEK_SET);
    if (!(secName = malloc(elf64_sechdr[elf64_header.e_shstrndx].sh_size))) {
        printf("Malloc fail\n");
        goto fail;
    }

    if (!fread(secName,
               elf64_sechdr[elf64_header.e_shstrndx].sh_size, 1, fp)) {
        printf("File read fail\n");
        goto fail;
    }

    if (VERBOSE) printf("[Nr] Type\tAddress\t\tOffset\t\tSize\t\tName\n");
    for(i = 0; i < elf64_header.e_shnum; i++) {
        char *names = secName + elf64_sechdr[i].sh_name;

        if (!strncmp(names, ".text", 6)) {
            *isize = (int)elf64_sechdr[i].sh_size;
            fseek(fp, (int)elf64_sechdr[i].sh_offset, SEEK_SET);
            if (imem != NULL &&
                !fread(imem, (int)elf64_sechdr[i].sh_size, 1, fp)) {
                printf("File read fail\n");
                goto fail;
            }
        } else if (!strncmp(names, ".data", 6)) {
            *dsize = (int)elf64_sechdr[i].sh_size;
            fseek(fp, (int)elf64_sechdr[i].sh_offset, SEEK_SET);
            if (dmem != NULL &&
                !fread(dmem, (int)elf64_sechdr[i].sh_size, 1, fp)) {
                printf("File read fail\n");
                goto fail;
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

    if ((!imem && !*isize) || (!dmem && !*dsize)) {
        goto fail;
    }

    if (elf64_sechdr) free(elf64_sechdr);
    if (secName) free(secName);
    return 1;

fail:
    if (elf64_sechdr) free(elf64_sechdr);
    if (secName) free(secName);
    return 0;
}
// LCOV_EXCL_STOP

int elfread(char *file, char *imem, char *dmem, int *isize, int *dsize)
{
    FILE *fp;
    char elf_header[EI_NIDENT];

    if ((fp = fopen(file, "r")) == NULL) {
        printf("Can not open file %s\n", file);
        return 0;
    }

    if (!fread(&elf_header, sizeof(elf_header), 1, fp)) {
        printf("Can not read file %s\n", file);
        fclose(fp);
        return 0;
    }

    if (elf_header[0] == 0x7F || elf_header[1] == 'E') {
        if (elf_header[EI_CLASS] == 1) { // ELF32
            int result = elf32_read(fp, imem, dmem, isize, dsize);
            fclose(fp);
            return result;
        } else { // ELF64
            // LCOV_EXCL_START
            int result = elf64_read(fp, imem, dmem, isize, dsize);
            fclose(fp);
            return result;
            // LCOV_EXCL_STOP
        }
    } else {
        printf("The file %s is not an ELF format\n", file);
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

#if LIBRARY == 0
int main(int argc, char **argv)
{
    int result;
    int isize, dsize;

    if (argc != 2) {
        printf("Usage: elfread file.elf\n");
        return 1;
    }

    result = elfread(argv[1], NULL, NULL, &isize, &dsize);

    return result ? 0 : 1;
}
#endif // LIBRARY
#endif // USE_LIBBFD

