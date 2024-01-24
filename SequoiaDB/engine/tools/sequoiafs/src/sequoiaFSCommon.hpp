#ifndef __SEQUOIAFSCOMMON_HPP__
#define __SEQUOIAFSCOMMON_HPP__

#include "ossTypes.hpp"
#include "ossUtil.hpp"
#include "ossProc.hpp"
#include "utilStr.hpp"
#include "ossIO.hpp"
#include "pd.hpp"
#include "msg.h"

#include "sdbConnectionPool.hpp"

using namespace bson;

#define FS_MAX_NAMESIZE   NAME_MAX

#define INVALID_UID_GID 0xFFFFFFFF

#define LISTEN_PORT  "11742"
#define ROOT_ID 1

#define SEQUOIAFS_CS "sequoiafs"
#define SEQUOIAFS_MOUNTID_CL "mountid"
#define SEQUOIAFS_MOUNTID_FULLCL SEQUOIAFS_CS "." SEQUOIAFS_MOUNTID_CL
#define SEQUOIADB_SERVICE_CS "sequoiadb"
#define SEQUOIADB_SERVICE_CL "serviceinfo"
#define SEQUOIADB_SERVICE_FULLCL SEQUOIADB_SERVICE_CS "." SEQUOIADB_SERVICE_CL 
#define SEQUOIADB_SERVICE_INDEX "serviceIdx"

//serviceinfo table
#define SERVICE_NAME       "ServiceName"
#define SERVICE_HOSTNAME   "HostName"
#define SERVICE_PORT       "Port"
#define SERVICE_TIMEOUT    "TimeOut"


#define SERVICE_NAME_MCS "MCS"

//mountcollection table
#define FS_MOUNT_CL      "MountCL"
#define FS_MOUNT_PATH    "MountPath"
#define FS_MOUNT_ID      "MountId"

//file and dir meta table
#define SEQUOIAFS_NAME          "Name"
#define SEQUOIAFS_MODE          "Mode"
#define SEQUOIAFS_UID           "Uid"
#define SEQUOIAFS_GID           "Gid"
#define SEQUOIAFS_NLINK         "NLink"
#define SEQUOIAFS_PID           "Pid"
#define SEQUOIAFS_ID            "Id"
#define SEQUOIAFS_LOBOID        "LobOid"
#define SEQUOIAFS_SIZE          "Size"
#define SEQUOIAFS_CREATE_TIME      "CreateTime"
#define SEQUOIAFS_MODIFY_TIME      "ModifyTime"
#define SEQUOIAFS_ACCESS_TIME      "AccessTime"
#define SEQUOIAFS_SYMLINK          "SymLink"


//MSG type
#define FS_REGISTER_REQ     10001                //SequoiaFS send req
#define FS_REGISTER_RSP     MAKE_REPLY_TYPE(FS_REGISTER_REQ)    // MCS send rsp
#define FS_RELEASELOCK      10007                //SequoiaFS send notify to MCS
#define MS_RELEASELOCK      10009                //MCS send notify to SquoiaFS

namespace sequoiafs
{ 
   struct _mcsRegReq
   {
      MsgHeader header;
      INT32 mountId;        // mountpoint id
      CHAR  mountIp[OSS_MAX_IP_ADDR + 1];
      INT32 pathLen;        //mountpoint len
      CHAR mountPath[0];    //mountpoint
   };

   struct _mcsRegRsp
   {
      MsgInternalReplyHeader reply;
   };

   struct _mcsNotifyReq
   {
      MsgHeader header;
      INT32 mountId;        // mountpoint ID
      INT64 parentId;
      INT32 nameLen;        //dirName len
      CHAR dirName[0]; 
   };

   struct lobHandle
   {
      OID oid;
      BOOLEAN isDirty;
      BOOLEAN isCreate;
      INT32 status;
      INT32 flId;
      INT64 parentId;
      CHAR fileName[FS_MAX_NAMESIZE + 1];
   };

   class _metaNode : public SDBObject
   {
      public:
         _metaNode()
         {
            *_name = '\0';//set first byte to '\0' to save time.
            _mode = 0;
            _uid = 0;
            _gid = 0;
            _nLink = 0;
            _pid = 0;
            _size= 0;
            _ctime = 0;
            _mtime = 0;
            _atime = 0;
            *_symLink = '\0';
         }
         _metaNode(const class _metaNode &node)
         {
            int len = 0;
            _mode = node._mode;
            _uid = node._uid;
            _gid = node._gid;
            _nLink = node._nLink;
            _pid = node._pid;
            _size= node._size;
            _ctime = node._ctime;
            _mtime = node._mtime;
            _atime = node._atime;
            ossStrncpy(_name, node._name, ossStrlen(node._name));
            len = ossStrlen(node._name) < (sizeof(_name) - 1) ?
                  ossStrlen(node._name) : (sizeof(_name) - 1);
            _name[len] = '\0';
            ossStrncpy(_symLink, node._symLink, ossStrlen(node._symLink));
            len = ossStrlen(node._symLink) < (sizeof(_symLink) - 1) ?
                  ossStrlen(node._symLink) : (sizeof(_symLink) - 1);
            _symLink[len] = '\0';
         }
         ~_metaNode(){}
         const CHAR* name() const { return _name; }
         const UINT32 mode() const { return _mode; }
         const UINT32 uid() const { return _uid; }
         const UINT32 gid() const { return _gid; }
         const UINT32 nLink() const { return _nLink; }
         const INT64 pid() const { return _pid; }
         const INT64 size() const { return _size; }
         const INT64 ctime() const { return _ctime; }
         const INT64 mtime() const { return _mtime; }
         const INT64 atime() const { return _atime; }
         const CHAR* symLink() const { return _symLink; }

         void setName( const CHAR *n )
         {
            int len = 0;
            if(n != NULL)
            {
               ossStrncpy(_name, n, ossStrlen(n));
               len = strlen(n) < (sizeof(_name) - 1) ?
                     strlen(n) : (sizeof(_name) - 1);
            }
            _name[len] = '\0';
         }
         void setMode( UINT32 m ) { _mode = m ; }
         void setUid( UINT32 u ) { _uid = u; }
         void setGid( UINT32 g ) { _gid = g; }
         void setNLink( UINT32 n ) { _nLink = n; }
         void setPid( INT64 p ) { _pid = p; }
         void setSize( INT64 s ) { _size = s ; }
         void setCtime( INT64 time ) { _ctime = time; }
         void setMtime( INT64 time ) { _mtime  = time ; }
         void setAtime( INT64 time ) { _atime = time ; }
         void setSymLink( const CHAR* sym )
         {
            int len = 0;
            if ( NULL != sym )
            {
               ossStrncpy(_symLink, sym, ossStrlen(sym));
               len = ossStrlen(sym) < (sizeof(_symLink) - 1) ?
                     ossStrlen(sym) : (sizeof(_symLink) - 1);
            }
            _symLink[len] = '\0';
         }

      public:
         CHAR _name[FS_MAX_NAMESIZE + 1];
         UINT32 _mode; //umode_t(u_shourt)
         UINT32 _uid;
         UINT32 _gid;
         UINT32 _nLink;
         INT64 _pid;
         INT64 _size;  //off_t long long
         INT64 _ctime;
         INT64 _mtime;
         INT64 _atime;
         CHAR _symLink[OSS_MAX_PATHSIZE + 1];
   };
   typedef class _metaNode metaNode;

   class _fileMeta : public metaNode
   {
      public:
         _fileMeta()
         {
            *_lobOid = '\0';
         }
         _fileMeta(const class _fileMeta &node)
         {
            int len = 0;
            ossStrncpy(_lobOid, node._lobOid, ossStrlen(node._lobOid));
            len = ossStrlen(node._lobOid) < (sizeof(_lobOid) - 1) ?
                  ossStrlen(node._lobOid) : (sizeof(_lobOid) - 1);
            _lobOid[len] = '\0';
         }

         ~_fileMeta(){}
         const CHAR *lobOid() const { return _lobOid; }
         void setLobOid( const CHAR* oid )
         {
            int len = 0;
            ossStrncpy(_lobOid, oid, sizeof(_lobOid));
            len = ossStrlen(oid) < (sizeof(_lobOid) - 1) ?
                  ossStrlen(oid) : (sizeof(_lobOid) - 1);
            _lobOid[len] = '\0';
         }

         void operator=(const _fileMeta &m)
         {
            setName(m.name());
            setMode(m.mode());
            setUid(m.uid());
            setGid(m.gid());
            setNLink(m.nLink());
            setPid(m.pid());
            setLobOid(m.lobOid());
            setSize(m.size());
            setCtime(m.ctime());
            setMtime(m.mtime());
            setAtime(m.atime());
            setSymLink(m.symLink());
         }
      public:
         CHAR _lobOid[30];
   };
   typedef class _fileMeta fileMeta;

   class _dirMeta : public metaNode
   {
      public:
         _dirMeta()
         {
            _id = 0;
         }
         _dirMeta(const class _dirMeta &node)
         {
            _id = node._id;
         }
         ~_dirMeta(){}
         const INT64 id() const { return _id;}
         void setId( INT64 d ) { _id = d; }

         void operator=(const _dirMeta &m)
         {
            setName(m.name());
            setMode(m.mode());
            setUid(m.uid());
            setGid(m.gid());
            setNLink(m.nLink());
            setPid(m.pid());
            setId(m.id());
            setSize(m.size());
            setCtime(m.ctime());
            setMtime(m.mtime());
            setAtime(m.atime());
            setSymLink(m.symLink());
         }
      public:
         INT64 _id;
   };
   typedef class _dirMeta dirMeta;

   INT64 min64(INT64 a, INT64 b);

   INT64 max64(INT64 a, INT64 b);

   INT32 buildDialogPath(CHAR *diaglogPath, CHAR *diaglogPathFromCmd,
                      UINT32 bufSize);
}

#endif
