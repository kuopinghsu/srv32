// Copyright © 2020 Kuoping Hsu
// rvsim.c: Instruction Set Simulator for RISC-V RV32I instruction sets
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "opcode.h"

extern int *dmem;
extern int mem_base;
extern int mem_size;
void prog_exit(int exitcode);

static int result = 0;

void srv32_tohost(
    int32_t htif_mem)
{
    char *ptr = (char*)dmem;
    int *htifMem = (int*)&ptr[DVA2PA(htif_mem)];

    int func = htifMem[0];
    int a0   = htifMem[1];
    int a1   = htifMem[2];
    int a2   = htifMem[3];
    //int a3   = htifMem[4];
    //int a4   = htifMem[5];
    //int a5   = htifMem[6];
    //int a6   = htifMem[7];

    switch(func) {
       case SYS_OPEN:
           result = (int)open((const char*)(&ptr[DVA2PA(a0)]),
                              O_RDWR | O_CREAT /* a1 */,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH /* a2 */ );
           break;
       case SYS_CLOSE:
           result = (int)close(a0);
           break;
       case SYS_LSEEK:
           result = (int)lseek(a0, a1, a2);
           break;
       case SYS_EXIT:
           result = 0;
           prog_exit(a0);
           break;
       case SYS_READ:
           result = (int)read(a0, (void *)(&ptr[DVA2PA(a1)]), a2);
           break;
       case SYS_WRITE:
           result = (int)write(a0, (const char*)(&ptr[DVA2PA(a1)]), a2);
           break;
       case SYS_DUMP: {
               FILE *fp;
               int i;
               if ((fp = fopen("dump.txt", "w")) == NULL) {
                   printf("Create dump.txt fail\n");
                   exit(1);
               }
               if ((a0 & 3) != 0 || (a1 & 3) != 0) {
                   printf("Alignment error on memory dumping.\n");
                   exit(1);
               }
               for(i = DVA2PA(a0)/4;
                   i < DVA2PA(a1)/4; i++) {
                   fprintf(fp, "%08x\n", dmem[i]);
               }
               fclose(fp);
           }
           result = a1;
           break;
       case SYS_DUMP_BIN: {
               FILE *fp;
               int i;
               if ((fp = fopen("dump.bin", "wb")) == NULL) {
                   printf("Create dump.bin fail\n");
                   exit(1);
               }
               for(i = DVA2PA(a0);
                   i < DVA2PA(a1); i++) {
                   fprintf(fp, "%c", (dmem[i/4]>>((i%4)*8))&0xff);
               }
               fclose(fp);
           }
           result = a1;
           break;
       default:
           break;
    }
}

