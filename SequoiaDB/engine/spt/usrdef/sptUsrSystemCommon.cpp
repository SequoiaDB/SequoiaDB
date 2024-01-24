/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = sptUsrSystemCommon.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/18/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossCmdRunner.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "ossSocket.hpp"
#include "ossIO.hpp"
#include "ossFile.hpp"
#include "oss.h"
#include "ossPath.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "sptSPDef.hpp"
#include "sptUsrSystemCommon.hpp"
#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "sptCommon.hpp"
#if defined (_LINUX)
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <pwd.h>
#else
#include <iphlpapi.h>
#pragma comment( lib, "IPHLPAPI.lib" )
#endif
using namespace bson ;
using std::vector ;
using std::set ;
namespace engine
{
   INT32 _sptUsrSystemCommon::ping( const string &hostname, string &err,
                                    BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode = SDB_OK ;

#if defined (_LINUX)
      cmd << "ping " << " -q -c 1 "  << "\"" << hostname << "\"" ;
#elif defined (_WINDOWS)
      cmd << "ping -n 2 -w 1000 " << "\"" << hostname << "\"" ;
#endif

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd \"ping\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      builder.append( CMD_USR_SYSTEM_TARGET, hostname ) ;
      builder.appendBool( CMD_USR_SYSTEM_REACHABLE, SDB_OK == exitCode ) ;

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::type( string &err, string &type )
   {
#if defined (_LINUX)
      type = "LINUX" ;
#elif defined (_WINDOWS)
      type = "WINDOWS" ;
#endif
      return SDB_OK ;
   }

   INT32 _sptUsrSystemCommon::getReleaseInfo( string &err, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;

      ossOSInfo info ;
      rc = ossGetOSInfo( info ) ;
      if ( SDB_OK != rc )
      {
         goto error;
      }

#if defined (_LINUX)
      rc = runner.exec( "lsb_release -a |grep -v \"LSB Version\"", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         rc = SDB_OK ;

#if defined (_LINUX)
         rc = _extractReleaseFileInfo(builder);
         if ( SDB_OK != rc )
         {
            rc = SDB_OK ;
            builder.append( CMD_USR_SYSTEM_DISTRIBUTOR, info._distributor ) ;
            builder.append( CMD_USR_SYSTEM_RELASE, info._release ) ;
            builder.append( CMD_USR_SYSTEM_DESP, info._desp ) ;
         }
         builder.append( CMD_USR_SYSTEM_KERNEL, info._release ) ;
         builder.append( CMD_USR_SYSTEM_BIT, info._bit ) ;
#elif defined (_WINDOWS)
         builder.append( CMD_USR_SYSTEM_DISTRIBUTOR, info._distributor ) ;
         builder.append( CMD_USR_SYSTEM_RELASE, info._release ) ;
         builder.append( CMD_USR_SYSTEM_DESP, info._desp ) ;
         builder.append( CMD_USR_SYSTEM_KERNEL, info._release ) ;
         builder.append( CMD_USR_SYSTEM_BIT, info._bit ) ;
#endif

         retObj = builder.obj() ;
         goto done ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"lsb_release -a\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      rc = _extractReleaseInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to extract info from release info:"
            << outStr ;
         err = ss.str() ;
         goto error ;
      }

      outStr = "" ;
#if defined (_LINUX)
      rc = runner.exec( "getconf LONG_BIT", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"getconf LONG_BIT\", rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"getconf LONG_BIT\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }
      builder.append( CMD_USR_SYSTEM_KERNEL, info._release ) ;
      if ( NULL != ossStrstr( outStr.c_str(), "64") )
      {
         builder.append( CMD_USR_SYSTEM_BIT, 64 ) ;
      }
      else
      {
         builder.append( CMD_USR_SYSTEM_BIT, 32 ) ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractReleaseInfo( const CHAR *buf,
                                                   BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      const string *distributor = NULL ;
      const string *release = NULL ;
      const string *desp = NULL ;

      /// not performance sensitive.
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\n:") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); itr++ )
      {
         if ( itr->empty() )
         {
            continue ;
         }

         try
         {
            boost::algorithm::trim( *itr ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to trim, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         if ( "Distributor ID" == *itr &&
              itr < splited.end() - 1 )
         {
            distributor = &( *( itr + 1 ) ) ;
         }
         else if ( "Release" == *itr &&
                   itr < splited.end() - 1 )
         {
            release = &( *( itr + 1 ) ) ;
         }
         else if ( "Description" == *itr &&
                   itr < splited.end() - 1 )
         {
            desp = &( *( itr + 1 ) ) ;
         }
      }
      if ( NULL == distributor ||
           NULL == release )
      {
         PD_LOG( PDERROR, "failed to split release info:%s",
                 buf )  ;
         rc = SDB_SYS ;
         goto error ;
      }

      builder.append( CMD_USR_SYSTEM_DISTRIBUTOR, *distributor ) ;
      builder.append( CMD_USR_SYSTEM_RELASE, *release ) ;
      builder.append( CMD_USR_SYSTEM_DESP, *desp ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined( _LINUX )

   #define REDHAT_RELEASE_FILE      "/etc/redhat-release"
   #define SUSE_RELEASE_FILE        "/etc/SuSE-release"
   #define OS_RELEASE_FILE          "/etc/os-release"

   INT32 _sptUsrSystemCommon::_extractReleaseFileInfo( bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT64 fileSize = 0 ;
      CHAR *readBuffer = NULL ;
      INT64 readSize = 0 ;
      string releaseFilePath ;
      enum distroType { Redhat, Suse, OS } ;
      distroType type ;

      vector<string> splited ;
      string distro ;
      string release ;
      string description ;

      ossFile file ;
      BOOLEAN ifExists ;

      if ( SDB_OK == ( rc = ossFile::exists( REDHAT_RELEASE_FILE, ifExists ) ) &&
            true == ifExists )
      {
         releaseFilePath = REDHAT_RELEASE_FILE ;
         type = Redhat ;
      }
      else if ( SDB_OK == ( rc = ossFile::exists( SUSE_RELEASE_FILE, ifExists ) ) &&
            true == ifExists )
      {
         releaseFilePath = SUSE_RELEASE_FILE ;
         type = Suse ;
      }
      else if (SDB_OK == ( rc = ossFile::exists( OS_RELEASE_FILE, ifExists ) ) &&
            true == ifExists )
      {
         releaseFilePath = OS_RELEASE_FILE ;
         type = OS ;
      }
      else
      {
         if ( SDB_OK == rc && false == ifExists )
         {
            rc = SDB_FNE;
         }
         goto error ;
      }

      rc = file.open( releaseFilePath, OSS_READONLY|OSS_SHAREREAD, 0 ) ;
      if ( SDB_OK != rc )
      {
         goto error;
      }
      rc = file.getFileSize( fileSize ) ;
      if ( SDB_OK != rc )
      {
         goto error;
      }
      readBuffer = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !readBuffer )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( readBuffer, 0, fileSize + 1 ) ;
      rc = file.readN( readBuffer, fileSize, readSize ) ;
      if ( SDB_OK != rc )
      {
         goto error;
      }
      file.close();

      splited = utilStrSplit( readBuffer, "\n" ) ;
      if ( splited.empty() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( Redhat == type )
      {
         description = splited[0] ;
         vector<string> words = utilStrSplit( splited[0] , " " ) ;
         for ( vector<string>::iterator iter = words.begin(); iter != words.end(); iter++ )
         {
            if ( iter->empty() )
            {
               continue ;
            }
            else if ( iter == words.begin() )
            {
               if ( "CentOS" == *iter )
               {
                  distro = "CentOS" ;
               }
               else if ("Red" == *iter )
               {
                  distro = "RedHatEnterpriseServer" ;
               }
            }
            else if ( "release" == *iter &&
                 iter != words.end() - 1 )
            {
               release = *( iter + 1 ) ;
            }
         }
      }
      else if ( Suse == type )
      {
         distro = "SUSE LINUX" ;
         for ( vector<string>::iterator iter = splited.begin(); iter != splited.end(); iter++ )
         {
            if ( iter == splited.begin() )
            {
               description = *iter ;
            }
            else
            {
               vector<string> words = utilStrSplit( *iter , "=" ) ;
               for ( vector<string>::iterator iter = words.begin(); iter != words.end(); iter++ )
               {
                  utilStrTrim( *iter ) ;
                  if ( "VERSION" == *iter &&
                        iter != words.end() - 1 )
                  {
                     utilStrTrim( *( iter + 1 )  ) ;
                     release = *( iter + 1 ) ;
                     break ;
                  }
               }
            }
         }
      }
      else if ( OS == type )
      {
            for ( vector<string>::iterator iter = splited.begin(); iter != splited.end(); iter++ )
         {
               vector<string> words = utilStrSplit( *iter , "=" ) ;
               for ( vector<string>::iterator iter = words.begin(); iter != words.end(); iter++ )
               {
                  if ( iter->empty() )
                  {
                     continue ;
                  }
                  else if ( "NAME" == *iter &&
                            iter != words.end() -1 )
                  {
                     distro = *( iter + 1 ) ;
                     boost::algorithm::erase_all( distro, "\"" ) ;
                  }
                  else if ( "PRETTY_NAME" == *iter &&
                            iter != words.end() -1 )
                  {
                     description = *( iter + 1 ) ;
                     boost::algorithm::erase_all( description, "\"" ) ;
                  }
                  else if ( "VERSION_ID" == *iter &&
                            iter != words.end() -1 )
                  {
                     release = *( iter + 1 ) ;
                     boost::algorithm::erase_all( release, "\"" ) ;
                  }
               }
            }
      }

      if ( 0 == distro.size() ||
           0 == release.size() ||
           0 == description.size() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      builder.append( CMD_USR_SYSTEM_DISTRIBUTOR, distro ) ;
      builder.append( CMD_USR_SYSTEM_RELASE, release ) ;
      builder.append( CMD_USR_SYSTEM_DESP, description ) ;

   done:
      if ( readBuffer )
      {
         SDB_OSS_FREE( readBuffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }
#endif //_Linux

   INT32 _sptUsrSystemCommon::getHostsMap( string &err, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      VEC_HOST_ITEM vecItems ;

      rc = _parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         goto error ;
      }

      _buildHostsResult( vecItems, builder ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::getAHostMap( const string &hostname,
                                           string &err,
                                           string &ip )
   {
      INT32 rc = SDB_OK ;
      VEC_HOST_ITEM vecItems ;

      rc = _parseHostsFile( vecItems, err ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         while ( it != vecItems.end() )
         {
            usrSystemHostItem &item = *it ;
            ++it ;
            if( LINE_HOST == item._lineType && hostname == item._host )
            {
               ip = item._ip ;
               goto done ;
            }
         }
         err = "hostname not exist" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::addAHostMap( const string &hostname,
                                           const string &ip,
                                           const BOOLEAN &isReplace,
                                           string &err )
   {
      INT32 rc = SDB_OK ;
      VEC_HOST_ITEM vecItems ;

      if ( !isValidIPV4( ip.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         err = "ip is not ipv4" ;
         goto error ;
      }
      rc = _parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         BOOLEAN hasMod = FALSE ;
         while ( it != vecItems.end() )
         {
            usrSystemHostItem &item = *it ;
            ++it ;
            if( item._lineType == LINE_HOST && hostname == item._host )
            {
               if ( item._ip == ip )
               {
                  goto done ;
               }
               else if ( !isReplace )
               {
                  err = "hostname already exist" ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               item._ip = ip ;
               hasMod = TRUE ;
            }
         }
         if ( !hasMod )
         {
            usrSystemHostItem info ;
            info._lineType = LINE_HOST ;
            info._host = hostname ;
            info._ip = ip ;
            vecItems.push_back( info ) ;
         }
         // write
         rc = _writeHostsFile( vecItems, err ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::delAHostMap( const string &hostname,
                                           string &err )
   {
      INT32 rc = SDB_OK ;
      VEC_HOST_ITEM vecItems ;

      rc = _parseHostsFile( vecItems, err ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         BOOLEAN hasDel = FALSE ;
         while ( it != vecItems.end() )
         {
            usrSystemHostItem &item = *it ;
            if( item._lineType == LINE_HOST && hostname == item._host )
            {
               // del
               it = vecItems.erase( it ) ;
               hasDel = TRUE ;
               continue ;
            }
            ++it ;
         }
         // write
         if ( hasDel )
         {
            rc = _writeHostsFile( vecItems, err ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined(_LINUX)
   INT32 _sptUsrSystemCommon::getCpuInfo( string &err, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;
#if defined(_ARMLIN64)
   #define CPU_CMD "cat /proc/cpuinfo | grep -E 'model name'"
#elif defined (_PPCLIN64)
   #define CPU_CMD "cat /proc/cpuinfo | grep -E 'processor|cpu|clock|machine'"
#else
   #define CPU_CMD "cat /proc/cpuinfo | grep -E 'model name|cpu MHz|cpu cores|physical id'"
#endif

      rc = runner.exec( CPU_CMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         rc = SDB_OK ;
      }
      else 
      {
         rc = runner.read( outStr ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
            stringstream ss ;
            ss << "failed to read msg from cmd \"" << CPU_CMD << "\", rc:"
               << rc ;
            err = ss.str() ;
            goto error ;
         } 
      }

      rc = _extractCpuInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract cpu info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         err = ss.str() ;
         goto error ;
      }

      {
         SINT64 user = 0 ;
         SINT64 sys = 0 ;
         SINT64 idle = 0 ;
         SINT64 other = 0 ;
         SINT64 iowait = 0 ;
         rc = ossGetCPUInfo( user, sys, idle, iowait, other ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         builder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
         builder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IOWAIT, iowait ) ;
         builder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;
      }
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined(_WINDOWS)
   INT32 _sptUsrSystemCommon::getCpuInfo( string &err, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;
      const CHAR *cmd = "wmic CPU GET CurrentClockSpeed,Name,NumberOfCores" ;

      rc = runner.exec( cmd, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         rc = SDB_OK ;
      }
      else
      {
         rc = runner.read( outStr ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
            stringstream ss ;
            ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
               << rc ;
            err = ss.str() ;
            goto error ;
         }
      }
      rc = _extractCpuInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract cpu info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         err = ss.str() ;
         goto error ;
      }

      {
         SINT64 user = 0 ;
         SINT64 sys = 0 ;
         SINT64 idle = 0 ;
         SINT64 iowait = 0 ;
         SINT64 other = 0 ;
         rc = ossGetCPUInfo( user, sys, idle, iowait, other ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         builder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
         builder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IOWAIT, iowait ) ;
         builder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }
#endif

   INT32 _sptUsrSystemCommon::snapshotCpuInfo( string &err,
                                               BSONObj &retObj )
   {
      INT32 rc      = SDB_OK ;
      SINT64 user   = 0 ;
      SINT64 sys    = 0 ;
      SINT64 idle   = 0 ;
      SINT64 iowait = 0 ;
      SINT64 other  = 0 ;
      rc = ossGetCPUInfo( user, sys, idle, iowait, other ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get cpuinfo:%d", rc ) ;
         stringstream ss ;
         ss << "failed to get cpuinfo:rc="
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      {
         BSONObjBuilder builder ;
         builder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
         builder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IOWAIT, iowait ) ;
         builder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;

         retObj = builder.obj() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::getMemInfo( string &err,
                                          BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;

#if defined (_LINUX)
      rc = runner.exec( "free -m |grep Mem", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         INT32 loadPercent = 0 ;
         INT64 totalPhys = 0 ;
         INT64 freePhys  = 0 ;
         INT64 availPhys = 0 ;
         INT64 totalPF = 0 ;
         INT64 availPF = 0 ;
         INT64 totalVirtual = 0 ;
         INT64 availVirtual = 0 ;
         rc = ossGetMemoryInfo( loadPercent, totalPhys, freePhys,
                                availPhys, totalPF, availPF,
                                totalVirtual, availVirtual ) ;
         if ( rc )
         {
            stringstream ss ;
            ss << "ossGetMemoryInfo failed, rc:" << rc ;
            err = ss.str() ;
            goto error ;
         }

         builder.append( CMD_USR_SYSTEM_SIZE, (INT32)(totalPhys/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_USED,
                         (INT32)((totalPhys-availPhys)/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_FREE, (INT32)(freePhys/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_AVAILABLE, (INT32)(availPhys/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;
         retObj = builder.obj() ;
         goto done ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"free\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      rc = _extractMemInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract mem info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         err = ss.str() ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined( _LINUX )
   INT32 _sptUsrSystemCommon::getDiskInfo( string &err,
                                           BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      SINT64 read = 0 ;
      OSSFILE file ;
      stringstream ss ;
      stringstream filess ;
      const UINT32 bufSize = 256 ;
      CHAR buf[ bufSize + 1 ] = { 0 } ;

      rc = ossOpen( CMD_DISK_SRC_FILE,
                    OSS_READONLY | OSS_SHAREREAD,
                    OSS_DEFAULTFILE,
                    file ) ;
      if ( SDB_OK != rc )
      {
         ss << "failed to open file(/etc/mtab), rc:" << rc ;
         err = ss.str() ;
         goto error ;
      }

      do
      {
         read = 0 ;
         ossMemset( buf, '\0', bufSize ) ;
         rc = ossReadN( &file, bufSize, buf, read ) ;

         if ( SDB_EOF == rc && filess.tellp() )
         {
            rc = SDB_OK ;
            break ;
         }
         if ( SDB_OK != rc )
         {
            ss << "failed to read file(/etc/mtab), rc:" << rc ;
            err = ss.str() ;
            goto error ;
         }

         filess << buf ;
         if ( read < bufSize )
         {
            break ;
         }
      } while ( TRUE ) ;

      rc = _extractDiskInfo( filess.str().c_str(), retObj ) ;
      if ( SDB_OK != rc )
      {
         err =  "Failed to extract disk info" ;
         goto error ;
      }
   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractDiskInfo( const CHAR *buf,
                                                BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONArrayBuilder arrBuilder ;
      vector<string> splited ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         BSONObjBuilder diskBuilder ;
         INT64 totalBytes   = 0 ;
         INT64 freeBytes    = 0 ;
         INT64 availBytes   = 0 ;
         INT32 loadPercent  = 0 ;
         const CHAR *fs     = NULL ;
         const CHAR *fsType = NULL ;
         const CHAR *mount  = NULL ;
         vector<string> columns ;

         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t " ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         if ( 6 != columns.size() )
         {
            continue ;
         }

         fsType = columns.at( 2 ).c_str() ;
         if ( ossStrcasecmp( CMD_DISK_IGNORE_TYPE_BINFMT_MISC, fsType ) == 0 ||
              ossStrcasecmp( CMD_DISK_IGNORE_TYPE_SYSFS, fsType ) == 0 ||
              ossStrcasecmp( CMD_DISK_IGNORE_TYPE_PROC, fsType ) == 0 ||
              ossStrcasecmp( CMD_DISK_IGNORE_TYPE_DEVPTS, fsType ) == 0 ||
              ossStrcasecmp( CMD_DISK_IGNORE_TYPE_FUSECTL, fsType ) == 0 ||
              ossStrcasecmp( CMD_DISK_IGNORE_TYPE_GVFS, fsType ) == 0 ||
              ossStrcasecmp( CMD_DISK_IGNORE_TYPE_SECURITYFS, fsType ) == 0 )
         {
            continue ;
         }

         fs = columns.at( 0 ).c_str() ;
         mount = columns.at( 1 ).c_str() ;

         rc = ossGetDiskInfo( mount, totalBytes, freeBytes,
                              availBytes, loadPercent ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get disk info, rc: %d, path: %s",
                    rc, mount ) ;
            rc = SDB_OK ;
         }
         else
         {
            diskBuilder.append( CMD_USR_SYSTEM_FILESYSTEM, fs ) ;
            diskBuilder.append( CMD_USR_SYSTEM_FSTYPE, fsType ) ;
            diskBuilder.appendNumber( CMD_USR_SYSTEM_SIZE,
                                      totalBytes / ( 1024 * 1024 ) ) ;
            diskBuilder.appendNumber( CMD_USR_SYSTEM_USED,
                              ( totalBytes - freeBytes ) / ( 1024 * 1024 ) ) ;
            diskBuilder.append( CMD_USR_SYSTEM_UNIT, "MB" ) ;
            diskBuilder.append( CMD_USR_SYSTEM_MOUNT, mount ) ;

            BOOLEAN isLocal = ( string::npos != columns.at( 0 ).find( "/dev/",
                                                                      0, 5 ) ) ;
            BOOLEAN gotStat = FALSE ;
            diskBuilder.appendBool( CMD_USR_SYSTEM_ISLOCAL, isLocal ) ;

            if ( isLocal )
            {
               ossDiskIOStat ioStat ;
               CHAR pathBuffer[ 256 ] = {0} ;
               INT32 rctmp = SDB_OK ;

               string driverName = columns.at( 0 ) ;

               rctmp = ossReadlink( driverName.c_str(), pathBuffer, 256 ) ;
               if ( SDB_OK == rctmp )
               {
                  // only match the driver name after /dev/
                  driverName = pathBuffer ;
                  if ( string::npos != driverName.find( "../", 0, 3 ) )
                  {
                     driverName.replace(0, 3, "" ) ;
                  }
                  else
                  {
                     driverName = columns.at( 0 ) ;
                     driverName.replace(0, 5, "" ) ;
                  }
               }
               else
               {
                  driverName.replace(0, 5, "" ) ;
               }

               ossMemset( &ioStat, 0, sizeof( ossDiskIOStat ) ) ;

               rctmp = ossGetDiskIOStat( driverName.c_str(), ioStat ) ;
               if ( SDB_OK == rctmp )
               {
                  diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC,
                                            (INT64)ioStat.rdSectors ) ;
                  diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC,
                                            (INT64)ioStat.wrSectors ) ;
                  gotStat = TRUE ;
               }
            }
            if ( !gotStat )
            {
               diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC, (INT64)0 ) ;
               diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC, (INT64)0 ) ;
            }

            arrBuilder << diskBuilder.obj() ;
         }
      }

      builder.append( CMD_USR_SYSTEM_DISKS, arrBuilder.arr() ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined( _WINDOWS )
   INT32 _sptUsrSystemCommon::getDiskInfo( string &err,
                                           BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;

#define DISK_CMD  "wmic VOLUME get Capacity,DriveLetter,Caption,"\
                  "DriveType,FreeSpace,SystemVolume"

      rc = runner.exec( DISK_CMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"" << DISK_CMD << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"df\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      rc = _extractDiskInfo( outStr.c_str(), retObj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract disk info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         err = ss.str() ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractDiskInfo( const CHAR *buf,
                                                BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      string fileSystem ;
      string freeSpace ;
      string total ;
      string mount ;
      vector<string> splited ;
      INT32 lineCount = 0 ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         ++lineCount ;
         if ( 1 == lineCount || itr->empty() )
         {
            continue ;
         }

         vector<string> columns ;

         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end();
               /// do not ++
               )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         if ( columns.size() < 6 || columns.at( 5 ) == "TRUE" ||
              columns.at( 3 ) != "3" )
         {
            continue ;
         }

         total = columns[ 0 ] ;
         fileSystem = columns[ 1 ] ;
         freeSpace = columns[ 4 ] ;
         mount = columns[ 2 ] ;

         // build
         SINT64 totalNum = 0 ;
         SINT64 usedNumber = 0 ;
         SINT64 avaNumber = 0 ;
         BSONObjBuilder lineBuilder ;
         try
         {
            avaNumber = boost::lexical_cast<SINT64>( freeSpace ) ;
            totalNum = boost::lexical_cast<SINT64>( total ) ;
            usedNumber = totalNum - avaNumber ;
            lineBuilder.append( CMD_USR_SYSTEM_FILESYSTEM,
                                fileSystem.c_str() ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_SIZE,
                                      (INT32)( totalNum / CMD_MB_SIZE ) ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_USED,
                                      (INT32)( usedNumber / CMD_MB_SIZE ) ) ;
            lineBuilder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;
            lineBuilder.append( CMD_USR_SYSTEM_MOUNT, mount ) ;
            lineBuilder.appendBool( CMD_USR_SYSTEM_ISLOCAL, TRUE ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC, (INT64)0 ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC, (INT64)0 ) ;
            arrBuilder << lineBuilder.obj() ;
         }
         catch ( std::exception )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         freeSpace.clear();
         total.clear() ;
         mount.clear() ;
         fileSystem.clear() ;
      } // end for

      retObj = BSON( CMD_USR_SYSTEM_DISKS << arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#endif

   INT32 _sptUsrSystemCommon::getNetcardInfo( string &err,
                                              BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      rc = _extractNetcards( builder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to get netcard info:" << rc ;
         err = ss.str() ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::getIpTablesInfo( string &err,
                                               BSONObj &retObj )
   {
      retObj = BSON( "FireWall" << "unknown" ) ;
      return SDB_OK ;
   }

#if defined( _LINUX )
   INT32 _sptUsrSystemCommon::snapshotNetcardInfo( string &err,
                                                   BSONObj &retObj )
   {
      INT32 rc        = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      stringstream ss ;
      BSONObjBuilder builder ;
      const CHAR *netFlowCMD = "cat /proc/net/dev | grep -v Receive |"
                               " grep -v bytes | sed 's/:/ /' |"
                               " awk '{print $1,$2,$3,$4,$5,$10,$11,$12,$13}'" ;

      rc = runner.exec( netFlowCMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         ss << "failed to exec cmd \"" << netFlowCMD << "\",rc:"
            << rc << ",exit:" << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"df\", rc:" << rc ;
         err = ss.str() ;
         goto error ;
      }

      rc = _extractNetCardSnapInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract netcard snapshotinfo:%d", rc ) ;
         ss << "failed to extract netcard snapshotinfo from buf:" << outStr ;
         err = ss.str() ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractNetCardSnapInfo( const CHAR *buf,
                                                       BSONObjBuilder &builder )
   {
      time_t myTime = time( NULL ) ;
      BSONArrayBuilder arrayBuilder ;
      INT32 rc = SDB_OK ;
      vector<string> vLines ;
      vector<string>::iterator iterLine ;

      try
      {
         boost::algorithm::split( vLines, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      iterLine = vLines.begin() ;

      while ( iterLine != vLines.end() )
      {
         if ( !iterLine->empty() )
         {
            const CHAR *oneLine = iterLine->c_str() ;
            vector<string> vColumns ;
            try
            {
               boost::algorithm::split( vColumns, oneLine,
                                        boost::is_any_of( "\t " ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            vector<string>::iterator iterColumn = vColumns.begin() ;
            while ( iterColumn != vColumns.end() )
            {
               if ( iterColumn->empty() )
               {
                  vColumns.erase( iterColumn++ ) ;
               }
               else
               {
                  iterColumn++ ;
               }
            }

            // if not match format,discard the row
            if ( vColumns.size() < 9 )
            {
               continue ;
            }
      //card rx_byte   rx_packet rx_err rx_drop tx_byte tx_packet tx_err tx_drop
      //lo   14755559460 44957591  0      0       14755559460 44957591 0 0
      //eth1 4334054313  11529654  0      0       9691246348  3513633  0 0
            try
            {
               BSONObjBuilder innerBuilder ;
               innerBuilder.append( CMD_USR_SYSTEM_NAME,
                             boost::lexical_cast<string>( vColumns.at( 0 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_BYTES,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 1 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_PACKETS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 2 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_ERRORS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 3 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_DROPS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 4 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_BYTES,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 5 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_PACKETS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 6 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_ERRORS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 7 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_DROPS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 8 ) ) ) ;
               BSONObj obj = innerBuilder.obj() ;
               arrayBuilder.append( obj ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         iterLine++ ;
      }

      try
      {
         builder.append( CMD_USR_SYSTEM_CALENDAR_TIME, (long long)myTime ) ;
         builder.append( CMD_USR_SYSTEM_NETCARDS, arrayBuilder.arr() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
#elif defined( _WINDOWS )
   INT32 _sptUsrSystemCommon::snapshotNetcardInfo( string &err,
                                                   BSONObj &retObj )
   {
      INT32 rc              = SDB_OK ;
      UINT32 exitCode       = 0 ;
      PMIB_IFTABLE pTable   = NULL ;
      stringstream ss ;
      time_t myTime ;
      BSONObjBuilder builder ;

      DWORD size = sizeof( MIB_IFTABLE ) ;
      pTable     = (PMIB_IFTABLE) SDB_OSS_MALLOC( size ) ;
      if ( NULL == pTable )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "new MIB_IFTABLE failed:rc=%d", rc ) ;
         ss << "new MIB_IFTABLE failed:rc=" << rc ;
         err = ss.str() ;
         goto error ;
      }

      ULONG uRetCode = GetIfTable( pTable, &size, TRUE ) ;
      if ( uRetCode == ERROR_NOT_SUPPORTED )
      {
         PD_LOG ( PDERROR, "GetIfTable failed:rc=%u", uRetCode ) ;
         rc = SDB_INVALIDARG ;
         ss << "GetIfTable failed:rc=" << uRetCode ;
         err = ss.str() ;
         goto error ;
      }

      if ( uRetCode == ERROR_INSUFFICIENT_BUFFER )
      {
         SDB_OSS_FREE( pTable ) ;
         pTable = (PMIB_IFTABLE) SDB_OSS_MALLOC( size ) ;
         if ( NULL == pTable )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "new MIB_IFTABLE failed:rc=%d", rc ) ;
            ss << "new MIB_IFTABLE failed:rc=" << rc ;
            err = ss.str() ;
            goto error ;
         }
      }

      // get the seconds since 1970.1.1:0:0:0(Calendar Time)
      myTime = time( NULL ) ;
      uRetCode = GetIfTable( pTable, &size, TRUE ) ;
      if ( NO_ERROR != uRetCode )
      {
         PD_LOG ( PDERROR, "GetIfTable failed:rc=%u", uRetCode ) ;
         rc = SDB_INVALIDARG ;
         ss << "GetIfTable failed:rc=" << uRetCode ;
         err = ss.str() ;
         goto error ;
      }

      try
      {
         BSONArrayBuilder arrayBuilder ;
         for ( UINT i = 0 ; i < pTable->dwNumEntries ; i++ )
         {
            MIB_IFROW Row = pTable->table[ i ];
            if ( IF_TYPE_ETHERNET_CSMACD != Row.dwType )
            {
               continue ;
            }

            BSONObjBuilder innerBuilder ;
            stringstream ss ;
            ss << "eth" << Row.dwIndex ;
            innerBuilder.append( CMD_USR_SYSTEM_NAME, ss.str() ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_BYTES,
                                 ( long long )Row.dwInOctets ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_PACKETS,
                          ( long long )
                                 ( Row.dwInUcastPkts + Row.dwInNUcastPkts ) ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_ERRORS,
                                 ( long long )Row.dwInErrors ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_DROPS,
                                 ( long long )Row.dwInDiscards ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_BYTES,
                                 ( long long )Row.dwOutOctets ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_PACKETS,
                          ( long long )
                                ( Row.dwOutUcastPkts + Row.dwOutNUcastPkts ) ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_ERRORS,
                                 ( long long )Row.dwOutErrors ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_DROPS,
                                 ( long long )Row.dwOutDiscards ) ;
            BSONObj obj = innerBuilder.obj() ;
            arrayBuilder.append( obj ) ;
         }

         builder.append( CMD_USR_SYSTEM_CALENDAR_TIME, (long long)myTime ) ;
         builder.append( CMD_USR_SYSTEM_NETCARDS, arrayBuilder.arr() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      if ( NULL != pTable )
      {
         SDB_OSS_FREE( pTable ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractNetCardSnapInfo( const CHAR *buf,
                                                       BSONObjBuilder &builder )
   {
      return SDB_INVALIDARG ;
   }
#endif

   INT32 _sptUsrSystemCommon::getHostName( string &err,
                                           string &hostname )
   {
      INT32 rc = SDB_OK ;
      CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
      rc = ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;
      if ( rc )
      {
         err = "get hostname failed" ;
         goto error ;
      }

      hostname = hostName ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::sniffPort( const UINT32& port, string &err,
                                         BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BOOLEAN result = FALSE ;
      BSONObjBuilder builder ;
      if ( 0 >= port || 65535 < port )
      {
         rc = SDB_INVALIDARG ;
         ss << "port must in range ( 0, 65536 )" ;
         goto error ;
      }

      {
         PD_LOG ( PDDEBUG, "sniff port is: %d", port ) ;
         _ossSocket sock( port, OSS_ONE_SEC ) ;
         rc = sock.initSocket() ;
         if ( rc )
         {
            PD_LOG ( PDWARNING, "failed to connect to port[%d], "
                     "rc: %d", port, rc ) ;
            ss << "failed to sniff port" ;
            goto error ;
         }
         rc = sock.bind_listen() ;
         if ( rc )
         {
            PD_LOG ( PDDEBUG, "port[%d] is busy, rc: %d", port, rc ) ;
            result = FALSE ;
            rc = SDB_OK ;
         }
         else
         {
            PD_LOG ( PDDEBUG, "port[%d] is usable", port ) ;
            result = TRUE ;
         }
         builder.appendBool( CMD_USR_SYSTEM_USABLE, result ) ;
         retObj = builder.obj() ;
         //close the socket
         sock.close() ;
      }

   done:
      return rc ;
   error:
      err = ss.str() ;
      goto done ;
   }

   INT32 _sptUsrSystemCommon::listProcess( const BOOLEAN& showDetail,
                                           string &err,
                                           BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode  = 0 ;
      BSONObjBuilder   builder ;
      BSONObj          optionObj ;
      string           outStr ;
      stringstream     cmd ;
      _ossCmdRunner    runner ;

      // build cmd
#if defined ( _LINUX )
   cmd << "ps ax -o user -o pid -o stat -o command" ;
#elif defined (_WINDOWS)
   cmd << "tasklist /FO \"CSV\"" ;
#endif

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      // extract result
      rc = _extractProcessInfo( outStr.c_str(), showDetail, builder ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract process info" ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::killProcess( const BSONObj &optionObj,
                                           string &err )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      INT32 sigNum       = 15 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             sigType ;
      UINT32             pid ;

      if ( TRUE == optionObj.hasField( "sig" ) )
      {
         if ( String != optionObj.getField( "sig" ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "sig must be string" ;
            goto error ;
         }
         sigType = optionObj.getStringField( "sig" ) ;

         if( "term" != sigType && "kill" != sigType )
         {
            rc = SDB_INVALIDARG ;
            err = "sig must be \"term\" or \"kill\"" ;
            goto error ;
         }
         else if ( "kill" == sigType )
         {
            sigNum = 9 ;
         }
      }

      if ( FALSE == optionObj.hasField( "pid" ) )
      {
         rc = SDB_INVALIDARG ;
         err = "pid must be config" ;
      }
      else if ( NumberInt != optionObj.getField( "pid" ).type() )
      {
         rc = SDB_INVALIDARG ;
         err = "pid must be int" ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get pid, rc: %d", rc ) ;
      pid = optionObj.getIntField( "pid" ) ;

#if defined( _LINUX )
      cmd << "kill -" << sigNum ;
      cmd << " " << pid ;
#elif defined( _WINDOWS )
      cmd << "taskkill" ;
      if ( 9 == sigNum )
      {
         cmd << " /F" ;
      }
      cmd << " /PID " << pid ;
#endif

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::addUser( const BSONObj &configObj,
                                       string &err )
   {
#if defined (_LINUX)
      INT32 rc          = SDB_OK ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;
      BOOLEAN           nameFlag = 0 ;

      // init cmd
      cmd << "useradd" ;

      BSONObjIterator it ( configObj ) ;
      while ( it.more() )
      {
         BSONElement elem = it.next() ;
         if ( 0 == ossStrcmp( elem.fieldName(), "name" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "name must be string" ;
               goto error ;
            }
            nameFlag = 1 ;
            cmd << " " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "passwd" ) )
         {
            // if password has been input, we don't save command to history file.
            sdbSetIsNeedSaveHistory( FALSE ) ;
            
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "password must be string" ;
               goto error ;
            }
            cmd << " -p \'" << elem.valuestr() << "\'" ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "gid" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "gid must be string" ;
               goto error ;
            }
            cmd << " -g " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "groups" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "groups must be string" ;
               goto error ;
            }
            cmd << " -G " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "dir" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "dir must be string" ;
               goto error ;
            }
            cmd << " -d " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "createDir" ) )
         {
            if ( Bool != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "createDir must be bool" ;
               goto error ;
            }

            if ( TRUE == elem.boolean() )
            {
               cmd << " -m " ;
            }
            else
            {
               cmd << " -M " ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( 0 == nameFlag )
      {
         rc = SDB_INVALIDARG ;
         err = "name must be config" ;
         goto error ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::addGroup( const BSONObj &configObj,
                                        string &err )
   {
#if defined (_LINUX)
      INT32 rc        = SDB_OK ;
      string          outStr ;
      stringstream    cmd ;
      _ossCmdRunner   runner ;
      UINT32          exitCode ;
      BOOLEAN         nameFlag = 0 ;

      // check argument and build cmd
      cmd << "groupadd" ;

      BSONObjIterator it( configObj ) ;
      while ( it.more() )
      {
         BSONElement elem = it.next() ;
         if ( 0 == ossStrcmp( elem.fieldName(), "name" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "name must be string" ;
               goto error ;
            }
            nameFlag = 1 ;
            cmd << " " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "id" ))
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "id must be string" ;
            }
            cmd << " -g " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "isUnique" ) )
         {
            if ( Bool != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "isUnique must be bool" ;
               goto error ;
            } 
            if ( FALSE == elem.boolean() )
            {
               cmd << " -o " ;
            }
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "passwd" ) )
         {
            sdbSetIsNeedSaveHistory( FALSE ) ;
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "passwd must be string" ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( 0 == nameFlag )
      {
         rc = SDB_INVALIDARG ;
         err = "name must be config" ;
         goto error ;
      }
      
      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::setUserConfigs( const BSONObj &configObj,
                                              string &err )
   {
#if defined(_LINUX)
      INT32 rc          = SDB_OK ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;
      BOOLEAN           nameFlag = 0 ;

      cmd << "usermod" ;

      BSONObjIterator it( configObj ) ;
      while ( it.more() )
      {
         BSONElement elem = it.next() ;
         if ( 0 == ossStrcmp( elem.fieldName(), "name" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "name must be string" ;
               goto error ;
            }
            nameFlag = 1 ;
            cmd << " " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "passwd" ) )
         {
            sdbSetIsNeedSaveHistory( FALSE ) ;
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "passwd must be string" ;
               goto error ;
            }
            cmd << " -p \'" << elem.valuestr() << "\' ";
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "gid" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "gid must be string" ;
               goto error ;
            }
            cmd << " -g " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "groups" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "groups must be string" ;
               goto error ;
            }
            cmd << " -G " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "isAppend" ) )
         {
            if ( Bool != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "isAppend must be bool" ;
               goto error ;
            }
            if ( TRUE == elem.boolean() )
            {
               cmd << " -a " ;
            }
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "dir" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "dir must be string" ;
               goto error ;
            }
            cmd << " -d " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "isMove" ) )
         {
            if ( Bool != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "isMove must be bool" ;
               goto error ;
            }
            if ( TRUE == elem.boolean() )
            {
               cmd << " -m " ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( 0 == nameFlag )
      {
         rc = SDB_INVALIDARG ;
         err = "name must be config" ;
         goto error ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::delUser( const BSONObj &configObj,
                                       string &err )
   {
#if defined (_LINUX)
      INT32 rc          = SDB_OK ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;
      BOOLEAN           nameFlag = 0 ;

      cmd << "userdel" ;
      
      BSONObjIterator it( configObj ) ;
      while ( it.more() )
      {
         BSONElement elem = it.next() ;
         if ( 0 == ossStrcmp( elem.fieldName(), "name" ) )
         {
            if ( String != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "name must be string" ;
               goto error ;
            }
            nameFlag = 1 ;
            cmd << " " << elem.valuestr() ;
         }
         else if ( 0 == ossStrcmp( elem.fieldName(), "isRemoveDir" ) )
         {
            if ( Bool != elem.type() )
            {
               rc = SDB_INVALIDARG ;
               err = "isRemoveDir must be bool" ;
               goto error ;
            }
            if ( TRUE == elem.boolean() )
            {
               cmd << " -r " ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( 0 == nameFlag )
      {
         rc = SDB_INVALIDARG ;
         err = "name must be config" ;
         goto error ;
      }
      
      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit: %d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::delGroup( const string &groupName,
                                        string &err )
   {
#if defined (_LINUX)
      INT32 rc          = SDB_OK ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;

      cmd << "groupdel " << groupName ;
      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::listLoginUsers( const BSONObj &optionObj,
                                              string &err,
                                              BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "who" ;
      showDetail = optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      // extract result
      rc = _extractLoginUsersInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract login user info" ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::listAllUsers( const BSONObj &optionObj,
                                            string &err,
                                            BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      string             outStr ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;

      // check argument and build cmd
      cmd << "cat /etc/passwd" ;
      showDetail = optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      // extract result
      rc = _extractAllUsersInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract all users info" ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::listGroups( const BSONObj &optionObj,
                                          string &err,
                                          BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "cat /etc/group" ;
      showDetail = optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      // extract result
      rc = _extractGroupsInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract group info" ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::getCurrentUser( string &err,
                                              BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      stringstream       gidStr ;
      _ossCmdRunner      runner ;
      string             username ;
      string             homeDir ;
      OSSUID             uid ;
      OSSGID             gid ;

      cmd << "whoami 2>/dev/null" ;
      // run command
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // read result
      rc = runner.read( username ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }
      if( username[ username.size() - 1 ] == '\n' )
      {
         username.erase( username.size()-1, 1 ) ;
      }

      // get user info
      rc = ossGetUserInfo( username.c_str(), uid, gid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get gid" ) ;
         rc = SDB_OK ;
      }
      else
      {
         gidStr << gid ;
      }

      // get home dir
      rc = getHomePath( homeDir, err ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err.c_str() ) ;
         homeDir = "" ;
         rc = SDB_OK ;
      }

      builder.append( "user", username ) ;
      builder.append( "gid", gidStr.str() ) ;
      builder.append( "dir", homeDir ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      retObj = BSONObj() ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::getSystemConfigs( const string &type,
                                                string &err,
                                                BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      vector<string>     typeSplit ;
      string configsType[] = { "kernel", "vm", "fs", "debug", "dev", "abi",
                               "net" } ;

      if( !type.empty() )
      {
         // input format: "xxx|xxxx|xxx"
         try
         {
            boost::algorithm::split( typeSplit, type, boost::is_any_of( " |" ) ) ;
         }
         catch( std::exception )
         {
            rc = SDB_SYS ;
            err = "Failed to split result" ;
            PD_LOG( PDERROR, "Failed to split result" ) ;
            goto error ;
         }

         // if specify all type, remove other type
         if( typeSplit.end() != find( typeSplit.begin(), typeSplit.end(),
                                      "all" ) )
         {
            typeSplit.clear() ;
            typeSplit.push_back( "" ) ;
         }
         else
         {
            for( vector< string >::iterator itr = typeSplit.begin();
                 itr != typeSplit.end(); )
            {
               // if not required type ,erase it
               if( configsType + 7 == find( configsType,
                                            configsType + 7,
                                            *itr ) )
               {
                  itr = typeSplit.erase( itr ) ;
               }
               else
               {
                  itr++ ;
               }
            }
         }
      }
      else
      {
         typeSplit.push_back( "" ) ;
      }

      rc = _getSystemInfo( typeSplit, builder ) ;
      if( SDB_OK != rc )
      {
         err = "Failed to get system info" ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::getProcUlimitConfigs( string &err,
                                                    BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc               = SDB_OK ;
      BSONObjBuilder         builder ;
      INT32 resourceType[] = { RLIMIT_CORE, RLIMIT_DATA, RLIMIT_NICE,
                               RLIMIT_FSIZE, RLIMIT_SIGPENDING, RLIMIT_MEMLOCK,
                               RLIMIT_RSS, RLIMIT_NOFILE, RLIMIT_MSGQUEUE,
                               RLIMIT_RTPRIO, RLIMIT_STACK, RLIMIT_CPU,
                               RLIMIT_NPROC, RLIMIT_AS, RLIMIT_LOCKS } ;
      char *resourceName[] = { "core_file_size", "data_seg_size",
                               "scheduling_priority", "file_size",
                               "pending_signals", "max_locked_memory",
                               "max_memory_size", "open_files",
                               "POSIX_message_queues", "realtime_priority",
                               "stack_size", "cpu_time", "max_user_processes",
                               "virtual_memory", "file_locks" } ;

      // get ulimit
      for ( UINT32 index = 0; index < CMD_RESOURCE_NUM; index++ )
      {
         rlimit rlim ;
         if ( 0 != getrlimit( resourceType[ index ], &rlim ) )
         {
            rc = SDB_SYS ;
            err = "Failed to get user limit info" ;
            goto error ;
         }

         // -1 mean unlimited
         // if val > max exact int or val == ulimited, append as string
         if ( OSS_SINT64_JS_MAX < (UINT64)rlim.rlim_cur &&
              (UINT64)-1 != rlim.rlim_cur )
         {
            builder.append( resourceName[ index ],
                            boost::lexical_cast<string>( rlim.rlim_cur ) ) ;
         }
         else
         {
            builder.append( resourceName[ index ], (INT64)rlim.rlim_cur ) ;
         }
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::setProcUlimitConfigs( const BSONObj &configsObj,
                                                    string &err )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      INT32 resourceType[] = { RLIMIT_CORE, RLIMIT_DATA, RLIMIT_NICE,
                               RLIMIT_FSIZE, RLIMIT_SIGPENDING, RLIMIT_MEMLOCK,
                               RLIMIT_RSS, RLIMIT_NOFILE, RLIMIT_MSGQUEUE,
                               RLIMIT_RTPRIO, RLIMIT_STACK, RLIMIT_CPU,
                               RLIMIT_NPROC, RLIMIT_AS, RLIMIT_LOCKS } ;
      char *resourceName[] = { "core_file_size", "data_seg_size",
                               "scheduling_priority", "file_size",
                               "pending_signals", "max_locked_memory",
                               "max_memory_size", "open_files",
                               "POSIX_message_queues", "realtime_priority",
                               "stack_size", "cpu_time", "max_user_processes",
                               "virtual_memory", "file_locks" } ;
      // set ulimit
      for ( UINT32 index = 0; index < CMD_RESOURCE_NUM; index++ )
      {
         char *limitItem = resourceName[ index ] ;
         if ( configsObj[ limitItem ].ok() )
         {
            // support string type or number type
            if( FALSE == configsObj.getField( limitItem ).isNumber() &&
                String != configsObj.getField( limitItem ).type() )
            {
               rc = SDB_INVALIDARG ;
               err = "value must be number or string" ;
               goto error ;
            }

            rlimit rlim ;
            if ( 0 != getrlimit( resourceType[ index ], &rlim ) )
            {
               rc = SDB_SYS ;
               err = "Failed to get user limit info" ;
               goto error ;
            }

            if ( configsObj.getField( limitItem ).isNumber() )
            {
               rlim.rlim_cur = (UINT64)configsObj.getField( limitItem ).numberLong() ;
            }
            else
            {
               try
               {
                  string valStr = configsObj.getStringField( limitItem ) ;
                  rlim.rlim_cur = boost::lexical_cast<UINT64>( valStr ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_INVALIDARG ;
                  stringstream ss ;
                  ss << configsObj.getStringField( limitItem )
                     << " could not be interpreted as number" ;
                  err = ss.str() ;
                  goto error ;
               }
            }

            if ( 0 != setrlimit( resourceType[ index ], &rlim ) )
            {
               if ( EINVAL == errno )
               {
                  rc = SDB_INVALIDARG ;
                  err = "Invalid argument: " + string( limitItem ) ;
                  PD_LOG( PDERROR, "Invalid argument, argument: %s",
                          limitItem ) ;
                  goto error ;
               }
               else if ( EPERM == errno )
               {
                  rc = SDB_PERM ;
                  err = "Permission error" ;
                  PD_LOG( PDERROR, "Permission error" ) ;
                  goto error ;
               }
               else
               {
                  rc = SDB_SYS ;
                  err = "Failed to set ulimit configs" ;
                  PD_LOG( PDERROR, "Failed to set ulimit configs" ) ;
                  goto error ;
               }
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::runService( const string& serviceName,
                                          const string& command,
                                          const string& options,
                                          string &err,
                                          string& retStr )
   {
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

#if defined (_LINUX)
      cmd << "service " << serviceName << " " << command ;
#elif defined (_WINDOWS)
      cmd << "sc " << command << " " << serviceName ;
#endif

      if ( !options.empty() )
      {
         cmd << " " << options ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
      if( '\n' == outStr[ outStr.size() - 1 ]  )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }

      retStr = outStr ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::createSshKey( string &err )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             homePath ;
      string             cmd ;
      // get home path
      rc = getHomePath( homePath, err ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get home path, rc: %d", rc ) ;
         goto error ;
      }

      cmd = "echo -e \"n\" | ssh-keygen -t rsa -f " + homePath +
            "/.ssh/id_rsa -N \"\" " ;
      // create Ssh key
      rc = runner.exec( cmd.c_str(),
                        exitCode, FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd
            << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystemCommon::getHomePath( string &homePath, string &err )
   {
      INT32              rc = SDB_OK ;
      string             cmd ;
      _ossCmdRunner      runner ;
      UINT32             exitCode = 0 ;
      string             outStr ;

#if defined (_LINUX)
      cmd = "whoami 2>/dev/null" ;
#elif defined (_WINDOWS)
      cmd = "cmd /C set HOMEPATH" ;
#endif
      // run cmd
      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to exec cmd" ;
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read msg from cmd runner" ;
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         goto error ;
      }
      if( !outStr.empty() && outStr[ outStr.size() - 1 ] == '\n' )
      {
#if defined (_LINUX)
         // erase /n
         outStr.erase( outStr.size()-1, 1 ) ;
#elif defined (_WINDOWS)
         // erase /r/n
         outStr.erase( outStr.size()-2, 2 ) ;
#endif
      }

#if defined (_LINUX)
      {
         OSSUID uid = 0 ;
         OSSGID gid = 0 ;
         struct passwd *pw = NULL ;
         rc = ossGetUserInfo( outStr.c_str(), uid, gid ) ;
         if( SDB_OK != rc )
         {
            err = "Failed to get user info" ;
            PD_LOG( PDERROR, "Failed to get user info, rc: %d", rc ) ;
            goto error ;
         }

         pw = getpwuid( uid ) ;
         if( NULL == pw )
         {
            err = "Failed to get pwuid" ;
            PD_LOG( PDERROR, "Failed to getpwuid" ) ;
            goto error ;
         }
         homePath = pw->pw_dir ;
      }
#elif defined (_WINDOWS)
      {
         vector< string > splited ;
         try
         {
            boost::algorithm::split( splited, outStr,
                                     boost::is_any_of( "=" ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            err = "Failed to split result" ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         homePath = splited[ 1 ] ;
         for( UINT32 index = 2; index < splited.size(); index++ )
         {
            homePath += splited[ index ] ;
         }
      }
#endif
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::getUserEnv( string &err, BSONObj &retObj )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder      builder ;
      UINT32 exitCode     = 0 ;
      string              cmd ;
      _ossCmdRunner       runner ;
      string              outStr ;

// build cmd
#if defined (_LINUX)
      cmd = "env" ;
#elif defined (_WINDOWS)
      cmd = "cmd /C set" ;
#endif

      // run cmd
      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      // extract result
      rc = _extractEnvInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract env info" ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::getPID( UINT32 &pid, string &err )
   {
      pid = (UINT32)ossGetCurrentProcessID() ;
      return SDB_OK ;
   }

   INT32 _sptUsrSystemCommon::getTID( UINT32 &tid, string &err )
   {
      tid = (UINT32)ossGetCurrentThreadID() ;
      return SDB_OK ;
   }

   INT32 _sptUsrSystemCommon::getEWD( string &ewd, string &err )
   {
      INT32 rc = SDB_OK ;
      CHAR buf[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      rc = ossGetEWD( buf, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         err = "Get current executable file's working directory failed" ;
         goto error ;
      }
      ewd = buf ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined( _LINUX )
   INT32 _sptUsrSystemCommon::_extractProcessInfo( const CHAR *buf,
                                                   const BOOLEAN &showDetail,
                                                   BSONObjBuilder &builder )
   {
      INT32 rc          = SDB_OK ;
      vector<string>    splited ;
      vector<BSONObj>   procVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
      USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
      root         1  0.0  0.0  84096  1352 ?        Ss   Jun12   0:06 /sbin/init
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }

            // filename may contain ' ', need to merge
            for ( UINT32 index = 4; index < columns.size(); index++ )
            {
               columns[ 3 ] += " " + columns[ index ] ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_USER, columns[ 0 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_STATUS, columns[ 2 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 3 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }

            // filename may contain ' ', need to merge
            for ( UINT32 index = 4; index < columns.size(); index++ )
            {
               columns[ 3 ] += " " + columns[ index ] ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 3 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj > into BsonObj
      for( UINT32 index = 0; index < procVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            procVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }
#elif defined( _WINDOWS )
   INT32 _sptUsrSystemCommon::_extractProcessInfo( const CHAR *buf,
                                                   const BOOLEAN &showDetail,
                                                   BSONObjBuilder &builder )
   {
      INT32 rc            = SDB_OK ;
      vector<string>      splited ;
      vector< BSONObj >   procVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
      System Idle Process","0","Services","0","24 K"
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ",\"" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result must contain 5 col
            if ( 5 != columns.size() )
            {
               continue ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_USER, "" ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_STATUS, "" ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 0 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ",\"" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result must contain 5 col
            if ( 5 != columns.size() )
            {
               continue ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 0 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }

      // merge into BsonObj
      for( UINT32 index = 0; index < procVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            procVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }
#endif

#if defined( _LINUX )
   #define HOSTS_FILE      "/etc/hosts"
#else
   #define HOSTS_FILE      "C:\\Windows\\System32\\drivers\\etc\\hosts"
#endif // _LINUX

   INT32 _sptUsrSystemCommon::_parseHostsFile( VEC_HOST_ITEM & vecItems,
                                               string &err )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      INT64 fileSize = 0 ;
      CHAR *pBuff = NULL ;
      INT64 hasRead = 0 ;

      rc = ossGetFileSizeByName( HOSTS_FILE, &fileSize ) ;
      if ( rc )
      {
         ss << "get file[" << HOSTS_FILE << "] size failed: " << rc ;
         goto error ;
      }
      pBuff = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !pBuff )
      {
         ss << "alloc memory[" << fileSize << "] failed" ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( pBuff, 0, fileSize + 1 ) ;

      rc = ossOpen( HOSTS_FILE, OSS_READONLY|OSS_SHAREREAD, 0,
                    file ) ;
      if ( rc )
      {
         ss << "open file[" << HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      // read file
      rc = ossReadN( &file, fileSize, pBuff, hasRead ) ;
      if ( rc )
      {
         ss << "read file[" << HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      ossClose( file ) ;
      isOpen = FALSE ;

      rc = _extractHosts( pBuff, vecItems ) ;
      if ( rc )
      {
         ss << "extract hosts failed: " << rc ;
         goto error ;
      }

      // remove last empty
      if ( vecItems.size() > 0 )
      {
         VEC_HOST_ITEM::iterator itr = vecItems.end() - 1 ;
         usrSystemHostItem &info = *itr ;
         if ( info.toString().empty() )
         {
            vecItems.erase( itr ) ;
         }
      }

   done:
      if ( isOpen )
      {
         ossClose( file ) ;
      }
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      err = ss.str() ;
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_writeHostsFile( VEC_HOST_ITEM & vecItems,
                                               string & err )
   {
      INT32 rc = SDB_OK ;
      std::string tmpFile = HOSTS_FILE ;
      tmpFile += ".tmp" ;
      OSSFILE file ;
      BOOLEAN isOpen = FALSE ;
      BOOLEAN isBak = FALSE ;
      stringstream ss ;

      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

      // 1. first back up the file
      if ( SDB_OK == ossAccess( HOSTS_FILE ) )
      {
         if ( SDB_OK == ossRenamePath( HOSTS_FILE, tmpFile.c_str() ) )
         {
            isBak = TRUE ;
         }
      }

      // 2. Create the file
      rc = ossOpen ( HOSTS_FILE, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_RU|OSS_WU|OSS_RG|OSS_RO, file ) ;
      if ( rc )
      {
         ss << "open file[" <<  HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      // 3. write data
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         UINT32 count = 0 ;
         while ( it != vecItems.end() )
         {
            ++count ;
            usrSystemHostItem &item = *it ;
            ++it ;
            string text = item.toString() ;
            if ( !text.empty() || count < vecItems.size() )
            {
               text += OSS_NEWLINE ;
            }
            rc = ossWriteN( &file, text.c_str(), text.length() ) ;
            if ( rc )
            {
               ss << "write context[" << text << "] to file[" << HOSTS_FILE
                  << "] failed: " << rc ;
               goto error ;
            }
         }
      }

      // 4. remove tmp
      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

   done:
      if ( isOpen )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      if ( isBak )
      {
         if ( isOpen )
         {
            ossClose( file ) ;
            isOpen = FALSE ;
            ossDelete( HOSTS_FILE ) ;
         }
         ossRenamePath( tmpFile.c_str(), HOSTS_FILE ) ;
      }
      err = ss.str() ;
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractHosts( const CHAR *buf,
                                             VEC_HOST_ITEM &vecItems )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      if ( splited.empty() )
      {
         goto done ;
      }

      for ( vector<string>::iterator itr = splited.begin() ;
            itr != splited.end() ;
            itr++ )
      {
         usrSystemHostItem item ;

         if ( itr->empty() )
         {
            vecItems.push_back( item ) ;
            continue ;
         }
         vector<string> columns ;

         try
         {
            boost::algorithm::trim( *itr ) ;
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end();
                /// do not ++
               )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         /// xxx.xxx.xxx.xxx xxxx
         /// xxx.xxx.xxx.xxx xxxx.xxxx xxxx
         if ( 2 != columns.size() && 3 != columns.size() )
         {
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         if ( !isValidIPV4( columns.at( 0 ).c_str() ) )
         {
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         item._ip = columns[ 0 ] ;
         if ( columns.size() == 3 )
         {
            item._com = columns[ 1 ] ;
            item._host = columns[ 2 ] ;
         }
         else
         {
            item._host = columns[ 1 ] ;
         }
         item._lineType = LINE_HOST ;
         vecItems.push_back( item ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptUsrSystemCommon::_buildHostsResult( VEC_HOST_ITEM & vecItems,
                                                BSONObjBuilder &builder )
   {
      BSONArrayBuilder arrBuilder ;
      VEC_HOST_ITEM::iterator it = vecItems.begin() ;
      while ( it != vecItems.end() )
      {
         usrSystemHostItem &item = *it ;
         ++it ;

         if ( LINE_HOST != item._lineType )
         {
            continue ;
         }
         arrBuilder << BSON( CMD_USR_SYSTEM_IP << item._ip <<
                             CMD_USR_SYSTEM_HOSTNAME << item._host ) ;
      }
      builder.append( CMD_USR_SYSTEM_HOSTS, arrBuilder.arr() ) ;
   }


#if defined (_LINUX)
   #if defined (_ARMLIN64)
   INT32 _sptUsrSystemCommon::_extractCpuInfo( const CHAR *buf,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      UINT32 coreNum = 0 ;
      string strModelName = "model name" ;
      string modelName ;
      vector<string> splited ;
      vector<string>::iterator iter ;
      BSONArrayBuilder arrBuilder ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( iter = splited.begin(); iter != splited.end(); )
      {
         if( iter->empty() )
         {
            iter = splited.erase( iter ) ;
         }
         else
         {
            ++iter ;
         }
      }

      for ( iter = splited.begin(); iter != splited.end(); ++iter )
      {
         // *iter is in the format of "xxx : xx", so let's
         // split it with ":"
         vector<string> columns ;
         vector<string>::iterator iter2 ;

         try
         {
            boost::algorithm::split( columns, *iter, boost::is_any_of(":") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         for ( iter2 = columns.begin(); iter2 != columns.end(); ++iter2 )
         {
            boost::algorithm::trim( *iter2 ) ;
         }

         if ( strModelName == columns.at( 0 ) )
         {
            if ( modelName.empty() )
            {
               modelName = columns.at( 1 ) ;
            }
         }
         else
         {
            continue ;
         }

         ++coreNum ;
      }

      arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum <<
                          CMD_USR_SYSTEM_INFO << modelName <<
                          CMD_USR_SYSTEM_FREQ << "" ) ;

      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
   #elif defined (_PPCLIN64)
   INT32 _sptUsrSystemCommon::_extractCpuInfo( const CHAR *buf,
                                              BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      // extract the follow 3 fields from the return content
      string strProcessor  = "processor" ;
      string strCpu        = "cpu" ;
      string strClock      = "clock" ;
      string strMachine    = "machine" ;
      // use to record the frequency of those 3 fields
      INT32 processorCount = 0 ;
      INT32 cpuCount       = 0 ;
      INT32 clockCount     = 0 ;
      INT32 machineCount   = 0 ;
      string modelName     = "" ;
      string machine       = "" ;
      vector<string> splited ;
      vector<string> vecFreq ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); // don't itr++
          )
      {
         if( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); itr++ )
      {
         // *itr is in the format of "xxx : xx", so let's
         // split it with ":"
         vector<string> columns ;

         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of(":") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end(); itr2++ )
         {
            boost::algorithm::trim( *itr2 ) ;
         }
         if ( strProcessor == columns.at(0) )
         {
            processorCount++ ;
         }
         else if ( strCpu == columns.at(0) )
         {
            if ( modelName == "" )
            {
               modelName = columns.at(1) ;
            }
            cpuCount++ ;
         }
         else if ( strClock== columns.at(0) )
         {
            vecFreq.push_back( columns.at(1) ) ;
            clockCount++ ;
         }
         else if ( strMachine == columns.at(0) )
         {
            machine = columns.at(1) ;
            machineCount = 1 ;
         }
         else
         {
            PD_LOG( PDERROR, "unexpect field[%s]", columns.at(0).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // check and keep the cpu info
         if ( 1 == machineCount )
         {
            if ( processorCount != cpuCount ||
                 cpuCount != clockCount ||
                 clockCount != processorCount )
            {
               PD_LOG( PDERROR, "unexpect cpu info[%s]", buf ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            // merge cpu info
            {
               UINT32 coreNum    = processorCount ;
               string info       = modelName ;
               string strAvgFreq ;
               FLOAT32 totalFreq = 0.0 ;

               for ( vector<string>::iterator itr2 = vecFreq.begin();
                     itr2 != vecFreq.end(); itr2++ )
               {
                  string freq = *itr2 ;
                  try
                  {
                     boost::algorithm::replace_last( freq, "MHz", "" ) ;
                     FLOAT32 inc = boost::lexical_cast<FLOAT32>( freq ) ;
                     totalFreq += inc / 1000.0 ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDERROR, "unexpected err happened:%s, content:[%s]",
                             e.what(), freq.c_str() ) ;
                     rc = SDB_SYS ;
                     goto error ;
                  }
               }
               try
               {
                  strAvgFreq = boost::lexical_cast<string>( totalFreq / coreNum ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "unexpected err happened:%s, content:[%f]",
                          e.what(), totalFreq / coreNum ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum
                                   << CMD_USR_SYSTEM_INFO << info
                                   << CMD_USR_SYSTEM_FREQ << strAvgFreq + "GHz" ) ;
            }
            // clean the counters
            processorCount = 0 ;
            cpuCount       = 0 ;
            clockCount     = 0 ;
            machineCount   = 0 ;
            modelName      = "" ;
            machine        = "" ;
            vecFreq.clear() ;
         }
      }
      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   #else
   INT32 _sptUsrSystemCommon::_extractCpuInfo( const CHAR *buf,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      // extract the follow 4 fields from the return content
      string strModelName  = "model name" ;
      string strFreq       = "cpu MHz" ;
      string strCoreNum    = "cpu cores" ;
      string strPhysicalID = "physical id" ;
      // use to mark which field we had accessed
      INT32 flag           = 0x00000000 ;
      BOOLEAN mustPush ;
      vector<string> splited ;
      vector<cpuInfo> vecCpuInfo ;
      set<string> physicalIDSet ;
      cpuInfo info ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); // don't itr++
          )
      {
         if( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // there is at lease one cpu
      physicalIDSet.insert( "0" ) ;
      info.reset() ;
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         // *itr is in the format of "xxx : xx", so let's
         // split it with ":"
         vector<string> columns ;
         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t:" ),
                                     boost::token_compress_on ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         try
         {
            for ( vector<string>::iterator itr2 = columns.begin();
                  itr2 != columns.end(); itr2++ )
            {
               boost::algorithm::trim( *itr2 ) ;
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to trim, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         mustPush = FALSE ;
         if ( strModelName == columns.at( 0 ) )
         {
            if ( ( flag ^ 0x00000001 ) > flag )
            {
               info.modelName = columns.at( 1 ) ;
               flag ^= 0x00000001 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else if ( strFreq == columns.at( 0 ) )
         {
            if ( ( flag ^ 0x00000010 ) > flag )
            {
               info.freq = columns.at( 1 ) ;
               flag ^= 0x00000010 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else if ( strCoreNum == columns.at( 0 ) )
         {
            if ( ( flag ^ 0x00000100 ) > flag )
            {
               info.coreNum = columns.at( 1 ) ;
               flag ^= 0x00000100 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else if ( strPhysicalID == columns.at(0) )
         {
            if ( ( flag ^ 0x00001000 ) > flag )
            {
               physicalIDSet.insert( columns.at(1) ) ;
               info.physicalID = columns.at(1) ;
               flag ^= 0x00001000 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "unexpect field[%s]", columns.at(0).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // check and keep the cpu info
         if ( TRUE == mustPush )
         {
            vecCpuInfo.push_back( info ) ;
            info.reset() ;
            flag = 0 ;
            // need to decrease itr becase not use the value of itr
            itr-- ;
         }
      }

      // push last info obj
      if ( flag )
      {
         vecCpuInfo.push_back( info ) ;
      }

      // merge the cpu info
      for ( set<string>::iterator itr = physicalIDSet.begin();
            itr != physicalIDSet.end(); itr++ )
      {
         string physicalID = *itr ;
         UINT32 coreNum    = 0 ;
         string modelName  = "" ;
         string strAvgFreq ;
         FLOAT32 totalFreq = 0.0 ;
         for ( vector<cpuInfo>::iterator itr2 = vecCpuInfo.begin();
               itr2 != vecCpuInfo.end(); itr2++ )
         {
            if ( physicalID == itr2->physicalID )
            {
               // sum freq
               try
               {
                  FLOAT32 inc = boost::lexical_cast<FLOAT32>( itr2->freq ) ;
                  totalFreq += inc / 1000.0 ;
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "unexpected err happened:%s, content:[%s]",
                          e.what(), (itr2->freq).c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               // set modelName if it is uninitialized
               if ( modelName == "" )
               {
                  modelName = itr2->modelName ;
               }
               // add core num
               coreNum++ ;
            }
         }
         try
         {
            strAvgFreq = boost::lexical_cast<string>( totalFreq / coreNum ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s, content:[%f]",
                    e.what(), totalFreq / coreNum ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum
                             << CMD_USR_SYSTEM_INFO << modelName
                             << CMD_USR_SYSTEM_FREQ << strAvgFreq + "GHz" ) ;
      }
      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   #endif /// _PPCLIN64
#endif // _LINUX

#if defined (_WINDOWS)
   INT32 _sptUsrSystemCommon::_extractCpuInfo( const CHAR *buf,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      vector<string> splited ;
      INT32 lineCount = 0 ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         ++lineCount ;
         if ( 1 == lineCount || itr->empty() )
         {
            continue ;
         }
         vector<string> columns ;

         try
         {
            boost::algorithm::trim( *itr ) ;
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end();
               /// do not ++
               )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         /// eg: 3200 AMD Athlon(tm) II X2 B26 Processor 2
         if ( columns.size() < 3 )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         UINT32 coreNum = 0 ;
         stringstream info ;

         try
         {
            coreNum = boost::lexical_cast<UINT32>(
               columns.at( columns.size() - 1 ) ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         for ( UINT32 i = 1; i < columns.size() - 1 ; i++ )
         {
            info << columns.at( i ) << " " ;
         }

         arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum
                             << CMD_USR_SYSTEM_INFO << info.str()
                             << CMD_USR_SYSTEM_FREQ << columns[ 0 ] ) ;
      }

      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
#endif //_WINDOWs

   INT32 _sptUsrSystemCommon::_extractMemInfo( const CHAR *buf,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\t ") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            /// do not ++
          )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            ++itr ;
         }
      }
      /// Mem:       8194232    2373776    5820456          0     387924     992756
      /// choose total used free
      if ( splited.size() < 4 )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         builder.append( CMD_USR_SYSTEM_SIZE,
                         boost::lexical_cast<UINT32>(splited.at( 1 ) ) ) ;
         builder.append( CMD_USR_SYSTEM_USED,
                         boost::lexical_cast<UINT32>(splited.at( 2 ) ) ) ;
         builder.append( CMD_USR_SYSTEM_FREE,
                         boost::lexical_cast<UINT32>(splited.at( 3) ) ) ;
         builder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractNetcards( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      BSONArrayBuilder arrBuilder ;

#if defined (_WINDOWS)
      PIP_ADAPTER_INFO pAdapterInfo = NULL ;
      DWORD dwRetVal = 0 ;
      ULONG ulOutbufLen = sizeof( PIP_ADAPTER_INFO ) ;

      pBuff = (CHAR*)SDB_OSS_MALLOC( ulOutbufLen ) ;
      if ( !pBuff )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      pAdapterInfo = (PIP_ADAPTER_INFO)pBuff ;

      // first call GetAdapterInfo to get ulOutBufLen size
      dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutbufLen ) ;
      if ( dwRetVal == ERROR_BUFFER_OVERFLOW )
      {
         SDB_OSS_FREE( pBuff ) ;
         pBuff = ( CHAR* )SDB_OSS_MALLOC( ulOutbufLen ) ;
         if ( !pBuff )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         pAdapterInfo = (PIP_ADAPTER_INFO)pBuff ;
         dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutbufLen ) ;
      }

      if ( dwRetVal != NO_ERROR )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         PIP_ADAPTER_INFO pAdapter = pAdapterInfo ;
         while ( pAdapter )
         {
            stringstream ss ;
            ss << "eth" << pAdapter->Index ;
            arrBuilder << BSON( CMD_USR_SYSTEM_NAME << ss.str()
                                << CMD_USR_SYSTEM_IP <<
                                pAdapter->IpAddressList.IpAddress.String ) ;
            pAdapter = pAdapter->Next ;
         }
      }
#elif defined (_LINUX)
      struct ifconf ifc ;
      struct ifreq *ifreq = NULL ;
      INT32 sock = -1 ;

      pBuff = ( CHAR* )SDB_OSS_MALLOC( 1024 ) ;
      if ( !pBuff )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ifc.ifc_len = 1024 ;
      ifc.ifc_buf = pBuff;

      if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
      {
         PD_LOG( PDERROR, "failed to init socket" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( SDB_OK != ioctl( sock, SIOCGIFCONF, &ifc ) )
      {
         close( sock ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "failed to call ioctl" ) ;
         goto error ;
      }

      ifreq = ( struct ifreq * )pBuff ;
      for ( INT32 i = ifc.ifc_len / sizeof(struct ifreq);
            i > 0;
            --i )
      {
         arrBuilder << BSON( CMD_USR_SYSTEM_NAME << ifreq->ifr_name
                             << CMD_USR_SYSTEM_IP <<
                             inet_ntoa(((struct sockaddr_in*)&
                                         (ifreq->ifr_addr))->sin_addr) ) ;
         ++ifreq ;
      }
      close( sock ) ;
#endif
      builder.append( CMD_USR_SYSTEM_NETCARDS, arrBuilder.arr() ) ;
   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractLoginUsersInfo( const CHAR *buf,
                                                      BSONObjBuilder &builder,
                                                      BOOLEAN showDetail )
   {
      INT32 rc            = SDB_OK ;
      vector<string>      splited ;
      vector< BSONObj >   userVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
            xxxxxxxxx tty          2016-10-11 13:01
         or
            xxxxxxxxx pts/0        2016-10-11 13:01 (xxx.xxx.xxx.xxx)
         or
            xxxxxxxxx pts/1        Dec  2 11:44     (192.168.10.53)

      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;
            string loginIp ;
            string loginTime ;
            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }
            else
            {
               string &ipStr = columns.back() ;
               if ( ipStr[ ipStr.size() - 1 ] == ')' )
               {
                  loginIp = ipStr.substr( 1, ipStr.size() - 2 );
                  loginTime = columns[ 2 ] ;
                  for ( UINT32 index = 3; index < columns.size() - 1; index++ )
                  {
                     loginTime += " " + columns[ index ] ;
                  }
               }
               else
               {
                  loginIp = "" ;
                  loginTime = columns[ 2 ] ;
                  for( UINT32 index = 3; index < columns.size(); index++ )
                  {
                     loginTime += " " + columns[ index ] ;
                  }
               }
            }
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_USER, columns[ 0 ] ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_TIME, loginTime ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_FROM, loginIp ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_TTY, columns[ 1 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_USER,
                                  columns[ 0 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj > into BsonObj
      for( UINT32 index = 0; index < userVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            userVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractAllUsersInfo( const CHAR *buf,
                                                    BSONObjBuilder &builder,
                                                    BOOLEAN showDetail )
   {
      INT32 rc           = SDB_OK ;
      vector<string>     splited ;
      vector< BSONObj >  userVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
         root:x:0:0:root:/root:/bin/bash
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ":" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }

            // if no match format
            if ( columns.size() != 7 )
            {
               continue ;
            }
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_USER, columns[ 0 ] ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_GID, columns[ 3 ] ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_DIR, columns[ 5 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ":" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }

            // if no match format
            if ( columns.size() != 7 )
            {
               continue ;
            }
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_USER, columns[ 0 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj > into BsonObj
      for( UINT32 index = 0; index < userVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            userVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractGroupsInfo( const CHAR *buf,
                                                  BSONObjBuilder &builder,
                                                  BOOLEAN showDetail )
   {
      INT32 rc           = SDB_OK ;
      vector<string>     splited ;
      vector< BSONObj >  groupVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
         cdrom:x:24:sequoiadb
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder groupObjBuilder ;
            string groupMem ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(":") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // if no match format
            if ( columns.size() < 3 )
            {
               continue ;
            }

            // if group has list of user
            if ( columns.size() == 4 )
            {
               groupMem = columns[ 3 ] ;
            }
            else
            {
               groupMem = "" ;
            }
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_NAME, columns[ 0 ] ) ;
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_GID, columns[ 2 ] ) ;
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_MEMBERS, groupMem ) ;
            groupVec.push_back( groupObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin() ;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder groupObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ":" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // if no match format
            if ( columns.size() < 3 )
            {
               continue ;
            }
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_NAME, columns[ 0 ] ) ;
            groupVec.push_back( groupObjBuilder.obj() ) ;
         }
      }

      // merge into BsonObj
      for( UINT32 index = 0; index < groupVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            groupVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_getSystemInfo( vector< string > typeSplit,
                                              BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 bufLen = 1024 + 1 ;
      multimap< string, string > fileMap ;
      vector< string > keySplit ;
      vector< string > valueSplit ;
      CHAR *buf = NULL ;
      INT32 increaseLen = 1024 ;

      buf = (CHAR*) SDB_OSS_MALLOC( bufLen ) ;
      if( NULL == buf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "buf malloc failed" ) ;
         goto error ;
      }

      for( vector< string >::iterator itr = typeSplit.begin();
           itr != typeSplit.end(); itr++ )
      {
         string searchDir = "/proc/sys/" + (*itr) ;
         rc = ossAccess( searchDir.c_str() ) ;
         if( SDB_OK != rc )
         {
            rc = SDB_OK ;
            continue ;
         }

         rc = ossEnumFiles( searchDir, fileMap, "", 10 ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      for( multimap< string, string >::iterator itr = fileMap.begin();
           itr != fileMap.end();
           itr++ )
      {
         ossPrimitiveFileOp op ;
         string key ;
         string value ;
         keySplit.clear() ;
         valueSplit.clear() ;

         // open file
         rc = op.Open( itr->second.c_str(), OSS_PRIMITIVE_FILE_OP_READ_ONLY ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Can't open file: %s", itr->second.c_str() ) ;
            continue ;
         }

         // read file content
         {
            INT32 readByte = 0 ;
            INT32 hasRead = 0 ;
            INT32 readLen = bufLen-1 ;
            CHAR *curPos = buf ;
            BOOLEAN finishRead = FALSE ;
            BOOLEAN isReadSuccess = TRUE ;

            while( !finishRead )
            {
               rc = op.Read( readLen, curPos , &readByte ) ;
               if( SDB_OK != rc || 0 > readByte )
               {
                  PD_LOG( PDERROR, "Failed to read file: %s",
                          itr->second.c_str() ) ;
                  isReadSuccess = FALSE ;
                  break ;
               }
               hasRead += readByte ;
               curPos = buf + hasRead ;
               // mem not enough, need to realloc, newBuffSize = 2*oldBuffSize
               if ( readByte == readLen )
               {
                  INT32 newBufLen = bufLen + increaseLen ;
                  CHAR *pNewBuf = (CHAR*)SDB_OSS_REALLOC( buf, newBufLen ) ;
                  if ( NULL == pNewBuf )
                  {
                     rc = SDB_OOM ;
                     PD_LOG( PDERROR, "Failed to realloc buff" ) ;
                     goto error ;
                  }
                  bufLen = newBufLen ;
                  buf = pNewBuf ;
                  readLen = increaseLen ;
                  increaseLen *= 2 ;
               }
               else
               {
                  finishRead = TRUE ;
               }
            }
            if( FALSE == isReadSuccess )
            {
               continue ;
            }
            buf[ hasRead ] = '\0' ;
         }

         // split key
         try
         {
            boost::algorithm::split( keySplit, itr->second,
                                     boost::is_any_of( "/" ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         for( std::vector< string >::iterator vecItr = keySplit.begin();
              vecItr != keySplit.end(); )
         {
            if ( vecItr->empty() )
            {
               vecItr = keySplit.erase( vecItr ) ;
            }
            else
            {
               vecItr++ ;
            }
         }

         if ( keySplit.size() < 3 )
         {
            continue ;
         }

         key = *( keySplit.begin()+2 ) ;
         for( std::vector< string >::iterator vecItr = keySplit.begin()+3;
              vecItr != keySplit.end(); vecItr++ )
         {
            key += "." + ( *vecItr ) ;
         }

         // split value
         try
         {
            boost::algorithm::split( valueSplit, buf,
                                     boost::is_any_of( "\r\n" ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         for( std::vector< string >::iterator vecItr = valueSplit.begin();
              vecItr != valueSplit.end(); )
         {
            if ( vecItr->empty() )
            {
               vecItr = valueSplit.erase( vecItr ) ;
            }
            else
            {
               boost::replace_all( *vecItr, "\t", "    " ) ;
               vecItr++ ;
            }
         }

         if ( valueSplit.size() > 0 )
         {
            value = *( valueSplit.begin() ) ;
            for( std::vector< string >::iterator vecItr = valueSplit.begin()+1;
               vecItr != valueSplit.end(); vecItr++ )
            {
               value += ";" + ( *vecItr ) ;
            }
         }
         else
         {
            value = "" ;
         }
         builder.append( key, value ) ;
      }
   done:
      SDB_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystemCommon::_extractEnvInfo( const CHAR *buf,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector< string > splited ;
      vector< pair< string, string > > envVec ;
      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
         PWD=/home/users/wujiaming
         LANG=en_US.UTF-8
         SHLVL=1
         HOME=/root
         LANGUAGE=en_US:en
         LOGNAME=root
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
      {
         vector<string> columns ;
         string value ;

         // if no contain '=', it is the part of previous row
         if ( std::string::npos == (*itrSplit).find( "=" ) )
         {
            if ( envVec.size() )
            {
               envVec.back().second += *itrSplit ;
            }
            continue ;
         }

         try
         {
            boost::algorithm::split( columns, *itrSplit,
                                     boost::is_any_of("=") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itrCol = columns.begin() ;
               itrCol != columns.end(); )
         {
            if ( itrCol->empty() )
            {
               itrCol = columns.erase( itrCol ) ;
            }
            else
            {
               itrCol++ ;
            }
         }

         // at least conatain 1 cols
         if ( columns.size() < 1 )
         {
            continue ;
         }
         else if ( columns.size() == 1 )
         {
            value = "" ;
         }
         else
         {
            value = *( columns.begin() + 1 ) ;
            /*
               may contain result like "LS_COLORS=rs=0:di=01;34:ln=01"
               need to merge into a string
            */
            for ( vector<string>::iterator itrCol = columns.begin() + 2 ;
                  itrCol != columns.end(); itrCol++ )
            {
               value += "=" + *itrCol ;
            }
         }
         envVec.push_back( pair< string, string >( *columns.begin(), value ) ) ;
      }

      for ( vector< pair< string, string > >::iterator itr = envVec.begin() ;
            itr != envVec.end();
            itr++ )
      {
         builder.append( itr->first, itr->second ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

