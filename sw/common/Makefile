include Makefile.common

OBJECTS = startup.o syscall.o
TARGET = libsys.a

.PHONY: all clean

all: $(TARGET)

.SUFFIXES: .c .S .h .o

%.o: %.S %.h
	$(AS) $(ASFLAGS) -o $@ -c $<

%.o: %.c %.S %.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ *.o

clean:
	$(RM) $(OBJECTS) $(TARGET)
