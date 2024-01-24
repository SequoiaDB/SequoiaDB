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

   Source File Name = omagentUtil.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossPrimitiveFileOp.hpp"
#include "pd.hpp"
#include "omagentUtil.hpp"
#include "ossProc.hpp"
#include "pmdOptions.hpp"
#include "ossPath.hpp"
#include "ossIO.hpp"
#include "pmdDef.hpp"
#include "utilNodeOpr.hpp"
#include "ossSocket.hpp"
#include "utilCommon.hpp"
#include "ossCmdRunner.hpp"

namespace engine
{
   /*
      Local Define
   */
   #define CM_NPIPE_SIZE                  64


   INT32 checkBuffer ( CHAR **ppBuffer, INT32 *bufferSize,
                       INT32 packetLength )
   {
      INT32 rc = SDB_OK ;
      if ( packetLength > *bufferSize )
      {
         CHAR *pOrigMem = *ppBuffer ;
         INT32 newSize = ossRoundUpToMultipleX ( packetLength, SDB_PAGE_SIZE ) ;
         if ( newSize < 0 )
         {
            pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                    "new buffer overflow" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         *ppBuffer = (CHAR*)SDB_OSS_REALLOC ( *ppBuffer, sizeof(CHAR)*(newSize)) ;
         if ( !*ppBuffer )
         {
            PD_LOG ( PDERROR, "Failed to allocate %d bytes send buffer",
                     newSize ) ;
            rc = SDB_OOM ;
            // realloc does NOT free original memory if it fails, so we have to
            // assign pointer to original
            *ppBuffer = pOrigMem ;
            goto error ;
         }
         *bufferSize = newSize ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }


   INT32 readFile ( const CHAR *name , CHAR **buf , UINT32 *bufSize,
                    UINT32 *readSize )
   {
      ossPrimitiveFileOp op ;
      ossPrimitiveFileOp::offsetType offset ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( name && buf && bufSize, "Invalid arguments" ) ;

      rc = op.Open ( name , OSS_PRIMITIVE_FILE_OP_READ_ONLY ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Can't open file: %s", name ) ;
         goto error ;
      }

      rc = op.getSize ( &offset ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to get file's size" ) ;
         goto error ;
      }

      if ( *bufSize < offset.offset + 1 )
      {
         if ( *buf )
         {
            SDB_OSS_FREE( *buf ) ;
            *buf = NULL ;
            *bufSize = 0 ;
         }
         *buf = (CHAR *) SDB_OSS_MALLOC ( offset.offset + 1 ) ;
         if ( ! *buf )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR , "Failed to alloc memory" ) ;
            goto error ;
         }
         *bufSize = offset.offset + 1 ;
      }

      rc = op.Read ( offset.offset , *buf , NULL ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to read file" ) ;
         goto error ;
      }
      (*buf)[ offset.offset ] = 0 ;
      if ( readSize ) *readSize = offset.offset ;

   done :
      op.Close() ;
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN portCanUsed ( UINT32 port, INT32 timeoutMilli )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = FALSE ;
      _ossSocket sock( port, timeoutMilli ) ;

      if ( port <= 1024 || port > 65535 )
      {
         goto error ;
      }
      rc = sock.initSocket() ;
      if ( rc )
      {
         PD_LOG ( PDEVENT, "Failed to connect to port[%s], "
                  "rc = %d", port, rc ) ;
         goto error ;
      }
      rc = sock.bind_listen() ;
      if ( rc )
      {
         PD_LOG ( PDEVENT, "Failed to bind/listen socket, rc = %d", rc ) ;
         goto error ;
      }
      result = TRUE ;
      // close the socket
      sock.close() ;

   done:
      return result ;
   error:
      goto done ;

   }

   // get bson field
   INT32 omaGetIntElement ( const BSONObj &obj, const CHAR *fieldName,
                            INT32 &value )
   {
      SINT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Integer",
                 obj.toString().c_str()) ;
      value = ele.numberInt() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omaGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               const CHAR **value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName && value, "field name and value can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be String",
                 obj.toString().c_str()) ;
      *value = ele.valuestr() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omaGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               string& value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be String",
                 obj.toString().c_str()) ;
      value = ele.String() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omaGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                            BSONObj &value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( Object == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Object",
                 obj.toString().c_str()) ;
      value = ele.embeddedObject().getOwned() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omaGetSubObjArrayElement ( const BSONObj &obj,
                                    const CHAR *objFieldName,
                                    const CHAR *subObjFieldName,
                                    const CHAR *subObjNewFieldName,
                                    BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj value ;

      rc = omaGetObjElement( obj, objFieldName, value ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Get field[%s] failed, rc = %d",
                  objFieldName, rc ) ;
         goto error ;
      }
      ele = value.getField ( subObjFieldName ) ;
      if ( Array != ele.type () )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Wrong bson format, rc = %d", rc ) ;
         goto error ;
      }
      builder.appendAs( ele, subObjNewFieldName ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omaGetBooleanElement ( const BSONObj &obj, const CHAR *fieldName,
                                BOOLEAN &value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( Bool == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Bool",
                 obj.toString().c_str()) ;
      value = ele.boolean() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   /*
      Node Manager Tool Functions Implement
   */
   INT32 omStartDBNode( const CHAR *pExecName,
                        const CHAR *pCfgPath,
                        const CHAR *pSvcName,
                        OSSPID &pid,
                        BOOLEAN useCurUser )
   {
      INT32 rc                = SDB_OK ;
      ossCmdRunner runner ;
      string cmdline ;
      UINT32 exit = 0 ;

      // verify the configuration file
      rc = ossAccess ( pCfgPath ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Can not access the configure file: %s", pCfgPath ) ;
         goto error ;
      }

      cmdline += pExecName ;
      cmdline += " " ;
      cmdline += SDBCM_OPTION_PREFIX PMD_OPTION_CONFPATH ;
      cmdline += " " ;
      cmdline += pCfgPath ;
#if defined( _LINUX )
      cmdline += " " ;
      cmdline += SDBCM_OPTION_PREFIX PMD_OPTION_IGNOREULIMIT ;
#endif

      if ( useCurUser )
      {
         cmdline += " " ;
         cmdline += SDBCM_OPTION_PREFIX PMD_OPTION_CURUSER ;
      }

      rc = runner.exec( cmdline.c_str(), exit, FALSE, OSS_ONE_SEC * 900 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute command[%s], rc: %d",
                  cmdline.c_str(), rc ) ;
         goto error ;
      }
      // verify the executing result
      if ( exit == SDB_OK  )
      {
         UTIL_VEC_NODES nodes ;

         rc = utilListNodes( nodes, -1, pSvcName ) ;
         if ( SDB_OK == rc && nodes.size() > 0 &&
              SDB_TYPE_OMA != (*nodes.begin())._type )
         {
            pid = (*nodes.begin())._pid ;
            goto done ;
         }
         else
         {
            PD_LOG( PDERROR, "List node[%s] failed", pSvcName ) ;
            rc = SDBCM_FAIL ;
         }
      }
      else
      {
         rc = utilShellRC2RC( exit ) ;
      }

      if ( rc )
      {
         string outString ;
         runner.read( outString ) ;
         string nodeOut = omPickNodeOutString( outString, pSvcName ) ;

         if ( nodeOut.length() < PD_LOG_STRINGMAX - 100 )
         {
            PD_LOG( PDERROR, "node[%s] start[cmd: %s] failed, "
                    "out info:===>%s%s%s<===", pSvcName,
                    cmdline.c_str(), OSS_NEWLINE, nodeOut.c_str(),
                    OSS_NEWLINE ) ;
         }
         else
         {
            PD_LOG( PDERROR, "node[%s] start[cmd: %s] failed, "
                    "out info:===>", pSvcName,
                    cmdline.c_str() ) ;
            PD_LOG_RAW( PDERROR, nodeOut.c_str() ) ;
            PD_LOG_RAW( PDERROR, OSS_NEWLINE "<===" OSS_NEWLINE OSS_NEWLINE ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStopDBNode( const CHAR *pExecName, const CHAR *pServiceName,
                       BOOLEAN force )
   {
      INT32 rc                = SDB_OK ;
      CHAR *pArgumentBuffer   = NULL ;
      INT32 argBuffLen        = 0 ;

      list<const CHAR*> argv ;
      ossResultCode result ;
      OSSPID pid ;

      argv.push_back( pExecName ) ;
      if ( pServiceName && pServiceName[0] )
      {
         argv.push_back( SDBCM_OPTION_PREFIX PMD_OPTION_SVCNAME ) ;
         argv.push_back( pServiceName ) ;
      }
      if ( force )
      {
         argv.push_back( SDBCM_OPTION_PREFIX PMD_OPTION_FORCE ) ;
      }

      rc = ossBuildArguments( &pArgumentBuffer, argBuffLen, argv ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to build sdbstop arguments, rc: %d",
                  rc ) ;
         goto error ;
      }
      // call exec to run the command with arguments,
      // do NOT wait until program finish
      rc = ossExec ( pArgumentBuffer, pArgumentBuffer, NULL,
                     OSS_EXEC_SSAVE, pid, result, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute %s, rc: %d",
                  pArgumentBuffer, rc ) ;
         goto error ;
      }

      // verify the executing result
      if ( result.termcode != OSS_EXIT_NORMAL )
      {
         rc = SDBCM_FAIL ;
      }
      else
      {
         switch ( result.exitcode )
         {
            case SDB_OK:
               rc = SDB_OK ;
               break ;
            case STOPPART:
               rc = SDBCM_STOP_PART ;
               break ;
            default:
               rc = SDBCM_FAIL ;
         }
      }

   done:
      if ( pArgumentBuffer )
      {
         SDB_OSS_FREE( pArgumentBuffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omGetSvcListFromConfig( const CHAR * pCfgRootDir,
                                 vector < string > &svcList )
   {
      INT32 rc = SDB_OK ;

      rc = ossEnumSubDirs( pCfgRootDir, svcList, 1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to enum service in path[%s], rc: %d",
                 pCfgRootDir, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCheckDBProcessBySvc( const CHAR *svcname,
                                BOOLEAN &isRuning,
                                OSSPID &pid )
   {
      INT32 rc = SDB_OK ;
      isRuning = FALSE ;
      UTIL_VEC_NODES nodes ;

      rc = utilListNodes( nodes, -1, svcname ) ;
      if ( SDB_OK == rc && nodes.size() > 0 &&
           SDB_TYPE_OMA != (*nodes.begin())._type )
      {
         isRuning = TRUE ;
         pid = (*nodes.begin())._pid ;
      }

      return rc ;
   }

   string omPickNodeOutString( const string &out, const CHAR *pSvcname )
   {
      // %s: %u bytes out==>%s<==
      string nodeStr = out ;
      CHAR finder[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      const CHAR *pStr = out.c_str() ;
      const CHAR *pStr1 = NULL ;
      const CHAR *pStr2 = NULL ;

      ossSnprintf( finder, OSS_MAX_PATHSIZE, "%s: ", pSvcname ) ;
      pStr1 = ossStrstr( pStr, finder ) ;
      if ( pStr1 )
      {
         pStr1 += ossStrlen( finder ) ;

         ossStrcpy( finder, " bytes out==>" ) ;
         pStr2 = ossStrstr( pStr1, finder ) ;
         if ( pStr2 )
         {
            UINT32 len = ossStrlen( finder ) ;
            ossMemset( finder, 0, sizeof( finder ) ) ;
            ossStrncpy( finder, pStr1, pStr2 - pStr1 ) ;
            pStr2 += len ;

            const CHAR *pStr3 = ossStrstr( pStr2 + ossAtoi( finder ),
                                           "<==" ) ;
            if ( pStr3 )
            {
               nodeStr = out.substr( pStr2 - pStr, pStr3 - pStr2 ) ;
            }
            else
            {
               nodeStr = out.substr( pStr2 - pStr, ossAtoi( finder ) ) ;
            }
         }
      }

      return utilStrTrim( nodeStr ) ; ;
   }

}

