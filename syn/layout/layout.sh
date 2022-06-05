#!/bin/bash

rm -rf top* log qflow_* tmp.blif
rm *.v *.vh

ln -s ../../rtl/*.v .
ln -s ../../rtl/*.vh .

qflow -T gscl45nm synthesize top
qflow -T gscl45nm place top
qflow -T gscl45nm route top

magic -dnull -noconsole <<'END'
  lef read /usr/share/qflow/tech/gscl45nm/gscl45nm.lef
  def read top.def
  writeall force top
END

magic -dnull -noconsole <<'END'
  gds read /usr/share/qflow/tech/gscl45nm/gscl45nm.gds
  writeall force
END

magic -dnull -noconsole top <<'END'
  gds write top
END
