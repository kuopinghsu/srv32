
.PHONY: all clean hello dhrystone coremark common

all: hello #dhrystone coremark

hello: common
	make -C hello

dhrystone: common
	make -C dhrystone

coremark: common
	make -C coremark

common:
	make -C common

clean:
	make -C common clean
	make -C hello clean
	make -C dhrystone clean
	make -C coremark clean
