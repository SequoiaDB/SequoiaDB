/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilNodeOpr.cpp

   Descriptive Name =

   When/how to use: node operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2014  XJH Initial Draft

   Last Changed =

******************************************************************************/

#include "utilNodeOpr.hpp"
#include "pmdDef.hpp"
#include "utilCommon.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "ossPath.hpp"
#include "utilParam.hpp"
#include "pd.hpp"

#if defined( _LINUX )
#include <dirent.h>
#endif //_LINUX

namespace engine
{

   static INT32 _utilCheckOrCleanNamedPipe( const CHAR *fullPipeName,
                                            OSSPID &pid )
   {
#if defined( _LINUX )
      INT32 rc = SDB_OK ;
      const CHAR *pPidPtr = NULL ;

      pPidPtr = ossStrrchr( fullPipeName, '_' ) ;
      if ( !pPidPtr || 0 == *( pPidPtr + 1 ) ||
           0 == ( pid = ossAtoi( pPidPtr + 1 ) ) )
      {
         goto done ;
      }
      if ( ossIsProcessRunning( pid ) )
      {
         rc = SDB_FE ;
         goto error ;
      }
      else
      {
         rc = ossCleanNamedPipeByName( fullPipeName ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
#else
      return SDB_FE ;
#endif // _LINUX
   }

   #define UTIL_NODE_PIPE_TIMEOUT         ( 1000 )
   #define UTIL_NODE_OPEN_PIPE_TIMEOUT    ( 0 )
   #define UTIL_NODE_PIPE_BUFFSZ          ( 1024 )

   /*
      _utilNodePipe implement
   */
   _utilNodePipe::_utilNodePipe()
   {
      _isCreate = FALSE ;
      _isOpen   = FALSE ;
      _isConnect= FALSE ;
      _connectError = FALSE ;

      ossMemset( _pipeRName, 0, sizeof( _pipeRName ) ) ;
      ossMemset( _pipeWName, 0, sizeof( _pipeWName ) ) ;
   }

   _utilNodePipe::~_utilNodePipe()
   {
      autoRelease() ;
   }

   INT32 _utilNodePipe::autoRelease()
   {
      INT32 rc = SDB_OK ;

      if ( _isCreate )
      {
         rc = deletePipe() ;
      }
      if ( _isOpen )
      {
         rc = closePipe() ;
      }
      return rc ;
   }

   INT32 _utilNodePipe::createPipe( const CHAR * svcname )
   {
      INT32 rc = SDB_OK ;

      rc = autoRelease() ;
      if ( rc )
      {
         goto error ;
      }

#if defined( _WINDOWS )
      ossSnprintf ( _pipeRName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX"%s",
                    svcname ) ;
      ossSnprintf ( _pipeWName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX"%s",
                    svcname ) ;
      rc = ossCreateNamedPipe( _pipeRName, UTIL_NODE_PIPE_BUFFSZ,
                               UTIL_NODE_PIPE_BUFFSZ,
                               OSS_NPIPE_DUPLEX | OSS_NPIPE_BLOCK_WITH_TIMEOUT,
                               OSS_NPIPE_UNLIMITED_INSTANCES,
                               UTIL_NODE_PIPE_TIMEOUT, _pipeRHandle ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create named pipe[%s] failed, rc: %d",
                 _pipeRName, rc ) ;
         goto error ;
      }
#else
      ossSnprintf ( _pipeRName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX"%s_%d",
                    svcname, ossGetCurrentProcessID() ) ;
      ossSnprintf ( _pipeWName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX_BW"%s_%d",
                    svcname, ossGetCurrentProcessID() ) ;

      ossCleanNamedPipeByName( _pipeRName ) ;
      ossCleanNamedPipeByName( _pipeWName ) ;

      {
         vector< string > names ;
         OSSPID pid = OSS_INVALID_PID ;
         ossEnumNamedPipes ( names, ENGINE_NPIPE_PREFIX, OSS_MATCH_LEFT ) ;
         for ( UINT32 i = 0 ; i < names.size() ; ++i )
         {
            _utilCheckOrCleanNamedPipe( names[ i ].c_str(), pid ) ;
         }
      }

      rc = ossCreateNamedPipe( _pipeRName, UTIL_NODE_PIPE_BUFFSZ, 0,
                               OSS_NPIPE_DUPLEX |
                               OSS_NPIPE_BLOCK_WITH_TIMEOUT |
                               OSS_NPIPE_NONBLOCK,
                               OSS_NPIPE_UNLIMITED_INSTANCES,
                               UTIL_NODE_PIPE_TIMEOUT, _pipeRHandle ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create named pipe[%s] failed, rc: %d",
                 _pipeRName, rc ) ;
         goto error ;
      }

      rc = ossCreateNamedPipe( _pipeWName, 0, UTIL_NODE_PIPE_BUFFSZ,
                               OSS_NPIPE_DUPLEX |
                               OSS_NPIPE_BLOCK_WITH_TIMEOUT |
                               OSS_NPIPE_NONBLOCK,
                               OSS_NPIPE_UNLIMITED_INSTANCES,
                               UTIL_NODE_PIPE_TIMEOUT, _pipeWHandle ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create named pipe[%s] failed, rc: %d",
                 _pipeWName, rc ) ;
         ossDeleteNamedPipe( _pipeRHandle ) ;
         goto error ;
      }
#endif // _WINDOWS

      _isCreate = TRUE ;

   done:
      return rc ;
   error:
      autoRelease() ;
      goto done ;
   }

   INT32 _utilNodePipe::deletePipe()
   {
      INT32 rc = SDB_OK ;

      if ( FALSE == _isCreate )
      {
         goto done ;
      }

      disconnectPipe() ;

      rc = ossDeleteNamedPipe( _pipeRHandle ) ;
#ifndef _WINDOWS
      rc = ossDeleteNamedPipe( _pipeWHandle ) ;
#endif
      _isCreate = FALSE ;

   done:
      return rc ;
   }

   void _utilNodePipe::disconnectPipe()
   {
      if ( _isConnect )
      {
         ossDisconnectNamedPipe( _pipeRHandle ) ;
#ifndef _WINDOWS
         ossDisconnectNamedPipe( _pipeWHandle ) ;
#endif
         _isConnect = FALSE ;
      }
   }

   INT32 _utilNodePipe::openPipe( const CHAR * svcname, OSSPID pid )
   {
      INT32 rc = SDB_OK ;

      rc = autoRelease() ;
      if ( rc )
      {
         goto error ;
      }

#if defined( _WINDOWS )
      ossSnprintf ( _pipeRName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX"%s",
                    svcname ) ;
      ossSnprintf ( _pipeWName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX"%s",
                    svcname ) ;
      rc = ossOpenNamedPipe( _pipeRName,
                             OSS_NPIPE_DUPLEX | OSS_NPIPE_BLOCK_WITH_TIMEOUT,
                             UTIL_NODE_OPEN_PIPE_TIMEOUT, _pipeRHandle ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open named pipe[%s] failed, rc: %d",
                 _pipeRName, rc ) ;
         goto error ;
      }
#else
      ossSnprintf ( _pipeWName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX"%s_%d",
                    svcname, pid ) ;
      ossSnprintf ( _pipeRName, OSS_NPIPE_MAX_NAME_LEN,
                    ENGINE_NPIPE_PREFIX_BW"%s_%d",
                    svcname, pid ) ;

      rc = ossOpenNamedPipe( _pipeWName,
                             OSS_NPIPE_DUPLEX |
                             OSS_NPIPE_BLOCK_WITH_TIMEOUT |
                             OSS_NPIPE_NONBLOCK,
                             UTIL_NODE_OPEN_PIPE_TIMEOUT, _pipeWHandle ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open named pipe[%s] failed, rc: %d",
                 _pipeWName, rc ) ;
         goto error ;
      }

      rc = ossOpenNamedPipe( _pipeRName,
                             OSS_NPIPE_DUPLEX |
                             OSS_NPIPE_BLOCK_WITH_TIMEOUT |
                             OSS_NPIPE_NONBLOCK,
                             UTIL_NODE_OPEN_PIPE_TIMEOUT, _pipeRHandle ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open named pipe[%s] failed, rc: %d",
                 _pipeRName, rc ) ;
         ossCloseNamedPipe( _pipeWHandle ) ;
         goto error ;
      }
#endif // _WINDOWS

      _isOpen = TRUE ;

   done:
      return rc ;
   error:
      autoRelease() ;
      goto done ;
   }

   INT32 _utilNodePipe::closePipe()
   {
      INT32 rc = SDB_OK ;

      if ( FALSE == _isOpen )
      {
         goto done ;
      }

      rc = ossCloseNamedPipe( _pipeRHandle ) ;
#ifndef _WINDOWS
      rc = ossCloseNamedPipe( _pipeWHandle ) ;
#endif
      _isOpen = FALSE ;

   done:
      return rc ;
   }

   INT32 _utilNodePipe::writePipe( const CHAR * pBuff, INT32 size )
   {
      INT32 rc = SDB_OK ;
      INT64 bufWrite = 0 ;
      INT64 totalWrite = 0 ;

      _connectError = FALSE ;
      rc = connectPipe() ;
      if ( rc )
      {
         _connectError = TRUE ;
         goto error ;
      }

      while( totalWrite < size )
      {
#if defined( _WINDOWS )
         rc = ossWriteNamedPipe( _pipeRHandle, &pBuff[totalWrite],
                                 size - totalWrite, &bufWrite ) ;
#else
         rc = ossWriteNamedPipe( _pipeWHandle, &pBuff[totalWrite],
                                 size - totalWrite, &bufWrite ) ;
#endif //_WINDOWS
         if ( rc && SDB_INTERRUPT != rc )
         {
            if ( SDB_TIMEOUT != rc )
            {
               PD_LOG( PDERROR, "Write named pipe[%s] failed, rc: %d",
                       _pipeWName, rc ) ;
               disconnectPipe() ;
            }
            goto error ;
         }

         totalWrite += bufWrite ;
         bufWrite = 0 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilNodePipe::connectPipe()
   {
      INT32 rc = SDB_OK ;

      if ( FALSE == _isCreate )
      {
         goto done ;
      }
      if ( _isConnect )
      {
         goto done ;
      }
#if defined ( _WINDOWS )
      rc = ossConnectNamedPipe( _pipeRHandle,
                                OSS_NPIPE_DUPLEX,
                                UTIL_NODE_PIPE_TIMEOUT ) ;
      if ( rc )
      {
         if ( SDB_TIMEOUT != rc )
         {
            PD_LOG( PDERROR, "Connect to named pipe[%s] failed, rc: %d",
                    _pipeRName, rc ) ;
         }
         goto error ;
      }
#else
      rc = ossConnectNamedPipe( _pipeRHandle,
                                OSS_NPIPE_DUPLEX,
                                UTIL_NODE_PIPE_TIMEOUT ) ;
      if ( rc )
      {
         if ( SDB_TIMEOUT != rc )
         {
            PD_LOG( PDERROR, "Connect to named pipe[%s] failed, rc: %d",
                    _pipeRName, rc ) ;
         }
         goto error ;
      }

      rc = ossConnectNamedPipe( _pipeWHandle,
                                OSS_NPIPE_DUPLEX,
                                UTIL_NODE_PIPE_TIMEOUT ) ;
      if ( rc )
      {
         if ( SDB_TIMEOUT != rc )
         {
            PD_LOG( PDERROR, "Connect to named pipe[%s] failed, rc: %d",
                    _pipeWName, rc ) ;
         }
         ossDisconnectNamedPipe( _pipeRHandle ) ;
         goto error ;
      }
#endif // _WINDOWS

      _isConnect = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilNodePipe::readPipe( CHAR *pBuff, INT32 readSize, INT32 &hasRead )
   {
      INT32 rc = SDB_OK ;
      INT64 buffRead = 0 ;
      INT64 totalRead = 0 ;

      _connectError = FALSE ;
      rc = connectPipe() ;
      if ( rc )
      {
         _connectError = TRUE ;
         goto error ;
      }

      while( totalRead < readSize )
      {
         rc = ossReadNamedPipe( _pipeRHandle, &pBuff[totalRead],
                                readSize - totalRead, &buffRead,
                                UTIL_NODE_PIPE_TIMEOUT ) ;
         if ( rc && SDB_INTERRUPT != rc )
         {
            if ( SDB_TIMEOUT != rc )
            {
               PD_LOG ( PDERROR, "Read named pipe[%s] failed, rc: %d",
                        _pipeRName, rc ) ;
               disconnectPipe() ;
            }
            goto error ;
         }
         totalRead += buffRead ;
         buffRead = 0 ;

         if ( SDB_OK == rc )
         {
            break ;
         }
         rc = SDB_OK ;
      }
      hasRead = (INT32)totalRead ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      Local define
   */
   static INT32 _utilWriteReadPipe( const CHAR *pSvcName, OSSPID pid,
                                    const CHAR *pWriteBuf, INT32 writeLen,
                                    CHAR *pReadBuf, INT32 readLen,
                                    BOOLEAN checkLen )
   {
      INT32 rc = SDB_OK ;
      utilNodePipe nodePipe ;
      INT32 hasRead = 0 ;

      rc = nodePipe.openPipe( pSvcName, pid ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG ( PDERROR, "Failed to open named pipe: %s, rc: %d",
                  nodePipe.getReadPipeName(), rc ) ;
         goto error ;
      }

      rc = nodePipe.writePipe( pWriteBuf, writeLen ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send %s to %s, rc: %d",
                  pWriteBuf, nodePipe.getWritePipeName(), rc ) ;
         goto error ;
      }

      if ( !pReadBuf || 0 == readLen )
      {
         goto done ;
      }

      rc = nodePipe.readPipe( pReadBuf, readLen, hasRead ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read %s return from pipe %s, rc: %d",
                  pWriteBuf, nodePipe.getReadPipeName(), rc ) ;
         goto error ;
      }

      if ( checkLen && readLen != hasRead )
      {
         PD_LOG ( PDERROR, "Failed to read %s return from pipe %s, rc: %d",
                  pWriteBuf, nodePipe.getReadPipeName(), rc ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      nodePipe.closePipe() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilWriteReadPipe( const CHAR *pSvcName, OSSPID pid,
                            const CHAR *pWriteBuf, INT32 writeLen,
                            CHAR *pReadBuf, INT32 readLen,
                            BOOLEAN checkLen )
   {
      return _utilWriteReadPipe( pSvcName, pid, pWriteBuf, writeLen,
                                 pReadBuf, readLen, checkLen ) ;
   }

   INT32 utilGetNodeExtraInfo( utilNodeInfo & info )
   {
      INT32 rc = SDB_OK ;
      CHAR groupName[ OSS_MAX_GROUPNAME_SIZE + 1 ] = { 0 } ;
      CHAR dbPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      info._groupID     = 0 ;
      info._nodeID      = 0 ;
      info._primary     = -1 ;
      info._isAlone     = 0 ;
      info._dbPath      = "" ;
      info._groupName   = "" ;
      info._startTime   = 0 ;

      rc = _utilWriteReadPipe( info._svcname.c_str(), info._pid,
                               ENGINE_NPIPE_MSG_GID,
                               sizeof( ENGINE_NPIPE_MSG_GID ),
                               (CHAR *)&info._groupID,
                               sizeof( info._groupID ),
                               TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _utilWriteReadPipe( info._svcname.c_str(), info._pid,
                               ENGINE_NPIPE_MSG_NID,
                               sizeof( ENGINE_NPIPE_MSG_NID ),
                               (CHAR *)&info._nodeID,
                               sizeof( info._nodeID ),
                               TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _utilWriteReadPipe( info._svcname.c_str(), info._pid,
                               ENGINE_NPIPE_MSG_PRIMARY,
                               sizeof( ENGINE_NPIPE_MSG_PRIMARY ),
                               (CHAR *)&info._primary,
                               sizeof( info._primary ),
                               TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _utilWriteReadPipe( info._svcname.c_str(), info._pid,
                               ENGINE_NPIPE_MSG_GNAME,
                               sizeof( ENGINE_NPIPE_MSG_GNAME ),
                               (CHAR *)groupName,
                               OSS_MAX_GROUPNAME_SIZE,
                               FALSE ) ;
      if ( rc )
      {
         goto error ;
      }
      info._groupName = groupName ;

      rc = _utilWriteReadPipe( info._svcname.c_str(), info._pid,
                               ENGINE_NPIPE_MSG_PATH,
                               sizeof( ENGINE_NPIPE_MSG_PATH ),
                               (CHAR *)dbPath,
                               OSS_MAX_PATHSIZE,
                               FALSE ) ;
      if ( rc )
      {
         goto error ;
      }
      info._dbPath = dbPath ;

      rc = _utilWriteReadPipe( info._svcname.c_str(), info._pid,
                               ENGINE_NPIPE_MSG_STARTTIME,
                               sizeof( ENGINE_NPIPE_MSG_STARTTIME ),
                               (CHAR*)&info._startTime,
                               sizeof( UINT64 ), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   static BOOLEAN _isDigitalStr( const CHAR *pStr )
   {
      if ( NULL == pStr ) return FALSE ;
      while ( *pStr )
      {
         CHAR c = *pStr++ ;
         if ( c < '0' || c > '9' ) return FALSE ;
      }
      return TRUE ;
   }

   INT32 utilListNodes( UTIL_VEC_NODES & nodes,
                        INT32 typeFilter,
                        const CHAR * svcnameFilter,
                        OSSPID pidFilter,
                        INT32 roleFilter,
                        BOOLEAN allowAloneCM )
   {
      INT32 rc                   = SDB_OK ;
      DIR *pDir                  = NULL ;
      struct dirent *pDirent     = NULL ;
      BOOLEAN isOpen = FALSE ;
      CHAR *pStr                 = NULL ;
      INT32 beginType            = SDB_TYPE_DB ;
      CHAR *pSvcBegin            = NULL ;
      CHAR *pSvcEnd              = NULL ;
      UINT32 matchNum            = 0 ;
      utilNodeInfo findNode ;

      pDir = opendir( PROC_PATH ) ;
      PD_CHECK( pDir != NULL, SDB_IO, error, PDERROR,
                "Failed to open the directory:%s, errno=%d",
                PROC_PATH, ossGetLastError() ) ;
      isOpen = TRUE ;

      while( (pDirent = readdir( pDir )) != NULL )
      {
         if ( !_isDigitalStr( pDirent->d_name ) )
         {
            continue ;
         }
         CHAR pathName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossSnprintf( pathName, OSS_MAX_PATHSIZE, PROC_CMDLINE_PATH_FORMAT,
                      pDirent->d_name ) ;
         FILE *fp = NULL ;
         fp = fopen( pathName, "r" ) ;
         if ( !fp )
         {
            continue ;
         }
         CHAR commandLine[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         CHAR *pTmp = fgets ( commandLine, OSS_MAX_PATHSIZE, fp ) ;
         fclose(fp) ;
         if ( NULL == pTmp )
         {
            continue ;
         }

         pStr = NULL ;
         matchNum = 0 ;
         beginType = SDB_TYPE_DB ;
         pSvcBegin = NULL ;
         pSvcEnd = NULL ;

         pSvcBegin = ossStrchr( commandLine, '(' ) ;
         if ( !pSvcBegin )
         {
            continue ;
         }
         pSvcEnd = ossStrchr( pSvcBegin + 1, ')' ) ;
         if ( !pSvcEnd || pSvcEnd - pSvcBegin <= 1 ||
              ossStrlen( pSvcEnd ) > 3  )
         {
            continue ;
         }
         *pSvcEnd = 0 ;
         if ( svcnameFilter && 0 != svcnameFilter &&
              0 != ossStrcmp( pSvcBegin + 1, svcnameFilter ) )
         {
            continue ;
         }

         while ( beginType < SDB_TYPE_MAX )
         {
            pStr = ossStrstr( commandLine,
                              utilDBTypeStr( (SDB_TYPE)beginType ) ) ;
            if ( pStr == commandLine &&
                 (UINT32)( pSvcBegin - pStr) ==
                 (UINT32)ossStrlen( utilDBTypeStr( (SDB_TYPE)beginType ) ) )
            {
               if ( -1 == typeFilter || typeFilter == beginType )
               {
                  ++matchNum ;
                  findNode._type = beginType ;
                  break ;
               }
            }

            if ( typeFilter == beginType )
            {
               break ;
            }
            ++beginType ;
         }
         if ( 0 == matchNum )
         {
            continue ;
         }

         findNode._pid = ossAtoi( pDirent->d_name ) ;
         if ( pidFilter != OSS_INVALID_PID &&
              pidFilter != findNode._pid )
         {
            continue ;
         }

         findNode._role = SDB_ROLE_MAX ;
         if ( ' ' == *( pSvcEnd + 1 ) )
         {
            findNode._role = utilShortStr2DBRole( pSvcEnd + 2 ) ;
         }

         if ( SDB_ROLE_MAX == findNode._role )
         {
            switch( findNode._type )
            {
               case SDB_TYPE_OM :
                  findNode._role = SDB_ROLE_OM ;
                  break ;
               case SDB_TYPE_OMA :
                  findNode._role = SDB_ROLE_OMA ;
                  break ;
               case SDB_TYPE_DB :
                  findNode._role = SDB_ROLE_STANDALONE ;
                  break ;
               default :
                  break ;
            }
         }

         if ( roleFilter != -1 && roleFilter != findNode._role )
         {
            continue ;
         }

         findNode._svcname = string( pSvcBegin + 1 ) ;
         *pSvcEnd = ')' ;
         findNode._orgname = commandLine ;

         utilGetNodeExtraInfo( findNode ) ;

         if ( SDB_TYPE_OMA == findNode._type )
         {
            if ( 0 != findNode._groupID )
            {
               if ( FALSE == allowAloneCM )
               {
                  continue ;
               }
               findNode._groupID = 0 ;
               findNode._isAlone = 1 ;
            }
         }

         nodes.push_back( findNode ) ;

         if ( pidFilter != OSS_INVALID_PID ||
              ( svcnameFilter && 0 != *svcnameFilter ) )
         {
            break ;
         }
      }

   done:
      if ( isOpen )
      {
         closedir( pDir ) ;
      }
      return rc ;
   error:
      goto done ;
   }

#else

   INT32 utilListNodes( UTIL_VEC_NODES & nodes, INT32 typeFilter,
                        const CHAR * svcnameFilter, OSSPID pidFilter,
                        INT32 roleFilter, BOOLEAN allowAloneCM )
   {
      INT32 rc = SDB_OK ;
      vector< string > names ;
      utilNodeInfo findNode ;
      UINT32 prefixLen = ossStrlen( ENGINE_NPIPE_PREFIX ) ;
      OSSPID pid = OSS_INVALID_PID ;

      rc = ossEnumNamedPipes ( names, ENGINE_NPIPE_PREFIX, OSS_MATCH_LEFT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to enum pipes, rc: %d", rc ) ;

      for ( UINT32 i = 0 ; i < names.size() ; ++i )
      {
         findNode._svcname = names[ i ].substr( prefixLen ) ;

         if ( svcnameFilter && 0 != *svcnameFilter &&
              0 != ossStrcmp( findNode._svcname.c_str(), svcnameFilter ) )
         {
            continue ;
         }

         rc = _utilWriteReadPipe( findNode._svcname.c_str(), 0,
                                  ENGINE_NPIPE_MSG_TYPE,
                                  sizeof( ENGINE_NPIPE_MSG_TYPE ),
                                  (CHAR*)&findNode._type,
                                  sizeof( findNode._type ),
                                  TRUE ) ;
         if ( rc )
         {
            continue ;
         }
         if ( -1 != typeFilter && typeFilter != findNode._type )
         {
            continue ;
         }

         rc = _utilWriteReadPipe( findNode._svcname.c_str(), 0,
                                  ENGINE_NPIPE_MSG_PID,
                                  sizeof( ENGINE_NPIPE_MSG_PID ),
                                  (CHAR *)&findNode._pid,
                                  sizeof( findNode._pid ),
                                  TRUE ) ;
         if ( rc )
         {
            continue ;
         }
         if ( pidFilter != OSS_INVALID_PID && pidFilter != findNode._pid )
         {
            continue ;
         }

         rc = _utilWriteReadPipe( findNode._svcname.c_str(), findNode._pid,
                                  ENGINE_NPIPE_MSG_ROLE,
                                  sizeof( ENGINE_NPIPE_MSG_ROLE ),
                                  (CHAR *)&findNode._role,
                                  sizeof( findNode._role ),
                                  TRUE ) ;
         if ( rc )
         {
            continue ;
         }
         if ( roleFilter != -1 && roleFilter != findNode._role )
         {
            continue ;
         }

         findNode._orgname = names[ i ] ;
         utilGetNodeExtraInfo( findNode ) ;

         if ( SDB_TYPE_OMA == findNode._type )
         {
            if ( 0 != findNode._groupID )
            {
               if ( FALSE == allowAloneCM )
               {
                  continue ;
               }
               findNode._groupID = 0 ;
               findNode._isAlone = 1 ;
            }
         }

         nodes.push_back( findNode ) ;

         if ( pidFilter != OSS_INVALID_PID ||
              ( svcnameFilter && 0 != *svcnameFilter ) )
         {
            break ;
         }
      }
      rc = SDB_OK ;

   done:
      return rc ;
   error:
      goto done ;
   }

#endif // _LINUX

   INT32 utilEnumNodes( const string &localPath,
                        UTIL_VEC_NODES &nodes,
                        INT32 typeFilter,
                        const CHAR *svcnameFilter,
                        INT32 roleFilter )
   {
      INT32 rc = SDB_OK ;
      vector< string > allsvcnames ;
      utilNodeInfo node ;
      CHAR confPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      node._orgname = "" ;
      node._pid = OSS_INVALID_PID ;

      if ( SDB_TYPE_OMA != typeFilter )
      {
         rc = ossEnumSubDirs( localPath, allsvcnames, 1 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Enum %s failed, rc: %d", localPath.c_str(),
                    rc ) ;
            goto error ;
        }
      }

      for ( UINT32 i = 0 ; i < allsvcnames.size() ; ++i )
      {
         if ( svcnameFilter && *svcnameFilter &&
              0 != ossStrcmp( svcnameFilter, allsvcnames[ i ].c_str() ) )
         {
            continue ;
         }
         node._svcname = allsvcnames[ i ] ;

         utilBuildFullPath( localPath.c_str(), node._svcname.c_str(),
                            OSS_MAX_PATHSIZE, confPath ) ;

         rc = utilGetRoleByConfigPath( confPath, node._role ) ;
         if ( -1 != roleFilter && ( SDB_OK != rc ||
              roleFilter != node._role ) )
         {
            continue ;
         }

         node._type = utilRoleToType( (SDB_ROLE)node._role ) ;
         if ( -1 != typeFilter && typeFilter != node._type )
         {
            continue ;
         }

         node._dbPath = "" ;
         utilGetDBPathByConfigPath( confPath, node._dbPath ) ;

         nodes.push_back( node ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilWaitNodeOK( utilNodeInfo & node, const CHAR * svcname,
                         OSSPID pid, INT32 typeFilter,
                         INT32 timeout, BOOLEAN allowAloneCM )
   {
      INT32 rc = SDB_OK ;
      UTIL_VEC_NODES nodes ;

      if ( timeout < 0 )
      {
         timeout = 0x7FFFFFFF ;
      }
      else if ( timeout == 0 )
      {
         timeout = 1 ;
      }

      while ( timeout > 0 )
      {
         --timeout ;

         nodes.clear() ;
         rc = utilListNodes( nodes, typeFilter, svcname, pid,
                             -1, allowAloneCM ) ;
         if ( SDB_OK == rc && nodes.size() > 0 )
         {
            node = ( *nodes.begin() ) ;
            rc = SDB_OK ;
            goto done ;
         }

         if ( pid != OSS_INVALID_PID && !ossIsProcessRunning( pid ) )
         {
            PD_LOG( PDERROR, "Process[%d] has exist", pid ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         ossSleep( OSS_ONE_SEC ) ;
      }
      rc = SDB_TIMEOUT ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilStopNode( utilNodeInfo & node, INT32 timeout, BOOLEAN force )
   {
      INT32 rc = SDB_OK ;

      if ( timeout < 0 )
      {
         timeout = 0x7FFFFFFF ;
      }
      else if ( timeout == 0 )
      {
         timeout = 1 ;
      }

#if defined( _LINUX )
      rc = ossTerminateProcess( node._pid, FALSE ) ;
#else
      rc = _utilWriteReadPipe( node._svcname.c_str(), node._pid,
                               ENGINE_NPIPE_MSG_SHUTDOWN,
                               sizeof( ENGINE_NPIPE_MSG_SHUTDOWN ),
                               NULL, 0, FALSE ) ;
#endif // _LINUX
      if ( rc && ossIsProcessRunning( node._pid ) )
      {
         PD_LOG( PDERROR, "kill or $shutdown node[%d] failed, rc: %d",
                 node._pid, rc ) ;
         goto error ;
      }
      else if ( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      while ( timeout > 0 )
      {
         --timeout ;
         if ( !ossIsProcessRunning( node._pid ) )
         {
            rc = SDB_OK ;
            goto done ;
         }
         ossSleep( OSS_ONE_SEC ) ;
      }
      rc = SDB_TIMEOUT ;

   done:
      if ( rc && force )
      {
         rc = ossTerminateProcess( node._pid, TRUE ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilEndNodePipeDup( const CHAR *svcname, OSSPID pid )
   {
      INT32 rc = SDB_OK ;
      INT32 result = SDB_OK ;

      rc = _utilWriteReadPipe( svcname, pid, ENGINE_NPIPE_MSG_ENDPIPE,
                               sizeof( ENGINE_NPIPE_MSG_ENDPIPE ) ,
                               ( CHAR* )&result, sizeof( result ),
                               FALSE ) ;
      if ( SDB_OK == rc )
      {
         rc = result ;
      }

#ifdef _DEBUG
      PD_LOG( PDEVENT, "End node[%s: %d] pipe result: %d", svcname, pid, rc ) ;
#endif // _DEBUG

      return rc ;
   }

   INT32 utilGetNodeVerInfo( const CHAR *pCommand,
                             utilNodeVerInfo & verInfo )
   {
      INT32 rc = SDB_OK ;
      FILE *fp = NULL ;
      CHAR buff[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( !pCommand )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      fp = ossPopen( pCommand, "r" ) ;
      if ( !fp )
      {
         PD_LOG( PDERROR, "File to popen[%s], rc: %d", pCommand,
                 ossGetLastError() ) ;
         rc = SDB_SYS ;
      }

      fread( buff, OSS_MAX_PATHSIZE, 1, fp ) ;

      rc = utilParseVersion( buff, verInfo._version, verInfo._subVersion,
                             verInfo._fixVersion, verInfo._release,
                             verInfo._buildStr ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse version info[%s], rc: %d",
                 buff, rc ) ;
         goto error ;
      }

   done:
      if ( fp )
      {
         ossPclose( fp ) ;
      }
      return rc ;
   error:
      goto done ;
   }

}


