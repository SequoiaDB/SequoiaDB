#!/bin/bash
. ./sequoiadbconfig.sh
. ./sequoiadbfun1.sh
. ./sequoiadbfun2.sh

sshUninstallSDB
if [ $? -ne 0 ]; then
   exit 1
fi

exit 0
