#!/bin/bash
INSTALL_DIR=/opt/test_common

# Temporary dir , used for put the file
SOFTWARE_FILE_DIR=$INSTALL_DIR"/Sequoiadb_files"

SDB_INSTALL_DIR=/opt/sequoiadb

# All data node's root dir 
DATABASE_DIR=/opt/sequoiadb/database/data

# Software file's name
INSTALL_SOFTWARE_FILE=sequoiadb-1.2.3-linux_x86_64-installer.run

# Sdb client's dir
SDB_CMD=$SDB_INSTALL_DIR/bin/sdb

# Machine's menory
machine_memory=32
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SDB_INSTALL_DIR/lib

# Will build the Groups list
GROUP_LIST="1 2 3 4 5"

# All the hostName
HOST1=ubun-test2
HOST2=ubun-test3
HOST3=ubun-test4
HOST4=SQUDBM02
HOST5=SQUDBD03
HOST6=SQUDBD04
#HOST2=power_rhel_2

#data node bast port , then it will add 10 for every circle
dataNodeBasePort=51010

# Host list 
HOSTLIST1="$HOST1 $HOST2 $HOST3"
HOSTLIST2="$HOST4 $HOST5 $HOST6"
HOSTLIST="$HOST1 $HOST2 $HOST3 $HOST4 $HOST5 $HOST6"

# How much machine is it
HOST_COUNT=${#HOSTLIST[*]}


# All the disk
DISK1=
DISK2=
DISK3=
DISK4=
DISK5=

# Disk list
DISKLIST="$DISK1 $DISK2 $DISK3 $DISK4 $DISK5"
# How much disk in one machine
DISK_COUNT=${#DISKLIST[*]}

# When connect the Sequoiadb , use which hostname to connect
SDB_HOST_NAME=${HOST1}

GROUP_COUNT_PER_HOST=1

START_TIME="2013-05-01 00:00:00"
END_TIME="2013-05-31 23:59:59"


