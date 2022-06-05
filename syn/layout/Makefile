
all: pretest qflow gds

qflow: syn place route sta

syn:
	qflow -T gscl45nm synthesize top

place:
	qflow -T gscl45nm place top

route:
	qflow -T gscl45nm route top

sta:
	qflow -T gscl45nm sta top

gds:
	echo "\
        lef read /usr/share/qflow/tech/gscl45nm/gscl45nm.lef \n\
        def read top.def \n\
        writeall force top \n\
    " | magic -dnull -noconsole
	echo "\
        gds read /usr/share/qflow/tech/gscl45nm/gscl45nm.gds \n\
        writeall force \n\
    " | magic -dnull -noconsole
	echo "\
        gds write top\n\
    " | magic -dnull -noconsole top

pretest:
	ln -s ../../rtl/*.v .
	ln -s ../../rtl/*.vh .

clean:
	-rm -rf top* log qflow_* tmp.blif project_vars.sh
	-rm *.v *.vh *.mag

