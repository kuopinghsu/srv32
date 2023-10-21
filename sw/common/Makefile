include Makefile.common

memsize ?= 256

OBJECTS = startup.o syscall.o
TARGET = libsys.a

.PHONY: all clean default.ld

all: $(TARGET) default.ld

.SUFFIXES: .c .S .h .o

default.ld: default.lds
	$(CPP) -DMEMSIZE=$(memsize) -o $@ -E $<

%.o: %.S %.h
	$(AS) $(ASFLAGS) -o $@ -c $<

%.o: %.c %.S %.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ *.o

clean:
	$(RM) $(OBJECTS) $(TARGET) default.ld

