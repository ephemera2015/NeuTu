#!/bin/bash
function initdir
{
  dirname=$1
  echo "Checking $dirname"
  x=`mkdir -p $dirname 2>&1 || true`
  username=$USER
  if [[ "$x" == *"Permission denied"* ]]
  then
    sudo ./sudo_initdir $dirname $username
  fi
}

set -e 


if [ `uname` = 'Darwin' ]; then
  scriptDir=$(cd $(dirname "$0") && pwd -P)
  export PATH=/usr/bin:/usr/local/bin:/bin:/sbin
else
  scriptDir=$(dirname `readlink -f "$0"`)
  export PATH=/usr/bin:/usr/local/bin:/bin
fi

if test $# -eq 0
then
  install_dir=/opt
else
  install_dir=$1
fi

if [ "${install_dir:0:1}" = "/" ]
then
  install_dir=$install_dir
else
  install_dir=$PWD/$install_dir
fi

if [ $# -gt 0 ]
then
  package=$2
fi

if [ $# -gt 1 ]
then
  channel=$3
fi

echo "Installing NeuTu under $install_dir"
initdir $install_dir
bindir=$install_dir/bin
initdir $bindir


downloadDir=$install_dir/Download
initdir $downloadDir

if [ -z $condaDir ]
then
  condaDir=$install_dir/Download/miniconda
fi
#echo $PATH
#which md5
#exit
cd $downloadDir
if [ ! -d $condaDir ]
then
  if [ `uname` = 'Darwin' ]; then
    curl -X GET https://repo.continuum.io/miniconda/Miniconda3-latest-MacOSX-x86_64.sh > Miniconda-latest-x86_64.sh
  else
    curl -X GET https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh > Miniconda-latest-x86_64.sh
  fi
  bash Miniconda-latest-x86_64.sh -b -p $condaDir
fi

cd $scriptDir
condarc=$condaDir/.condarc
echo 'channels:' > $condarc
echo '  - flyem-forge' >> $condarc
echo '  - conda-forge/label/cf201901' >> $condarc
echo '  - defaults' >> $condarc
#cp condarc $condarc

channel_arg=''
if [ ! -z $channel ]
then
  channel_arg="-c $channel"
fi

envName='neutu-env'
source $condaDir/bin/activate root
conda create -n $envName python=3.6 -y
source $condaDir/bin/activate $envName
conda install neutu -y $channel_arg

updateFile=$bindir/ntupd
touch $updateFile
echo '#!/bin/bash' > $updateFile
echo "source  $condaDir/bin/activate $envName" >> $updateFile
echo "conda update neutu -y $channel_arg" >> $updateFile
chmod u+x $updateFile

if [ `uname` = 'Darwin' ]
then
  neutu_bin_dir=$condaDir'/envs/neutu-env/bin/neutu.app/Contents/MacOS'
else
  neutu_bin_dir=$condaDir'/envs/neutu-env/bin'
fi

run_script=$bindir/neutu
touch $run_script
echo '#!/bin/bash' > $run_script
#echo "source $condaDir/bin/activate $envName" >> $run_script
echo $neutu_bin_dir'/neutu $*' >> $run_script
chmod a+x $run_script
