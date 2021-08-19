/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sptUsrSystemCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/18/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRSYSTEM_COMMON_HPP_
#define SPT_USRSYSTEM_COMMON_HPP_
#include <vector>
#include <string>
#include "../bson/bson.hpp"
using std::string ;
using std::vector ;
namespace engine
{

#if defined (_LINUX)
   struct _cpuInfo
   {
      string modelName ;
      string coreNum ;
      string freq ;
      string physicalID ;
      void reset()
      {
         modelName  = "" ;
         coreNum    = "1" ;
         freq       = "" ;
         physicalID = "0" ;
      }
   } ;
   typedef struct _cpuInfo cpuInfo ;

   #define HOSTS_FILE      "/etc/hosts"
#else
   #define HOSTS_FILE      "C:\\Windows\\System32\\drivers\\etc\\hosts"
#endif // _LINUX

   #define CMD_USR_SYSTEM_DISTRIBUTOR        "Distributor"
   #define CMD_USR_SYSTEM_RELASE             "Release"
   #define CMD_USR_SYSTEM_DESP               "Description"
   #define CMD_USR_SYSTEM_KERNEL             "KernelRelease"
   #define CMD_USR_SYSTEM_BIT                "Bit"
   #define CMD_USR_SYSTEM_IP                 "Ip"
   #define CMD_USR_SYSTEM_HOSTS              "Hosts"
   #define CMD_USR_SYSTEM_HOSTNAME           "HostName"
   #define CMD_USR_SYSTEM_FILESYSTEM         "Filesystem"
   #define CMD_USR_SYSTEM_CORE               "Core"
   #define CMD_USR_SYSTEM_INFO               "Info"
   #define CMD_USR_SYSTEM_FREQ               "Freq"
   #define CMD_USR_SYSTEM_CPUS               "Cpus"
   #define CMD_USR_SYSTEM_USER               "User"
   #define CMD_USR_SYSTEM_SYS                "Sys"
   #define CMD_USR_SYSTEM_IDLE               "Idle"
   #define CMD_USR_SYSTEM_OTHER              "Other"
   #define CMD_USR_SYSTEM_SIZE               "Size"
   #define CMD_USR_SYSTEM_FREE               "Free"
   #define CMD_USR_SYSTEM_USED               "Used"
   #define CMD_USR_SYSTEM_ISLOCAL            "IsLocal"
   #define CMD_USR_SYSTEM_MOUNT              "Mount"
   #define CMD_USR_SYSTEM_DISKS              "Disks"
   #define CMD_USR_SYSTEM_NAME               "Name"
   #define CMD_USR_SYSTEM_NETCARDS           "Netcards"
   #define CMD_USR_SYSTEM_TARGET             "Target"
   #define CMD_USR_SYSTEM_REACHABLE          "Reachable"
   #define CMD_USR_SYSTEM_USABLE             "Usable"
   #define CMD_USR_SYSTEM_UNIT               "Unit"
   #define CMD_USR_SYSTEM_FSTYPE             "FsType"
   #define CMD_USR_SYSTEM_IO_R_SEC           "ReadSec"
   #define CMD_USR_SYSTEM_IO_W_SEC           "WriteSec"

   #define CMD_USR_SYSTEM_RX_PACKETS         "RXPackets"
   #define CMD_USR_SYSTEM_RX_BYTES           "RXBytes"
   #define CMD_USR_SYSTEM_RX_ERRORS          "RXErrors"
   #define CMD_USR_SYSTEM_RX_DROPS           "RXDrops"

   #define CMD_USR_SYSTEM_TX_PACKETS         "TXPackets"
   #define CMD_USR_SYSTEM_TX_BYTES           "TXBytes"
   #define CMD_USR_SYSTEM_TX_ERRORS          "TXErrors"
   #define CMD_USR_SYSTEM_TX_DROPS           "TXDrops"

   #define CMD_USR_SYSTEM_CALENDAR_TIME      "CalendarTime"



   #define CMD_RESOURCE_NUM                  15
   #define CMD_MB_SIZE                       ( 1024*1024 )
   #define CMD_DISK_SRC_FILE                 "/etc/mtab"

   #define CMD_DISK_IGNORE_TYPE_PROC         "proc"
   #define CMD_DISK_IGNORE_TYPE_SYSFS        "sysfs"
   #define CMD_DISK_IGNORE_TYPE_BINFMT_MISC  "binfmt_misc"
   #define CMD_DISK_IGNORE_TYPE_DEVPTS       "devpts"
   #define CMD_DISK_IGNORE_TYPE_FUSECTL      "fusectl"
   #define CMD_DISK_IGNORE_TYPE_SECURITYFS   "securityfs"
   #define CMD_DISK_IGNORE_TYPE_GVFS         "fuse.gvfs-fuse-daemon"

   #define CMD_USR_SYSTEM_IP                 "Ip"
   #define CMD_USR_SYSTEM_HOSTS              "Hosts"
   #define CMD_USR_SYSTEM_HOSTNAME           "HostName"

   #define CMD_USR_SYSTEM_PROC_USER          "user"
   #define CMD_USR_SYSTEM_PROC_PID           "pid"
   #define CMD_USR_SYSTEM_PROC_STATUS        "status"
   #define CMD_USR_SYSTEM_PROC_CMD           "cmd"
   #define CMD_USR_SYSTEM_LOGINUSER_USER     "user"
   #define CMD_USR_SYSTEM_LOGINUSER_FROM     "from"
   #define CMD_USR_SYSTEM_LOGINUSER_TTY      "tty"
   #define CMD_USR_SYSTEM_LOGINUSER_TIME     "time"
   #define CMD_USR_SYSTEM_ALLUSER_USER       "user"
   #define CMD_USR_SYSTEM_ALLUSER_GID        "gid"
   #define CMD_USR_SYSTEM_ALLUSER_DIR        "dir"
   #define CMD_USR_SYSTEM_GROUP_NAME         "name"
   #define CMD_USR_SYSTEM_GROUP_GID          "gid"
   #define CMD_USR_SYSTEM_GROUP_MEMBERS      "members"

   enum USRSYSTEM_HOST_LINE_TYPE
   {
      LINE_HOST         = 1,
      LINE_UNKNONW,
   } ;

   struct _usrSystemHostItem
   {
      INT32    _lineType ;
      string   _ip ;
      string   _com ;
      string   _host ;

      _usrSystemHostItem()
      {
         _lineType = LINE_UNKNONW ;
      }

      string toString() const
      {
         if ( LINE_UNKNONW == _lineType )
         {
            return _ip ;
         }
         string space = "    " ;
         if ( _com.empty() )
         {
            return _ip + space + _host ;
         }
         return _ip + space + _com + space + _host ;
      }
   } ;
   typedef _usrSystemHostItem usrSystemHostItem ;

   typedef vector< usrSystemHostItem >    VEC_HOST_ITEM ;

   class _sptUsrSystemCommon: public SDBObject
   {
   public:
      static INT32 ping( const string &hostname, string &err,
                         bson::BSONObj &retObj ) ;

      static INT32 type( string &err, string &type ) ;

      static INT32 getReleaseInfo( string &err,
                                   bson::BSONObj &retObj ) ;

      static INT32 getHostsMap( string &err,
                                bson::BSONObj &retObj ) ;

      static INT32 getAHostMap( const string &hostname, string &err,
                                string& ip ) ;

      static INT32 addAHostMap( const string &hostname, const string &ip,
                                const BOOLEAN &isReplace,
                                string &err ) ;

      static INT32 delAHostMap( const string &hostname,
                                string &err ) ;

      static INT32 getCpuInfo( string &err, bson::BSONObj &retObj ) ;

      static INT32 snapshotCpuInfo( string &err,
                                    bson::BSONObj &retObj ) ;

      static INT32 getMemInfo( string &err, bson::BSONObj &retObj ) ;

      static INT32 getDiskInfo( string &err, bson::BSONObj &retObj ) ;

      static INT32 getNetcardInfo( string &err,
                                   bson::BSONObj &retObj ) ;

      static INT32 getIpTablesInfo( string &err,
                                    bson::BSONObj &retObj ) ;

      static INT32 snapshotNetcardInfo( string &err,
                                        bson::BSONObj &retObj ) ;

      static INT32 getHostName( string &err, string &hostname ) ;

      static INT32 sniffPort( const UINT32& port, string &err,
                              bson::BSONObj &retObj ) ;

      static INT32 listProcess( const BOOLEAN& showDetail,
                                string &err, bson::BSONObj &retObj ) ;

      static INT32 killProcess( const bson::BSONObj &optionObj,
                                string &err ) ;

      static INT32 addUser( const bson::BSONObj &configObj,
                            string &err ) ;

      static INT32 addGroup( const bson::BSONObj &configObj,
                             string &err ) ;

      static INT32 setUserConfigs( const bson::BSONObj &configObj,
                                   string &err ) ;

      static INT32 delUser( const bson::BSONObj &configObj,
                            string &err ) ;

      static INT32 delGroup( const string &groupName,
                             string &err ) ;

      static INT32 listLoginUsers( const bson::BSONObj &optionObj,
                                   string &err,
                                   bson::BSONObj &retObj ) ;

      static INT32 listAllUsers( const bson::BSONObj &optionObj,
                                 string &err,
                                 bson::BSONObj &retObj ) ;

      static INT32 listGroups( const bson::BSONObj &optionObj,
                               string &err,
                               bson::BSONObj &retObj ) ;

      static INT32 getCurrentUser( string &err,
                                   bson::BSONObj &retObj ) ;

      static INT32 getSystemConfigs( const string &type,
                                     string &err,
                                     bson::BSONObj &retObj ) ;

      static INT32 getProcUlimitConfigs( string &err,
                                         bson::BSONObj &retObj ) ;

      static INT32 setProcUlimitConfigs( const bson::BSONObj &configsObj,
                                         string &err ) ;

      static INT32 runService( const string& serviceName,
                               const string& command,
                               const string& options,
                               string &err,
                               string& retStr ) ;

      static INT32 createSshKey( string &err ) ;

      static INT32 getHomePath( string &homePath, string &err ) ;

      static INT32 getUserEnv( string &err, bson::BSONObj &retObj ) ;

      static INT32 getPID( UINT32 &pid, string &err ) ;

      static INT32 getTID( UINT32 &tid, string &err ) ;

      static INT32 getEWD( string &ewd, string &err ) ;
   private:
      static INT32 _extractReleaseInfo( const CHAR *buf,
                                        bson::BSONObjBuilder &builder ) ;

#if defined (_LINUX)
      static INT32 _extractReleaseFileInfo( bson::BSONObjBuilder &builder ) ;
#endif //_Linux

      static INT32 _parseHostsFile( VEC_HOST_ITEM &vecItems, string &err ) ;

      static INT32 _writeHostsFile( VEC_HOST_ITEM &vecItems, string &err ) ;

      static INT32 _extractHosts( const CHAR *buf, VEC_HOST_ITEM &vecItems ) ;

      static void _buildHostsResult( VEC_HOST_ITEM & vecItems,
                                     bson::BSONObjBuilder &builder ) ;

      static INT32 _extractCpuInfo( const CHAR *buf,
                                    bson::BSONObjBuilder &builder ) ;

      static INT32 _extractMemInfo( const CHAR *buf,
                                    bson::BSONObjBuilder &builder ) ;

      static INT32 _extractDiskInfo( const CHAR *buf,
                                     bson::BSONObj &retObj ) ;

      static INT32 _extractNetcards( bson::BSONObjBuilder &builder ) ;

      static INT32 _extractNetCardSnapInfo( const CHAR *buf,
                                            bson::BSONObjBuilder &builder ) ;

      static INT32 _extractProcessInfo( const CHAR *buf,
                                        const BOOLEAN &showDetail,
                                        bson::BSONObjBuilder &builder ) ;

      static INT32 _extractLoginUsersInfo( const CHAR *buf,
                                           bson::BSONObjBuilder &builder,
                                           BOOLEAN showDetail ) ;

      static INT32 _extractAllUsersInfo( const CHAR *buf,
                                         bson::BSONObjBuilder &builder,
                                         BOOLEAN showDetail ) ;

      static INT32 _extractGroupsInfo( const CHAR *buf,
                                       bson::BSONObjBuilder &builder,
                                       BOOLEAN showDetail ) ;

      static INT32 _getSystemInfo( std::vector< std::string > typeSplit,
                                   bson::BSONObjBuilder &builder ) ;

      static INT32 _extractEnvInfo( const CHAR *buf,
                                    bson::BSONObjBuilder &builder ) ;
   } ;
}
#endif
