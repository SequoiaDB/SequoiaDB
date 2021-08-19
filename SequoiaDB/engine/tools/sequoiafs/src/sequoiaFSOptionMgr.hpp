/*******************************************************************************


   Copyright ( C ) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY ; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sequoiaFSOptionMgr.hpp

   Descriptive Name = sequoiafs options manager.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
        03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef _SEQUOIAFS_OPTIONMGR_HPP_
#define _SEQUOIAFS_OPTIONMGR_HPP_

#include "pmdOptionsMgr.hpp"
#include "utilStr.hpp"

#define SDB_SEQUOIAFS_EXE_FILE_NAME    "sequoiafs"
#define SDB_SEQUOIAFS_CFG_FILE_NAME    SDB_SEQUOIAFS_EXE_FILE_NAME".conf"
#define SDB_SEQUOIAFS_LOG_FILE_NAME    SDB_SEQUOIAFS_EXE_FILE_NAME".log"
#define SDB_SEQUOIAFS_PID_FILE_NAME    SDB_SEQUOIAFS_EXE_FILE_NAME".pid"

#define SDB_SEQUOIAFS_HELP            "help"
#define SDB_SEQUOIAFS_HELP_FUSE       "helpfuse"
#define SDB_SEQUOIAFS_VERSION         "version"
#define SDB_SEQUOIAFS_HOSTS           "hosts"
#define SDB_SEQUOIAFS_USERNAME        "username"
#define SDB_SEQUOIAFS_PASSWD          "passwd"
#define SDB_SEQUOIAFS_COLLECTION      "collection"
#define SDB_SEQUOIAFS_META_DIR_CL     "metadircollection"
#define SDB_SEQUOIAFS_META_FILE_CL    "metafilecollection"
#define SDB_SEQUOIAFS_CONNECTION_NUM  "connectionnum"
#define SDB_SEQUOIAFS_CACHE_SIZE      "cachesize"
#define SDB_SEQUOIAFS_CONF_PATH       "confpath"
#define SDB_SEQUOIAFS_AUTOCREATE      "autocreate"
#define SDB_SEQUOIAFS_DIAGLEVEL       "diaglevel"
#define SDB_SEQUOIAFS_DIAGPATH        "diagpath"
#define SDB_SEQUOIAFS_DIAGNUM         "diagnum"
#define SDB_SEQUOIAFS_REPLSIZE        "replsize"
#define SDB_SEQUOIAFS_MOUNTPOINT      "mountpoint"
#define SDB_SEQUOIAFS_ALIAS           "alias"
#define SDB_SEQUOIAFS_ALLOWOTHER      "fuse_allow_other"
#define SDB_SEQUOIAFS_BIGWRITES       "fuse_big_writes"
#define SDB_SEQUOIAFS_MAXWRITE        "fuse_max_write"
#define SDB_SEQUOIAFS_MAXREAD         "fuse_max_read"
#define SDB_SEQUOIAFS_LARGEREAD       "fuse_large_read"

#define SDB_SEQUOIAFS_FUSE_PREFIX          "fuse_"
#define SDB_SEQUOIAFS_FUSE_EQUAL           "="
#define SDB_SEQUOIAFS_FUSE_ARG             "-o"

#define SDB_SEQUOIAFS_CONNECTION_DEFAULT_MAX_NUM 100
#define SDB_SEQUOIAFS_CACHE_DEFAULT_SIZE 2
#define SDB_SEQUOIAFS_HOSTS_DEFAULT_VALUE "localhost:11810"
#define SDB_SEQUOIAFS_USER_DEFAULT_NAME "sdbadmin"
#define SDB_SEQUOIAFS_USER_DEFAULT_PASSWD "sdbadmin"
#define SDB_SEQUOIAFS_REPLSIZE_DEFAULT_VALUE 2
#define SDB_SEQUOIAFS_MAXWRITE_DEFAULT_VALUE 131072
#define SDB_SEQUOIAFS_MAXREAD_DEFAULT_VALUE 131072 

const string SEQUOIAFS_META_CS = "sequoiafs" ;
const string SEQUOIAFS_META_DIR_SUFFIX = "_FS_SYS_DirMeta" ;
const string SEQUOIAFS_META_FILE_SUFFIX = "_FS_SYS_FileMeta" ;

namespace sequoiafs
{
   class _sequoiafsOptionMgr : public engine::_pmdCfgRecord
   {
      public:
         _sequoiafsOptionMgr() ;
         virtual ~_sequoiafsOptionMgr(){}

         INT32 init( INT32 argc, CHAR **argv, vector<string> *options4fuse ) ;
         INT32 save() ;
         void setSvcName( const CHAR *svcName ) ;
         PDLEVEL getDiaglogLevel()const ;
         const CHAR *getCfgFileName()const{return _cfgFileName ;}
         const CHAR *getHosts()const{return _hosts ;}
         const CHAR *getUserName()const{return _userName ;}
         const CHAR *getPasswd()const{return _passwd ;}
         const INT32 getConnNum()const{return _connectionNum ;}
         const INT32 getDiagMaxNUm()const{return _diagnum ;}
         const CHAR *getCollection()const{return _collection ;}
         const CHAR *getMetaFileCL()const{return _metaFileCollection ;}
         const CHAR *getMetaDirCL()const{return _metaDirCollection ;}
         const INT32 getCacheSize()const{return _cacheSize ;}
         const INT32 replsize()const{return _replsize ;}
         const CHAR *getCfgPath()const{return _cfgPath ;}
         const CHAR *getmountpoint()const{return _mountpoint ;}
         const CHAR *getAlias()const{return _alias;}
         CHAR *getDiaglogPath(){return _diagPath ;}
         INT32 parseCollection( const string collection, string *cs, string *cl ) ;
         INT32 parseAliasName();


      protected:
         virtual INT32 doDataExchange( engine::pmdCfgExchange *pEX ) ;
         virtual INT32 postLoaded(  engine::PMD_CFG_STEP step  ) ;

      private:
         CHAR _hosts[OSS_MAX_PATHSIZE + 1] ;
         CHAR _userName[OSS_MAX_PATHSIZE + 1] ;
         CHAR _passwd[OSS_MAX_PATHSIZE + 1] ;
         CHAR _collection[OSS_MAX_PATHSIZE + 1] ;
         CHAR _mountpoint[OSS_MAX_PATHSIZE + 1] ;
         CHAR _alias[OSS_MAX_PATHSIZE + 1] ;
         CHAR _metaFileCollection[OSS_MAX_PATHSIZE + 1] ;
         CHAR _metaDirCollection[OSS_MAX_PATHSIZE + 1] ;
         INT32 _connectionNum ;
         INT32 _cacheSize ;
         CHAR _cfgPath[OSS_MAX_PATHSIZE + 1] ;
         CHAR _cfgFileName[OSS_MAX_PATHSIZE + 1] ;
         CHAR _diagPath[OSS_MAX_PATHSIZE + 1] ;
         INT32 _diagnum ;
         UINT16 _diagLevel ;
         INT32 _replsize ;
         BOOLEAN _fuse_allow_other;
         BOOLEAN _fuse_big_writes;
         INT32 _fuse_max_write;
         INT32 _fuse_max_read;
         BOOLEAN _fuse_large_read;
   } ;
   typedef _sequoiafsOptionMgr sequoiafsOptionMgr ;
}

#endif