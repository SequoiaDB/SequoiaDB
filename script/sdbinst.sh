#!/bin/sh

CHMD="chmod"
COPY="cp"
REMV="rm -f"
LINK="ln -sf"
MKDR="mkdir -p"
RMDR="rmdir -p"
BASE="basename"
DIRN="dirname"

SOURCE_DIR="."

VERBOSE=YES
NAME="sequoiadb"
###############################################################################

Usage () {

    echo ""
    echo "Usage: sdbinst [-u <user name>] [-p <password>]                     "
    echo "       [-h <home directory>] [-d <install directory>]               "
    echo ""
    echo "user name      : database admin user name.                          "
    echo "                (this option defaults to $USER_NANE)                "
    echo "password       : database admin user's password                     "
    echo "                (this option defaults to $USER_NAME)                "
    echo "home directory : Path where user's home direcotry                   "
    echo "                (this option defaults to /home/$USER_NAME)          "
    echo "install directory : Path where software install                     "
    echo "                   (this option defaults to $INSTALL_DIR)           "
    exit 1
}

###############################################################################

Exec () {

    # 1 = The full command to be executed

    [ "$VERBOSE" = "YES"   ] && echo "Executing $1 ..."
    [ "$PREVIEW" = "YES"   ] && return 0
    `$1 >/dev/null 2>&1`     && return 0

    echo "Waring: $1 failed ..."

    return 1

}

###############################################################################


CreateUninstall () {

    # No Parameters

    [ "$VERBOSE" = "YES"   ] && echo "Creating uninstall file ..."
    #[ "$PREVIEW" = "YES"   ] && return 0

    #[ ! -d $INSTALL_DIR    ]           && \
    #Exec "$MKDR $install_PATH"          && \
    #Exec "$CHMD  $TARGET_PATH"


    echo "#!/bin/sh" >  $UNINST_FILE
    echo ""          >> $UNINST_FILE
    echo "# This is automatically generated by sdbinst" >> $UNINST_FILE

    echo "service $NAME stop"                           >> $UNINST_FILE
 
    echo "$REMV /etc/rc5.d/S99sequoiadb >/dev/null 2>&1"  >> $UNINST_FILE
    echo "$REMV /etc/rc3.d/S99sequoiadb >/dev/null 2>&1"  >> $UNINST_FILE
    echo "$REMV /etc/rc4.d/S99sequoiadb >/dev/null 2>&1"  >> $UNINST_FILE
    echo "$REMV /etc/init.d/sequoiadb >/dev/null 2>&1"  >> $UNINST_FILE
    echo "$REMV $SERVICE_CONF_DIR/$SERVICE_CONF_FILE >/dev/null 2>&1" >> $UNINST_FILE
    
 	  if [ "$ADD_USER" == "true" ]; then
 	      echo "userdel $USER_NAME >/dev/null 2>&1" >> $UNINST_FILE
 	      echo "groupdel $USER_NAME >/dev/null 2>&1" >> $UNINST_FILE
 	  fi
 	  
    echo "$REMV -r $INSTALL_DIR/sequoiadb >/dev/null 2>&1" >> $UNINST_FILE
}

###############################################################################

Bail () {

    # 1 = The message

    echo "sdbinst: $@"
    echo "Installation unsuccessful"
    exit 1

}


###############################################################################

ChangeOwner() {
   
    Exec "$CHMD u+x $INSTALL_DIR/sequoiadb/bin/*"
    Exec "$CHMD u+x $UNINST_FILE"
    Exec "chown -R $USER_NAME:$USER_NAME $INSTALL_DIR/sequoiadb"
}

###############################################################################
AddAdminUser() {
    
    echo "Begin to add user for database admin"
    USER_NAME_EXIST=`grep $USER_NAME /etc/passwd | awk -F: '{print $1}'`

    if [ "$USER_NAME_EXIST" = "$USER_NAME" ]; then
        echo "Waring: User($USER_NAME) is exist already, ignored password and home direcotry."
    else
    		Exec "groupadd $USER_NAME"
        Exec "useradd -s /bin/bash -d $HOME_DIR -m -g $USER_NAME $USER_NAME"
        ADD_USER="true"
    fi

}


###############################################################################

RegistService() {

    Exec "$MKDR $SERVICE_CONF_DIR"
    Exec "$REMV  $SERVICE_CONF_DIR/$SERVICE_CONF_FILE"

    echo "# This is automatically generated by sdbinst" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "# defaults for sequoiadb data base server" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "# Service name" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "NAME=sequoiadb" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "# database administrator name" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "SDBADMIN_USER=$USER_NAME" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "# sequoiadb install location" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE
    echo "INSTALL_DIR=$INSTALL_DIR/sequoiadb" >> $SERVICE_CONF_DIR/$SERVICE_CONF_FILE

    
    Exec "$COPY $SOURCE_DIR/sequoiadb /etc/init.d/"
    Exec "$CHMD u+x /etc/init.d/sequoiadb"
    Exec "$LINK /etc/init.d/sequoiadb /etc/rc5.d/S99sequoiadb"
    Exec "$LINK /etc/init.d/sequoiadb /etc/rc3.d/S99sequoiadb"
    Exec "$LINK /etc/init.d/sequoiadb /etc/rc4.d/S99sequoiadb"
}

###############################################################################

DoInstall () {

    Exec "$MKDR $INSTALL_DIR/sequoiadb"
    Exec "$COPY -r $SOURCE_DIR/* $INSTALL_DIR/sequoiadb"
}

USER_NAME=sdbadmin
INSTALL_DIR="/opt"
UNINST_FILE="sdbuninst"
SERVICE_CONF_DIR="/etc/default"
SERVICE_CONF_FILE="sequoiadb"
ADD_USER="false"

while [ $# -gt 0 ]
do
    case "$1" in
        -user|-u)
            [ -z "$2" ] && Bail "-u option needs an argument, database admin user name"
            USER_NAME="$2"
            shift;;
        -password|-p)
            [ -z "$2" ] && Bail "-p option needs an argument, database admin user's password"
            PASSWORD=$2
            shift;;
        -home|-h)
            [ -z "$2" ] && Bail "-h option needs an argument, user's home directory"
            HOME_DIR="$2"
            shift;;
        -direcotry|-d)
            [ -z "$2" ] && Bail "-d option needs an argument, software install directory"
            INSTALL_DIR="$2"
            shift;;
        *)
            Usage
            ;;
    esac
    shift
done


###############################################################################
# check that all required conditions are met or bail with error message
###############################################################################

[ -z "$HOME_DIR" ] &&\
HOME_DIR=/home/$USER_NAME

[ -z "$PASSWORD" ] &&\
PASSWORD=$USER_NAME


###############################################################################

[ "$SOURCE_DIR" = "$INSTALL_PATH" ] && \
echo "The sequoiadb tar.gz installer file should be untarred into a "    && \
echo "temporary directory from which you run the sdbinst command." && \
echo "This temporary directory (the source directory) cannot be"    && \
echo "the same as the target directory ($INSTALL_PATH)."             && \
Bail "Source and target directories are the same"


PWD=`pwd`

[ "$SOURCE_DIR" = "." ] &&\
[ "$PWD" = "$TARGET_PATH" ] && \
echo "The sequoiadb tar.gz installer file should be untarred into a "    && \
echo "temporary directory from which you run the sdbinst command." && \
echo "This temporary directory (the source directory) cannot be"    && \
echo "the same as the target directory ($INSTALL_PATH)."             && \
Bail "Source and target directories are the same."


UNINST_FILE="$INSTALL_DIR/sequoiadb/$UNINST_FILE"

echo "user=$USER_NAME; password=$PASSWORD; home_dir=$HOME_DIR; install_dir=$INSTALL_DIR"
###############################################################################



AddAdminUser && DoInstall && CreateUninstall && ChangeOwner &&RegistService

service sequoiadb start

