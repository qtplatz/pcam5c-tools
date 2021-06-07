#!/bin/bash

arch=`uname`-`arch`
cwd=`pwd`
host_system=`uname`
source ./config.sh

source_dir=("$cwd")
build_debug=false
build_clean=false
build_root=$(dirname "$cwd")
config=release
cmake_args=('-DCMAKE_BUILD_TYPE=Release')

while [ $# -gt 0 ]; do
    case "$1" in
	debug)
	    build_debug=true
	    config=debug
		cmake_args=('-DCMAKE_BUILD_TYPE=Debug')
	    shift
	    ;;
	clean)
	    shift
	    build_clean=true
	    ;;
	*)
	    break
	    ;;
    esac
done

if [ -z $cross_target ]; then
	build_dir=( "$build_root/build-$arch/pcam5c-tools.$config" )
else
	build_dir=( "$build_root/build-$cross_target/pcam5c-tools.$config" )
	CMAKE_TOOLCHAIN_FILE=${cwd}/toolchain-arm-linux-gnueabihf.cmake
fi

if [ $build_clean = true ]; then
    echo rm -rf $build_dir
    rm -rf $build_dir
    exit
fi


echo "source_dir       : ${source_dir}"
echo "build_dir        : ${build_dir}"
echo "cross_target     :" $cross_target
echo "toolchain        :" ${CMAKE_TOOLCHAIN_FILE}

# =============== change directory ==============
#echo "#" mkdir -p $build_dir
#echo "#" cd $build_dir
#echo "cd ${build_dir}; cmake ${source_dir}"

mkdir -p $build_dir
cd $build_dir
echo "#" pwd `pwd`

if [ $cross_target ]; then
    echo "## Cross build for $arch"
    case $cross_target in
		armhf|arm-linux-gnueabihf)
			cmake "${cmake_args[@]}" -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} $source_dir
			;;
		*)
			echo "Unknown cross_target: $cross_target"
			;;
    esac
else
	cmake "${cmake_args[@]}" $source_dir
fi
