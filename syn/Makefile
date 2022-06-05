.PHONY: syn clean layout

syn:
	yosys -s script/synth.scr

layout:
	make -C layout

clean:
	-$(RM) -f output/synth.* report/*.rpt
	make -C layout clean

