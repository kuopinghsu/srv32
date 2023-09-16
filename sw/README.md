# Diags

This folder is a software diagnostic file used to test the hardware core. Folders prefixed with "_" will not be tested in regression.

## Lists

| folder | description |
| --- | --- |
| common | common path for link script, startup and syscall |
| _file | file I/O operation test (for ISS simulator only) |
| _io | standard I/O test (for ISS simulator only) |
| coremark | coremark benchmark |
| cpp | C++ example for global constructor (provided by chatGPT) |
| dhrystone | dhrystone benchmark |
| exception | exception test |
| irq | IRQ test & various CSR tests |
| perf | FreeRTOS performance test |
| pi | PI calculation |
| pi_pthread | FreeRTOS pthread example to calculate PI |
| hello | hello world example |
| qsort | quick sort |
| scimark2 | SciMark2 (C version) |
| sem | FreeRTOS semaphor test |

## LICENSE & NOTES

*   The dhrystone and coremark are licensed by their respective authors, please refer to the header file for permission.
*   cpp C++ example provided by chatGPT.
*   perf is the FreeRTOS performane test is form [here](https://github.com/foss-xtensa/amazon-freertos/tree/xtensa-v10.2.1-stable/demos/cadence/sim/common/application_code/cadence_code)
*   pi is written by Dik T. Winter
*   pi_pthread is referenced form [here](https://www.stolaf.edu/people/rab/os/pub0/modules/Pi_Integration_SharedMemory/Pthreads/Pthreads.html)
*   [scimark2](http://math.nist.gov/scimark)
*   FreeRTOS diags only ELF file is provided, to build the code, refer [here](https://github.com/kuopinghsu/FreeRTOS-RISCV) for details.
*   The other folders are licensed under the MIT written by Kuoping Hsu.
