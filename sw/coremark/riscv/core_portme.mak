# Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# Original Author: Shay Gal-on

#File : core_portme.mak

# Flag : OUTFLAG
#	Use this flag to define how to to get an executable (e.g -o)
OUTFLAG= -o
# Flag : CC
#	Use this flag to define compiler to use
include ../common/Makefile.common

ITERATIONS = 4

# Flag : CFLAGS
#	Use this flag to define compiler options. Note, you can add compiler options from the command line using XCFLAGS="other flags"
PORT_CFLAGS = -O3 $(ARCH) -nostartfiles -nostdlib -L../common -DPERFORMANCE_RUN=1
PORT_CFLAGS += -fno-common -funroll-loops -finline-functions -falign-functions=16
PORT_CFLAGS += -falign-jumps=4 -falign-loops=4 -finline-limit=1000
PORT_CFLAGS += -fno-if-conversion2 -fselective-scheduling -fno-tree-dominator-opts
PORT_CFLAGS += -fno-reg-struct-return -fno-rename-registers
PORT_CFLAGS += --param case-values-threshold=8 -fno-crossjumping
PORT_CFLAGS += -freorder-blocks-and-partition -fno-tree-loop-if-convert
PORT_CFLAGS += -fno-tree-sink -fgcse-sm -fno-strict-overflow

FLAGS_STR = "$(PORT_CFLAGS) $(XCFLAGS) $(XLFLAGS) $(LFLAGS_END)"
CFLAGS = $(PORT_CFLAGS) -I$(PORT_DIR) -I. -DFLAGS_STR=\"$(FLAGS_STR)\"
#Flag : LFLAGS_END
#	Define any libraries needed for linking or other flags that should come at the end of the link line (e.g. linker scripts). 
#	Note : On certain platforms, the default clock_gettime implementation is supported but requires linking of librt.
LFLAGS_END = -lc -lm -lgcc -lsys -T ../common/default.ld
# Flag : PORT_SRCS
# 	Port specific source files can be added here
PORT_SRCS = $(PORT_DIR)/core_portme.c
# Flag : LOAD
#	For a simple port, we assume self hosted compile and run, no load needed.

# Flag : RUN
#	For a simple port, we assume self hosted compile and run, simple invocation of the executable

#For native compilation and execution
LOAD = echo Loading done
RUN = $(CROSS_COMPILER)run

OEXT = .o
EXE = .elf

# Target : port_pre% and port_post%
# For the purpose of this simple port, no pre or post steps needed.

.PHONY : port_prebuild port_postbuild port_prerun port_postrun port_preload port_postload
port_pre% port_post% : 

# FLAG : OPATH
# Path to the output folder. Default - current folder.
OPATH = ./
MKDIR = mkdir -p

