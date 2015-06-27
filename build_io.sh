#! /bin/bash

# Make a directory in /tmp/OpenFPM_data

mkdir /tmp/openfpm_io
mv * .[^.]* /tmp/openfpm_io
mv /tmp/openfpm_io OpenFPM_IO

mkdir OpenFPM_IO/src/config

git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_devices.git OpenFPM_devices
git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_data.git OpenFPM_data
git clone git@ppmcore.mpi-cbg.de:incardon/openfpm_pdata.git OpenFPM_pdata
cd "$1/OpenFPM_data"
git checkout develop
cd ..

cd "$1/OpenFPM_IO"

sh ./autogen.sh
if [ "$2" == "master" ]
then
 sh ./configure --disable-gpu
else if [ "$2" == "gin" ]
then
 sh ./configure --with-boost=/sw/apps/boost/1.54.0/
else
 sh ./configure
fi
make

./src/io

