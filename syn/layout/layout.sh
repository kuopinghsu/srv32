#!/bin/bash

rm -rf riscv* log project_vars.sh qflow_* tmp.blif
rm *.v *.vh

ln -s ../../rtl/*.v .
ln -s ../../rtl/*.vh .

qflow synthesize riscv
qflow place riscv
qflow route riscv

wget http://opencircuitdesign.com/qflow/example/osu035_stdcells.gds2

magic -dnull -noconsole <<'END'
  lef read /usr/share/qflow/tech/osu035/osu035_stdcells.lef
  def read riscv.def
  writeall force riscv
END

magic -dnull -noconsole <<'END'
  gds read osu035_stdcells.gds2
  writeall force
END

magic -dnull -noconsole riscv <<'END'
  gds write riscv
END
