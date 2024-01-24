# The autogen will replace the following 4 values.
DB_VERSION="x.x.x"
RELEASE="0"
GIT_VERSION="xxxxxxxxxx"
BUILD_TIME="0000-00-00-00.00.00"
    
def get_version():
    return DB_VERSION
    
def get_release():
    return RELEASE
    
def get_git_version():
    return GIT_VERSION
    
def get_build_time():
    return BUILD_TIME
