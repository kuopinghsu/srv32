.PHONY: hello dhrystone coremark tools

hello: clean tools
	@make -C sw hello
	@make -C sim

dhrystone: clean tools
	@make -C sw dhrystone
	@make -C sim

coremark: clean tools
	@make -C sw coremark
	@make -C sim

tools:
	@make -C tools

clean:
	@make -C sw clean
	@make -C sim clean
	@make -C tools clean

