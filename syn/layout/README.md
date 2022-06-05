
# QFlow Installation on Ubuntu

## install packages

  % sudo apt update
  % sudo apt -y install build-essential
  % sudo apt -y install m4 csh cmake libx11-dev tcl-dev tk-dev libz-dev libgsl-dev
  % sudo apt -y install python3 python3-numpy ngspice
  % sudo apt -y install yosys magic

## Installing netgen

  % git clone https://github.com/RTimothyEdwards/netgen.git
  % cd netgen
  % git checkout netgen-1.5
  % ./configure --prefix=/usr
  % make
  % sudo make install

## Installing graywolf

  % git clone https://github.com/rubund/graywolf.git
  % cd graywolf
  % git checkout 0.1.6
  % mkdir build
  % cd build
  % cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  % make
  % sudo make install

## Installing Qrouter

  % git clone https://github.com/RTimothyEdwards/qrouter.git
  % cd qrouter
  % git checkout qrouter-1.4
  % ./configurer --prefix=/usr
  % make
  % sudo make install

## Install qflow

  % git clone https://github.com/RTimothyEdwards/qflow.git
  % cd qflow
  % git checkout qflow-1.4
  % ./configure --prefix=/usr
  % make
  % sudo make install

