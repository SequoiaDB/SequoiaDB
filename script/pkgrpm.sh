#!/bin/bash

###########################################
# script parameter description:
# $1: source files's path
###########################################

function err_exit()
{
   if [ $1 -ne 0 ]; then
      exit 1;
   fi
}

function get_macro_val_frm_line()
{
   if [[ "def$2" != def#define ]]; then
      return ;
   fi
   if [[ "def$1" != "def$3" ]]; then
      return ;
   fi
   echo "$4"
}

##################################
# function parameter description:
# $1: file name
# $2: macro name
##################################
function get_macro_val()
{
   val=$2
   while read LINE
   do
      rs=`get_macro_val_frm_line $val $LINE`
      if [[ -n "$rs" ]]; then
         val=$rs;
         break;
      fi
   done < $1
   if [[ "def$2" != "def$val" ]];then
      val=`get_macro_val $1 $val`
   fi
   echo "$val"
}

################################## begin #####################################

########################
# define variable
########################
files_src_path=$1
edition_type=$2 

script_path=""
dir_name=`dirname $0`
if [[ ${dir_name:0:1} != "/" ]]; then
   script_path=$(pwd)/$dir_name  #relative path
else
   script_path=$dir_name         #absolute path
fi

root_path="$script_path/.."

pkg_path="$root_path/package"
pkg_tmp_path="$pkg_path/tmp"
pkg_conf_file="$pkg_path/conf/rpm/sequoiadb.spec"

mkdir -p $pkg_tmp_path

########################
# get version
########################
ver_name="SDB_ENGINE_VERISON_CURRENT"
subver_name="SDB_ENGINE_SUBVERSION_CURRENT"
ver_info_file=$root_path/SequoiaDB/engine/include/ossVer.h
begin_ver=`get_macro_val $ver_info_file $ver_name`
sub_ver=`get_macro_val $ver_info_file $subver_name`
rls=`$script_path/../bin/sequoiadb --version | grep Release | awk '{print $2}'`

if [ "$edition_type" == "enterprise" ]
then
   sdb_name="sequoiadb-$begin_ver.$sub_ver.$edition_type"   
else
   sdb_name="sequoiadb-$begin_ver.$sub_ver"
fi

########################
# compress source file
########################
echo "compress the source files"

pkg_src_path="$pkg_tmp_path/$sdb_name"
mkdir -p "$pkg_src_path"
cp -rf $files_src_path/* $pkg_src_path
err_exit $?
tar -czf $pkg_tmp_path/$sdb_name.tar.gz -C $pkg_tmp_path $sdb_name --remove-files
err_exit $?

########################
# prepare the spec file
########################
echo "prepare the spec file"
mkdir -p $pkg_tmp_path/rpm/SPECS
cp -f $pkg_conf_file $pkg_tmp_path/rpm/SPECS
sed -i "s/SEQUOIADB_VERISON/$begin_ver/g"  $pkg_tmp_path/rpm/SPECS/sequoiadb.spec
sed -i "s/SEQUOIADB_SUBVERSION/$sub_ver/g" $pkg_tmp_path/rpm/SPECS/sequoiadb.spec
sed -i "s/SEQUOIADB_RELEASE/$rls/g"        $pkg_tmp_path/rpm/SPECS/sequoiadb.spec
if [ $edition_type == "enterprise" ]
then
   sed -i "s/SEQUOIADB_EDITION/.$edition_type/g" $pkg_tmp_path/rpm/SPECS/sequoiadb.spec
else
   sed -i "s/SEQUOIADB_EDITION//g" $pkg_tmp_path/rpm/SPECS/sequoiadb.spec   
fi 

########################
# generate rpm 
########################
echo "build the RPM package"
mkdir -p $pkg_tmp_path/rpm/SOURCES
mkdir -p $pkg_tmp_path/rpm/BUILD
mkdir -p $pkg_tmp_path/rpm/BUILDROOT
mkdir -p $pkg_tmp_path/rpm/RPMS
mkdir -p $pkg_tmp_path/rpm/SRPMS

rm -rf $pkg_tmp_path/rpm/SOURCES/*
mv $pkg_tmp_path/$sdb_name.tar.gz $pkg_tmp_path/rpm/SOURCES

rpmbuild --rmsource --define "_topdir $pkg_tmp_path/rpm" -bb $pkg_tmp_path/rpm/SPECS/sequoiadb.spec
err_exit $?

pkg_output_path="$pkg_path/output"
mkdir -p $pkg_output_path

rm -rf $pkg_output_path/RPMS
mv $pkg_tmp_path/rpm/RPMS $pkg_output_path
rm -rf $pkg_tmp_path/rpm
