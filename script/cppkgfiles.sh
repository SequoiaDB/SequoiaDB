#!/bin/bash

################################################
# description:
#     copy compile output, prepare for rpm pack
# $1: dest path(e.g: /trunk/package/tmp/sequoiadb)
# $2: build type(debug or release)
################################################

function copy_file()
{
   cp $1 $2
   if [ $? -ne 0 ]; then
      echo "ERROR: Failed to copy the file!"
      exit 1;
   fi
}

function copy_folder()
{
   rsync -vaq --exclude=".svn" $1 $2
   if [ $? -ne 0 ]; then
      echo "ERROR: Failed to copy the folder!"
      exit 1;
   fi
}

################################## begin #####################################
pkg_src_tmp=$1
build_type=$2

script_path=""
dir_name=`dirname $0`
if [[ ${dir_name:0:1} != "/" ]]; then
   script_path=$(pwd)/$dir_name  #relative path
else
   script_path=$dir_name         #absolute path
fi

mkdir -p $pkg_src_tmp
rm -rf $pkg_src_tmp/*

root_path="$script_path/.."

#########################################
# folder: bin
#########################################
echo "collect the files of \"bin\""
src_dir_bin="$root_path/bin"
dst_dir_bin="$pkg_src_tmp/bin"

mkdir -p $dst_dir_bin
copy_folder "$src_dir_bin/*"           "$dst_dir_bin"
copy_file   $root_path/script/sdbwsart $dst_dir_bin
copy_file   $root_path/script/sdbwstop $dst_dir_bin

#########################################
# folder: conf
#########################################
echo "collect the files of \"conf\""
mkdir -p $pkg_src_tmp/conf/local
mkdir -p $pkg_src_tmp/conf/log

src_dir_conf_smp="$root_path/conf/samples"
dst_dir_conf_smp="$pkg_src_tmp/conf/samples"
mkdir -p $dst_dir_conf_smp
copy_folder "$src_dir_conf_smp/*" "$dst_dir_conf_smp"

src_dir_conf_scp="$root_path/conf/script"
dst_dir_conf_scp="$pkg_src_tmp/conf/script"
mkdir -p $dst_dir_conf_scp
copy_folder "$src_dir_conf_scp/*" "$dst_dir_conf_scp"

copy_file $root_path/conf/sdbcm.conf $pkg_src_tmp/conf/

#########################################
# folder: java
#########################################
echo "collect the files of \"java\""
src_dir_java="$root_path/driver/java"
src_dir_jdk="$root_path/java"
dst_dir_java="$pkg_src_tmp/java"

mkdir -p $dst_dir_java
copy_file $src_dir_java/sequoiadb.jar $dst_dir_java

jdk_name=""
arch_info=`arch`
if [ "os_$arch_info" == "os_x86_64" ];then
   jdk_name="jdk_linux64"
elif [ "os_$arch_info" == "os_x86_32" ];then
   jdk_name="jdk_linux32"
elif [ "os_$arch_info" == "os_ppc64" ];then
   jdk_name="jdk_ppclinux64"
elif [ "os_$arch_info" == "os_ppc64le" ];then
   jdk_name="jdk_ppclelinux64"
else
   echo "The platform is not supported!"
   exit 1
fi

mkdir -p $dst_dir_java/jdk
copy_folder "$src_dir_jdk/$jdk_name/*" "$dst_dir_java/jdk"

#########################################
# folder: lib
#########################################
echo "collect the files of \"lib\""

# c cpp
src_dir_lib="$root_path/client/lib"
dst_dir_lib="$pkg_src_tmp/lib"
mkdir -p $dst_dir_lib
copy_folder "$src_dir_lib/*" "$dst_dir_lib"

# php
if [ "$build_type" == "debug" ];then
   build_dir="dd"
else
   build_dir="normal"
fi

src_dir_phplib="$root_path/driver/php5/build/$build_dir"
dst_dir_phplib="$src_dir_lib/phplib"
mkdir -p $dst_dir_phplib
copy_folder "$src_dir_phplib/*.so" "$dst_dir_phplib"

#########################################
# folder: include
#########################################
echo "collect the files of \"include\""
src_dir_inc="$root_path/client/include"
dst_dir_inc="$pkg_src_tmp/include"
mkdir -p $dst_dir_inc
copy_folder "$src_dir_inc/*" "$dst_dir_inc"

#########################################
# folder: postgresql
#########################################
echo "collect the files of \"postgresql\""
src_dir_pg="$root_path/driver/postgresql"
dst_dir_pg="$pkg_src_tmp/postgresql"

mkdir -p $dst_dir_pg
copy_file $src_dir_pg/sdb_fdw.so       $dst_dir_pg
copy_file $src_dir_pg/sdb_fdw.control  $dst_dir_pg
copy_file $src_dir_pg/sdb_fdw--1.0.sql $dst_dir_pg

#########################################
# folder: python
#########################################
echo "collect the files of \"python\""
src_dir_py="$root_path/driver/python"
dst_dir_py="$pkg_src_tmp/python"

mkdir -p $dst_dir_py
copy_file $src_dir_py/pysequoiadb.tar.gz $dst_dir_py

#########################################
# folder: CSharp
#########################################
echo "collect the files of \"CSharp\""
src_dir_csp="$root_path/driver/C#.Net/build/release"
dst_dir_csp="$pkg_src_tmp/CSharp"
mkdir -p $dst_dir_csp
copy_file $src_dir_csp/sequoiadb.dll $dst_dir_csp

#########################################
# folder: hadoop
#########################################
echo "collect the files of \"hadoop\""
src_dir_hdp="$root_path/driver/hadoop"
dst_dir_hdp="$pkg_src_tmp/hadoop"
mkdir -p $dst_dir_hdp

copy_file "$src_dir_hdp/hive/*.jar"             "$dst_dir_hdp"
copy_file "$src_dir_hdp/hadoop-connector/*.jar" "$dst_dir_hdp"

#########################################
# folder: license
#########################################
echo "collect the files of \"license\""
src_dir_lcs="$root_path/license"
dst_dir_lcs="$pkg_src_tmp/license"

mkdir -p $dst_dir_lcs
copy_folder "$src_dir_lcs/*" "$dst_dir_lcs"

#########################################
# folder: samples
#########################################
echo "collect the files of \"samples\""
src_dir_smp="$root_path/client/samples"
dst_dir_smp="$pkg_src_tmp/samples"
mkdir -p $dst_dir_smp
copy_folder "$src_dir_smp/*" "$dst_dir_smp"

#########################################
# folder: www
#########################################
echo "collect the files of \"www\""
src_dir_www="$root_path/client/admin/admintpl"
dst_dir_www="$pkg_src_tmp/www"
mkdir -p $dst_dir_www
copy_folder "$src_dir_www/*" "$dst_dir_www"

#########################################
# folder: web
#########################################
echo "collect the files of \"web\""
src_dir_web="$root_path/SequoiaDB/web"
dst_dir_web="$pkg_src_tmp/web"

mkdir -p $dst_dir_web
copy_folder "$src_dir_web/*" "$dst_dir_web"

#########################################
# folder: doc
#########################################
echo "collect the files of \"doc\""
src_dir_doc="$root_path/doc"
dst_dir_doc="$pkg_src_tmp/doc"

mkdir -p $dst_dir_doc

mkdir -p $dst_dir_doc/manual
copy_folder "$src_dir_doc/manual/*" "$dst_dir_doc/manual"

docVer=`$root_path/bin/sequoiadb --version | grep SequoiaDB | awk '{print $3}'`
doc_tar_name=sequoiadb-document-${docVer}-linux.tar.gz

staf bl465-4 FS COPY FILE /test_tar/${doc_tar_name} TODIRECTORY $dst_dir_doc
tar zxvf $dst_dir_doc/$doc_tar_name -C $dst_dir_doc
rm -f $dst_dir_doc/$doc_tar_name

#########################################
# folder: tools
#########################################
echo "collect the files of \"tools\""
src_dir_tol="$root_path/tools"
dst_dir_tol="$pkg_src_tmp/tools"
mkdir -p $dst_dir_tol

mkdir -p $dst_dir_tol/sdbsupport
copy_folder "$src_dir_tol/sdbsupport/*" "$dst_dir_tol/sdbsupport"

php_arch_path=""
if [ "os_$arch_info" == "os_x86_64" ];then
   php_arch_path="php_linux"
elif [ "os_$arch_info" == "os_x86_32" ];then
   php_arch_path="php_linux"
elif [ "os_$arch_info" == "os_ppc64" ];then
   php_arch_path="php_power"
elif [ "os_$arch_info" == "os_ppc64le" ];then
   php_arch_path="php_powerle"
else
   echo "The platform is not supported!"
   exit 1
fi

mkdir -p $dst_dir_tol/server/php
copy_folder "$src_dir_tol/server/$php_arch_path/*" "$dst_dir_tol/server/php"

#########################################
# folder: packet
#########################################
echo "collect the files of \"packet\""
mkdir -p $pkg_src_tmp/packet

#########################################
# other files which are not in folder
#########################################
echo "collect other files"

copy_file $root_path/script/sequoiadb                $pkg_src_tmp
copy_file $root_path/script/install_om.sh            $pkg_src_tmp
copy_file $root_path/misc/ci/mkrelease/compatible.sh $pkg_src_tmp
copy_file $root_path/misc/ci/mkrelease/version.conf  $pkg_src_tmp

echo "collect files complete"