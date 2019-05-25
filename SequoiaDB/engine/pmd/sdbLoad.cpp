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

   Source File Name = sdbLoad.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  JW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "../client/bson/bson.h"
#include "../client/common.h"
#include "../client/network.h"
#include "pd.hpp"
#include "ossSocket.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "msgDef.h"
#include "openssl/md5.h"
#include "ossIO.hpp"

#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std ;
namespace po = boost::program_options ;

#define ADD_PARAM_OPTIONS_BEGIN( desc )\
           desc.add_options()
#define ADD_PARAM_OPTIONS_END ;

#define LOGPATH "sdbLoad.log"

#define OPTION_HELP           "help"
#define OPTION_SVCNAME        "svcname"
#define OPTION_FILENAME       "file"
#define OPTION_USER           "user"
#define OPTION_PASSWORD       "password"
#define OPTION_DELCHAR        "delchar"
#define OPTION_DELFIELD       "delfield"
#define OPTION_DELRECORD      "delrecord"
#define OPTION_FIELD          FIELD_NAME_FIELDS
#define OPTION_HEADERLINE     FIELD_NAME_HEADERLINE

#define OPTION_COLLECTSPACE      FIELD_NAME_COLLECTIONSPACE
#define OPTION_COLLECTION        FIELD_NAME_COLLECTION
#define OPTION_ASYNCHRONOUS      FIELD_NAME_ASYNCHRONOUS
#define OPTION_THREADNUM         FIELD_NAME_THREADNUM
#define OPTION_BUCKETNUM         FIELD_NAME_BUCKETNUM
#define OPTION_PARSEBUFFERSIZE   FIELD_NAME_PARSEBUFFERSIZE
#define OPTION_TYPE              FIELD_NAME_LTYPE

#define DEFAULT_HOSTNAME   "localhost"

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()
#define COMMANDS_OPTIONS \
   ( OPTION_HELP, "help" ) \
   ( COMMANDS_STRING ( OPTION_SVCNAME,             ",s"),      boost::program_options::value<string>(), "database service name, default: 11810" ) \
   ( COMMANDS_STRING ( OPTION_USER,                ",u"),      boost::program_options::value<string>(), "database user" ) \
   ( COMMANDS_STRING ( OPTION_PASSWORD,            ",w"),      boost::program_options::value<string>(), "database password" ) \
   ( COMMANDS_STRING ( OPTION_COLLECTSPACE,        ",c"),      boost::program_options::value<string>(), "database collection space name" ) \
   ( COMMANDS_STRING ( OPTION_COLLECTION,          ",l"),      boost::program_options::value<string>(), "database collection name" ) \
   ( COMMANDS_STRING ( OPTION_THREADNUM,           ",t"),      boost::program_options::value<string>(), "thread number, default: 4" ) \
   ( COMMANDS_STRING ( OPTION_PARSEBUFFERSIZE,     ",b"),      boost::program_options::value<string>(), "buffer size, default: 33554432" ) \
   ( COMMANDS_STRING ( OPTION_BUCKETNUM,           ",n"),      boost::program_options::value<string>(), "bucket number, default: 32768" ) \
   ( COMMANDS_STRING ( OPTION_DELCHAR,             ",a"),      boost::program_options::value<string>(), "string delimiter, default: \"" ) \
   ( COMMANDS_STRING ( OPTION_DELFIELD,            ",e"),      boost::program_options::value<string>(), "field delimiter, default: ," ) \
   ( COMMANDS_STRING ( OPTION_DELRECORD,           ",r"),      boost::program_options::value<string>(), "record delimiter, default: '\\n'" ) \
   ( OPTION_FILENAME,      boost::program_options::value<string>(), "database load file name" ) \
   ( OPTION_TYPE,          boost::program_options::value<string>(), "type of file to load, default: json (json,csv)" ) \
   ( OPTION_FIELD,         boost::program_options::value<string>(), "comma separated list of field names e.g. --fields name,age" ) \
   ( OPTION_HEADERLINE,    boost::program_options::value<string>(), "first line in input file is a header, default: false ( CSV only )" ) \
   ( OPTION_ASYNCHRONOUS,  boost::program_options::value<string>(), "asynchronous load, default: false" ) \


CHAR lServiceName [ OSS_MAX_SERVICENAME + 1 ]  = { 0 } ;
CHAR *lFileName  = NULL ;
CHAR *lcsName    = NULL ;
CHAR *lclName    = NULL ;
CHAR *lUser      = NULL ;
CHAR *lPassWord  = NULL ;
CHAR *lField     = NULL ;
INT32 lType = 0 ;
BOOLEAN isHeaderline   = FALSE ;
BOOLEAN isAsynchronous = FALSE ;
INT32 lThreadNum = 0 ;
INT32 lBucketNum = 0 ;
INT32 lBufferSize = 0 ;
CHAR  lDelCFR[4] ;
Socket* s ;

void init ( po::options_description &desc )
{
   ADD_PARAM_OPTIONS_BEGIN ( desc )
      COMMANDS_OPTIONS
   ADD_PARAM_OPTIONS_END
}

void displayArg ( po::options_description &desc )
{
   std::cout << desc << std::endl ;
}

void printMsg ( CHAR *buffer )
{
   struct tm otm ;
   struct timeval tv;
   struct timezone tz;
   time_t tt ;
   gettimeofday(&tv, &tz);
   tt = tv.tv_sec ;
#if defined (_WINDOWS)
   localtime_s( &otm, &tt ) ;
#else
   localtime_r( &tt, &otm ) ;
#endif
   printf ( "%u-%02u-%02u-%02u.%02u.%02u.%06u  %s" OSS_NEWLINE,
            otm.tm_year+1900,            // 1) Year (UINT32)
            otm.tm_mon+1,                // 2) Month (UINT32)
            otm.tm_mday,                 // 3) Day (UINT32)
            otm.tm_hour,                 // 4) Hour (UINT32)
            otm.tm_min,                  // 5) Minute (UINT32)
            otm.tm_sec,                  // 6) Second (UINT32)
            (UINT32)tv.tv_usec,          // 7) Microsecond (UINT32)
            buffer ) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_SDBLOAD_RESOLVEARG, "resolveArgument" )
INT32 resolveArgument ( po::options_description &desc, INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_SDBLOAD_RESOLVEARG );
   const CHAR *pFileName = NULL ;
   const CHAR *pCsName   = NULL ;
   const CHAR *pClName   = NULL ;
   const CHAR *pUser     = NULL ;
   const CHAR *pPassWord = NULL ;
   const CHAR *pField    = NULL ;
   const CHAR *pDel      = NULL ;
   po::variables_map vm ;
   try
   {
      po::store ( po::parse_command_line ( argc, argv, desc ), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      pdLog ( PDWARNING, __FUNC__, __FILE__, __LINE__,
            ( ( std::string ) "Unknown argument: " +
                e.get_option_name ()).c_str () ) ;
              std::cerr <<  "Unknown argument: "
                        << e.get_option_name () << std::endl ;
              rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      pdLog ( PDWARNING, __FUNC__, __FILE__, __LINE__,
             ( ( std::string ) "Invalid argument: " +
               e.get_option_name () ).c_str () ) ;
      std::cerr <<  "Invalid argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch( po::error &e )
   {
      std::cerr << e.what () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( vm.count ( OPTION_HELP ) )
   {
      displayArg ( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   lDelCFR[0] = '"' ;
   lDelCFR[1] = ',' ;
   lDelCFR[2] = '\n' ;
   lDelCFR[3] = 0 ;
   ossMemset ( lServiceName, 0, OSS_MAX_SERVICENAME + 1 ) ;
   if ( vm.count ( OPTION_SVCNAME ) )
   {
      ossStrncpy ( lServiceName, vm[OPTION_SVCNAME].as<string>().c_str(),
                   OSS_MAX_SERVICENAME ) ;
   }
   else
   {
      ossSnprintf ( lServiceName, OSS_MAX_SERVICENAME, "%d",
                    OSS_DFT_SVCPORT ) ;
   }

   if ( vm.count ( OPTION_FILENAME ) )
   {
      pFileName = vm[OPTION_FILENAME].as<string>().c_str() ;
      lFileName = (CHAR *)SDB_OSS_MALLOC ( ossStrlen ( pFileName ) + 1 ) ;
      lFileName[ossStrlen ( pFileName )] = 0 ;
      ossStrncpy ( lFileName, pFileName, ossStrlen ( pFileName ) ) ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "file name must enter" ) ;
      goto error ;
   }

   if ( vm.count ( OPTION_COLLECTSPACE ) )
   {
      pCsName = vm[OPTION_COLLECTSPACE].as<string>().c_str() ;
      lcsName = (CHAR *)SDB_OSS_MALLOC ( ossStrlen ( pCsName ) + 1 ) ;
      lcsName[ossStrlen ( pCsName )] = 0 ;
      ossStrncpy ( lcsName, pCsName, ossStrlen ( pCsName ) ) ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "file name must enter" ) ;
      goto error ;
   }

   if ( vm.count ( OPTION_COLLECTION ) )
   {
      pClName = vm[OPTION_COLLECTION].as<string>().c_str() ;
      lclName = (CHAR *)SDB_OSS_MALLOC ( ossStrlen ( pClName ) + 1 ) ;
      lclName[ossStrlen ( pClName )] = 0 ;
      ossStrncpy ( lclName, pClName, ossStrlen ( pClName ) ) ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "file name must enter" ) ;
      goto error ;
   }

   if ( vm.count ( OPTION_USER ) )
   {
      pUser = vm[OPTION_FILENAME].as<string>().c_str() ;
      lUser = (CHAR *)SDB_OSS_MALLOC ( ossStrlen ( pUser ) + 1 ) ;
      lUser[ossStrlen ( pUser )] = 0 ;
      ossStrncpy ( lUser, pUser, ossStrlen ( pUser ) ) ;
   }
   else
   {
      lUser = (CHAR *)SDB_OSS_MALLOC ( 1 ) ;
      *lUser = '\0' ;
   }

   if ( vm.count ( OPTION_PASSWORD ) )
   {
      pPassWord = vm[OPTION_FILENAME].as<string>().c_str() ;
      lPassWord= (CHAR *)SDB_OSS_MALLOC ( ossStrlen ( pPassWord ) + 1 ) ;
      lPassWord[ossStrlen ( pPassWord )] = 0 ;
      ossStrncpy ( lPassWord, pPassWord, ossStrlen ( pPassWord ) ) ;
   }
   else
   {
      lPassWord = (CHAR *)SDB_OSS_MALLOC ( 1 ) ;
      *lUser = '\0' ;
   }

   if ( vm.count ( OPTION_THREADNUM ) )
   {
      lThreadNum = ossAtoi ( vm[OPTION_THREADNUM].as<string>().c_str() ) ;
   }

   if ( vm.count ( OPTION_BUCKETNUM ) && vm.count ( OPTION_PARSEBUFFERSIZE ) )
   {
      lBucketNum = ossAtoi ( vm[OPTION_BUCKETNUM].as<string>().c_str() ) ;
      lBufferSize = ossAtoi( vm[OPTION_PARSEBUFFERSIZE].as<string>().c_str() ) ;
   }
   else if(!vm.count ( OPTION_BUCKETNUM ) && !vm.count(OPTION_PARSEBUFFERSIZE ))
   {
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "bucket number and buffer size must enter" ) ;
      goto error ;
   }

   if ( vm.count ( OPTION_ASYNCHRONOUS ) )
   {
      ossStrToBoolean ( vm[OPTION_ASYNCHRONOUS].as<string>().c_str(),
                        &isAsynchronous ) ;
   }

   if ( vm.count ( OPTION_HEADERLINE ) )
   {
      ossStrToBoolean ( vm[OPTION_HEADERLINE].as<string>().c_str(),
                        &isHeaderline ) ;
   }

   do
   {
      if ( vm.count ( OPTION_TYPE ) )
      {
         if ( ossStrlen ( vm[OPTION_TYPE].as<string>().c_str() ) == 3 )
         {
            if (!ossStrncmp( vm[OPTION_TYPE].as<string>().c_str(), "csv", 3 ))
            {
               lType = 1 ;
               break ;
            }
         }
         else if ( ossStrlen ( vm[OPTION_TYPE].as<string>().c_str() ) == 4 )
         {
            if (!ossStrncmp( vm[OPTION_TYPE].as<string>().c_str(), "json", 4))
            {
               lType = 0 ;
               break ;
            }
         }
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "bucket number and buffer size must enter" ) ;
         goto error ;
      }
   }while ( FALSE ) ;

   if ( vm.count ( OPTION_FIELD ) )
   {
      INT32 size = 0 ;
      pField = vm[OPTION_FIELD].as<string>().c_str() ;
      size = ossStrlen ( pField ) ;
      lField = (CHAR *)SDB_OSS_MALLOC ( size + 1 ) ;
      lField[size] = 0 ;
      ossStrncpy ( lField, pField, size ) ;
   }

   if ( vm.count ( OPTION_DELCHAR ) )
   {
      INT32 size = 0 ;
      pDel = vm[OPTION_DELCHAR].as<string>().c_str() ;
      size = ossStrlen ( pDel ) ;
      if ( size > 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "delchar is char" ) ;
         goto error ;
      }
      lDelCFR[0] = pDel[0] ;
      lDelCFR[3] = 1 ;
   }

   if ( vm.count ( OPTION_DELFIELD ) )
   {
      INT32 size = 0 ;
      pDel = vm[OPTION_DELFIELD].as<string>().c_str() ;
      size = ossStrlen ( pDel ) ;
      if ( size > 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "delfield is char" ) ;
         goto error ;
      }
      lDelCFR[1] = pDel[0] ;
      lDelCFR[3] = 1 ;
   }

   if ( vm.count ( OPTION_DELRECORD ) )
   {
      INT32 size = 0 ;
      pDel = vm[OPTION_DELRECORD].as<string>().c_str() ;
      size = ossStrlen ( pDel ) ;
      if ( size > 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "delrecord is char" ) ;
         goto error ;
      }
      lDelCFR[2] = pDel[0] ;
      lDelCFR[3] = 1 ;
   }
done :
   PD_TRACE_EXITRC ( SDB_SDBLOAD_RESOLVEARG, rc );
   return rc ;
error :
   goto done ;
}

INT32 _connectSdb ()
{
   INT32 rc = SDB_OK ;
   UINT16 port = 0 ;
   CHAR lHostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;

   rc = _ossSocket::getHostName ( lHostName, OSS_MAX_HOSTNAME + 1 ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "get host name error" ) ;
      goto error ;
   }
   rc = _ossSocket::getPort ( lServiceName, port ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "get port error" ) ;
      goto error ;
   }
   rc = clientConnect ( lHostName, lServiceName, FALSE, &s ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "connect database error" ) ;
      goto error ;
   }
   disableNagle( s ) ;
done:
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_SDBLOAD_LOADRECV, "loadRecv" )
INT32 loadRecv ( CHAR **buffer, SINT32 *size )
{
   INT32 rc = SDB_OK ;
   INT32 receivedLen = 0 ;
   INT32 totalReceivedLen = 0 ;
   PD_TRACE_ENTRY ( SDB_SDBLOAD_LOADRECV ) ;
   CHAR   *_pRecvBuffer   = NULL ;
   SINT32 _recvBufferSize = 0 ;

   while( TRUE )
   {
      rc = clientRecv ( s, ((CHAR *)&_recvBufferSize) + totalReceivedLen,
                        sizeof (_recvBufferSize) - totalReceivedLen,
                        &receivedLen,
                        -1 ) ;
      totalReceivedLen += receivedLen ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to recv msg, rc = %d", rc ) ;
         goto error ;
      }
      break ;
   }
   _pRecvBuffer = (CHAR *)malloc ( _recvBufferSize ) ;
   if ( !_pRecvBuffer )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "memory error" ) ;
      goto error ;
   }
   memset ( _pRecvBuffer, 0, _recvBufferSize ) ;
   *(SINT32 *)_pRecvBuffer = _recvBufferSize ;
   totalReceivedLen = 0 ;
   receivedLen = 0 ;
   while( TRUE )
   {
      rc = clientRecv ( s, _pRecvBuffer + sizeof(_recvBufferSize) + totalReceivedLen,
                        _recvBufferSize - sizeof(_recvBufferSize) - totalReceivedLen,
                        &receivedLen,
                        -1 ) ;
      totalReceivedLen += receivedLen ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to recv msg, rc = %d", rc ) ;
         goto error ;
      }
      break ;
   }
   *buffer = _pRecvBuffer ;
   *size = _recvBufferSize ;
done:
   PD_TRACE_EXITRC ( SDB_SDBLOAD_LOADRECV, rc );
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_SDBLOAD_CONNECTSDB, "connectSdb" )
INT32 connectSdb ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_SDBLOAD_CONNECTSDB ) ;
   CHAR *_pSendBuffer = NULL ;
   INT32 _sendBufferSize = 0 ;
   CHAR   *_pRecvBuffer   = NULL ;
   SINT32 _recvBufferSize = 0 ;
   INT32 msgLen = 0 ;
   INT32 sentSize = 0 ;
   INT32 totalSentSize = 0 ;
   CHAR md5 [ MD5_DIGEST_LENGTH * 2 + 1 ] ;

   rc = _connectSdb() ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to connect database, rc = %d", rc ) ;
      goto error ;
   }

   rc = md5Encrypt( lPassWord, md5, MD5_DIGEST_LENGTH * 2 + 1 ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to md5 encrypt, rc = %d", rc ) ;
      goto error ;
   }

   rc = clientBuildAuthMsg( &_pSendBuffer, &_sendBufferSize,
                            lUser, md5, 0, FALSE ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to build auth msg, rc = %d", rc ) ;
      goto error ;
   }
   msgLen = *((UINT32 *)_pSendBuffer) ;
   while( msgLen > totalSentSize )
   {
      rc = clientSend ( s, _pSendBuffer + totalSentSize,
                        msgLen - totalSentSize,
                        &sentSize, -1 ) ;
      totalSentSize += sentSize ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to send msg, rc = %d", rc ) ;
         goto error ;
      }
   }
   rc = loadRecv ( &_pRecvBuffer, &_recvBufferSize ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to recv msg, rc = %d", rc ) ;
      goto error ;
   }
   free ( _pRecvBuffer ) ;
done:
   PD_TRACE_EXITRC ( SDB_SDBLOAD_CONNECTSDB, rc );
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_SDBLOAD_MAIN, "main" )
INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_SDBLOAD_MAIN );

   CHAR pathBuffer [ OSS_MAX_PATHSIZE + 1 ] = {'\0'} ;
   CHAR  *_pSendBuffer = NULL ;
   INT32  _sendBufferSize = 0 ;
   CHAR  *_pRecvBuffer = NULL ;
   INT32  _recvBufferSize = 0 ;
   INT32 msgLen = 0 ;
   INT32 sentSize = 0 ;
   INT32 totalSentSize = 0 ;
   bson sendObj ;

   bson_init ( &sendObj ) ;

   sdbEnablePD( LOGPATH ) ;
   setPDLevel( PDINFO ) ;

   po::options_description desc ( "Command options" ) ;
   init ( desc ) ;
   rc = resolveArgument ( desc, argc, argv ) ;
   if ( rc )
   {
      if ( SDB_PMD_HELP_ONLY != rc )
      {
         PD_LOG ( PDERROR, "Invalid argument" ) ;
         displayArg ( desc ) ;
      }
      goto error ;
   }
   rc = connectSdb() ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Faild to connectSDB, rc = %d", rc ) ;
      goto error ;
   }

   ossMemset ( pathBuffer, 0, sizeof(pathBuffer) ) ;
   if ( !ossGetRealPath ( lFileName, pathBuffer, sizeof(pathBuffer) ) )
   {
      PD_LOG ( PDERROR, "Faild to get real path, %s", lFileName ) ;
      goto error ;
   }

   rc = bson_append_string ( &sendObj, FIELD_NAME_FILENAME,
                             pathBuffer ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = bson_append_string ( &sendObj, OPTION_COLLECTSPACE,
                             lcsName ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = bson_append_string ( &sendObj, OPTION_COLLECTION,
                             lclName ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( lThreadNum > 0 )
   {
      rc = bson_append_int ( &sendObj, OPTION_THREADNUM,
                             lThreadNum ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   if ( lBucketNum > 0 )
   {
      rc = bson_append_int ( &sendObj, OPTION_BUCKETNUM,
                             lBucketNum ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   if ( lBufferSize > 0 )
   {
      rc = bson_append_int ( &sendObj, OPTION_PARSEBUFFERSIZE,
                             lBufferSize ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   rc = bson_append_bool ( &sendObj, OPTION_ASYNCHRONOUS,
                           isAsynchronous ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = bson_append_bool ( &sendObj, OPTION_HEADERLINE,
                           isHeaderline ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( !isHeaderline && 1 == lType )
   {
      if ( !lField )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "if headerline is false, fields can not null") ;
         goto error ;
      }
      rc = bson_append_string ( &sendObj, OPTION_FIELD,
                                lField ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   rc = bson_append_int ( &sendObj, OPTION_TYPE,
                          lType ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( 1 == lDelCFR[3] )
   {
      lDelCFR[3] = 0 ;
      rc = bson_append_string ( &sendObj, FIELD_NAME_CHARACTER,
                             lDelCFR ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   bson_finish ( &sendObj ) ;

   rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                              CMD_ADMIN_PREFIX CMD_NAME_JSON_LOAD,
                              0, 0, 0, -1,
                              sendObj.data, NULL, NULL, NULL,
                              FALSE ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to build msg, rc = %d", rc ) ;
      goto error ;
   }
   msgLen = *((UINT32 *)_pSendBuffer) ;
   sentSize = 0 ;
   totalSentSize = 0 ;
   while ( msgLen > totalSentSize )
   {
      rc = clientSend ( s, _pSendBuffer + totalSentSize,
                        msgLen - totalSentSize, &sentSize, -1 ) ;
      totalSentSize += sentSize ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send msg, rc = %d", rc ) ;
         goto error ;
      }
   }
   while ( TRUE )
   {
      MsgOpMsg *msgOp = NULL ;
      CHAR *msgStr = NULL ;
      rc = loadRecv ( &_pRecvBuffer, &_recvBufferSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to recv msg, rc = %d", rc ) ;
         goto error ;
      }
      msgOp = (MsgOpMsg *)_pRecvBuffer ;
      if ( MSG_BS_MSG_REQ == msgOp->header.opCode )
      {
         msgStr = msgOp->msg ;
         printMsg ( msgStr ) ;
      }
      else
      {
         ossMemset ( _pSendBuffer, 0, _sendBufferSize ) ;
         rc = clientBuildDisconnectMsg ( &_pSendBuffer, &_sendBufferSize,
                                         0, FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to build disconnect msg, rc = %d", rc ) ;
            goto error ;
         }
         msgLen = *((UINT32 *)_pSendBuffer) ;
         sentSize = 0 ;
         totalSentSize = 0 ;
         while ( msgLen > totalSentSize )
         {         
            rc = clientSend ( s, _pSendBuffer + totalSentSize, 
                              msgLen - totalSentSize, &sentSize, -1 ) ;
            totalSentSize += sentSize ;
            if ( SDB_TIMEOUT == rc )
            {
               continue ;
            }
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to send msg, rc = %d", rc ) ;
               goto error ;
            }
         }
         goto done ;
      }
   }
done:
   SAFE_OSS_FREE( _pSendBuffer ) ;
   clientDisconnect ( &s ) ;
   PD_TRACE_EXITRC ( SDB_SDBLOAD_MAIN, rc );
   return SDB_OK == rc ? 0 : 1 ;
error:
   goto done ;
}
