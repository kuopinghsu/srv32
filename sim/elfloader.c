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
#include "../tools/elf.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

static int elf32_read(FILE *fp, char *mem,
                      int imem_base, int dmem_base,
                      int imem_size, int dmem_size)
{
    int i;
    Elf32_Ehdr elf32_header;
    Elf32_Phdr *elf32_phdr = NULL;
    Elf32_Phdr *ph;
    char *imem = NULL, *dmem = NULL;

    if (mem) {
        imem = (char*)&mem[0];
        dmem = (char*)&mem[imem_size];
    }

    fseek(fp, 0, SEEK_SET);
    if (!fread(&elf32_header, sizeof(Elf32_Ehdr), 1, fp)) {
        // LCOV_EXCL_START
        printf("File read fail\n");
        goto fail;
        // LCOV_EXCL_STOP
    }

    if (!(elf32_phdr =
           (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * elf32_header.e_phnum))) {
        // LCOV_EXCL_START
        goto fail;
        // LCOV_EXCL_STOP
    }

    fseek(fp, elf32_header.e_phoff, SEEK_SET);
    if (!fread(elf32_phdr,
               sizeof(Elf32_Phdr) * elf32_header.e_phnum, 1, fp)) {
        // LCOV_EXCL_START
        printf("File read fail\n");
        goto fail;
        // LCOV_EXCL_STOP
    }

    for(i = 0, ph = elf32_phdr; i < elf32_header.e_phnum; i++, ph++) {
        if (VERBOSE) printf("[%d] 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
                            (int)ph->p_type,
                            (int)ph->p_offset,
                            (int)ph->p_vaddr,
                            (int)ph->p_memsz);

    if (ph->p_type != PT_LOAD)
            continue;

        if (!mem)
            continue;

        fseek(fp, ph->p_offset, SEEK_SET);

        // instruction memory
        if ((int)ph->p_vaddr >= imem_base &&
            (int)(ph->p_vaddr+ph->p_memsz) <= (imem_base+imem_size)) {
            int idx = (int)ph->p_vaddr - imem_base;

            if (VERBOSE) printf("load intruction memory, address 0x%08x, size %d\n",
                                (int)ph->p_vaddr, (int)ph->p_memsz);

            if(!fread((void*)&imem[idx], (int)ph->p_memsz, 1, fp)) {
                // LCOV_EXCL_START
                printf("File read fail\n");
                goto fail;
                // LCOV_EXCL_STOP
            }
            continue;
        }

        // data memory
        if ((int)ph->p_vaddr >= dmem_base &&
            (int)(ph->p_vaddr+ph->p_memsz) <= (dmem_base+dmem_size)) {
            int idx = (int)ph->p_vaddr - dmem_base;

            if (VERBOSE) printf("load data memory, address 0x%08x, size %d\n",
                                (int)ph->p_vaddr, (int)ph->p_memsz);

            if(!fread((void*)&dmem[idx], (int)ph->p_memsz, 1, fp)) {
                // LCOV_EXCL_START
                printf("File read fail\n");
                goto fail;
                // LCOV_EXCL_STOP
            }
            continue;
        }

        // LCOV_EXCL_START
        printf("Error: memory %08x with size %d out of range\n",
               (int)ph->p_vaddr, (int)ph->p_memsz);
        exit(-1);
        // LCOV_EXCL_STOP
    }

    if (elf32_phdr) free(elf32_phdr);
    return 1;

// LCOV_EXCL_START
fail:
    if (elf32_phdr) free(elf32_phdr);
    return 0;
// LCOV_EXCL_STOP
}

extern "C"
int elfloader(char *file, char *mem,
              int imem_base, int dmem_base,
              int imem_size, int dmem_size)
{
    FILE *fp;
    char elf_header[EI_NIDENT];

    if ((fp = fopen(file, "rb")) == NULL) {
        // LCOV_EXCL_START
        printf("Can not open file %s\n", file);
        return 0;
        // LCOV_EXCL_STOP
    }

    if (!fread(&elf_header, sizeof(elf_header), 1, fp)) {
        // LCOV_EXCL_START
        printf("Can not read file %s\n", file);
        fclose(fp);
        return 0;
        // LCOV_EXCL_STOP
    }

    if (elf_header[0] == 0x7F || elf_header[1] == 'E') {
        if (elf_header[EI_CLASS] == 1) { // ELF32
            int result = elf32_read(fp, mem, imem_base, dmem_base, imem_size, dmem_size);
            fclose(fp);
            return result;
        } else { // ELF64
            // LCOV_EXCL_START
            return 0;
            // LCOV_EXCL_STOP
        }
    } else {
        // LCOV_EXCL_START
        printf("The file %s is not an ELF format\n", file);
        fclose(fp);
        return 0;
        // LCOV_EXCL_STOP
    }

    fclose(fp);
    return 1;
}

