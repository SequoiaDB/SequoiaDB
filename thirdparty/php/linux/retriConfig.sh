#!/bin/bash
SCRIPT=$(readlink -f "$0")
basedir=`dirname "$SCRIPT"`
i=0;
if test -d $basedir/php-$1
then
	if test -f $basedir/forlinux/main/build-defs.h
	then
	   cp $basedir/forlinux/main/build-defs.h $basedir/php-$1/main/build-defs.h
	   let i++;
	fi

   if test -f $basedir/forlinux/main/php_config.h
   then
      cp $basedir/forlinux/main/php_config.h $basedir/php-$1/main/php_config.h
      let i++;
   fi

	
	if test -f $basedir/forlinux/TSRM/tsrm_config.h
	then
	   cp $basedir/forlinux/TSRM/tsrm_config.h $basedir/php-$1/TSRM/tsrm_config.h
	   let i++;
	fi
	
	if test -f $basedir/forlinux/Zend/zend_config.h
	then
	   cp $basedir/forlinux/Zend/zend_config.h $basedir/php-$1/Zend/zend_config.h
	   let i++;
	fi

else
   echo "$basedir/php-$1 is not a true director"
   exit 1 ;
fi


if [ $i -eq 4 ]
then
   echo OK;
   exit 0 ;
fi
