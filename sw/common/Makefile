include Makefile.common

OBJECTS = startup.o syscall.o
TARGET = libsys.a

.PHONY: all clean

all: $(TARGET)

.SUFFIXES: .c .S .h .o
%.o: %.S S %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ *.o

clean:
	$(RM) $(OBJECTS) $(TARGET)
