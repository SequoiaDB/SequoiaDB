#!/bin/bash
####################################################################
# Decription:
#   Generate a VERSION file in upgrade when old version DB does not have one.
####################################################################

function getRootPath()
{
   confFile="/etc/default/sequoiadb"
   if [ -f $confFile ]; then
      source $confFile
   else
      echo "ERROR: File $confFile not exist!"
      exit 1
   fi
}

function generateVersionFile()
{
   filePath="$INSTALL_DIR/VERSION"
   sdbPath="$INSTALL_DIR/bin/sequoiadb"
   sdbVersion=`$sdbPath --version | grep -v "Git" | grep "version"`
   release=`$sdbPath --version | grep "Release"`
   gitVersion=`$sdbPath --version | grep "Git version"`
   buildTime=`$sdbPath --version | grep -v "version" | grep -v "Release"`
   echo $sdbVersion >> $filePath
   echo $release >> $filePath
   echo $gitVersion >> $filePath
   echo $buildTime >> $filePath
}

function main()
{
   getRootPath
   if [ -f "$INSTALL_DIR/VERSION" ]; then
      exit 0
   else
      generateVersionFile
   fi
}

main