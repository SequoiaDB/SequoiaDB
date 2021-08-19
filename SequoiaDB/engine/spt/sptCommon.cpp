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

   Source File Name = sptCommon.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptCommon.hpp"
#include "sptPrivateData.hpp"
#include "pd.hpp"
#include "ossUtil.h"
#include "ossMem.h"
#include <string>
#include <sstream>
#include "msg.h"
#include "sptSPDef.hpp"
#include "jsinterp.h"

using namespace std ;

namespace engine
{

   /*
      Global function
   */
   static OSS_THREAD_LOCAL CHAR     *__errmsg__            = NULL ;
   static OSS_THREAD_LOCAL UINT32   __errmsgSize__         = 0 ;
   static OSS_THREAD_LOCAL INT32    __errno__              = SDB_OK ;

   static OSS_THREAD_LOCAL CHAR     *__errobj__            = NULL ;
   static OSS_THREAD_LOCAL UINT32   __errobjSize__         = 0 ;

   static OSS_THREAD_LOCAL BOOLEAN  __printError__         = TRUE ;
   static OSS_THREAD_LOCAL BOOLEAN  __hasReadData__        = FALSE ;

   static OSS_THREAD_LOCAL BOOLEAN  __hasSetErrMsg__       = FALSE ;
   static OSS_THREAD_LOCAL BOOLEAN  __hasSetErrNo__        = FALSE ;
   static OSS_THREAD_LOCAL BOOLEAN  __hasSetErrObj__       = FALSE ;
   static OSS_THREAD_LOCAL BOOLEAN  __needClearErrorInfo__ = FALSE ;
   static OSS_THREAD_LOCAL BOOLEAN  __ignoreErrorPrefix__  = FALSE ;
   static                  BOOLEAN  __isNeedSaveHistory__  = TRUE ;
   /*
      Local Functions Define
   */
   static const CHAR* _buildObjByErrno( INT32 flag, const CHAR *pDetail )
   {
      /// BSON Struct
      /// Size(4Byte) + Type(1Byte)(value:16) + FileName('errno'+'\0') + Value(4Bytes)
      ///             + Type(1Byte)(value:2) + FileName('description'+'\0') + Size(4Bytes) + Value(...'\0')
      ///             + Type(1Byte)(value:2) + FileName('detail'+'\0') + Size(4Bytes) + Value(...'\0')
      ///             + Type(1Byte)(value:0)
      const CHAR *pErr = getErrDesp( flag ) ;
      UINT32 errLen = ossStrlen( pErr ) ;
      UINT32 detailLen = 0 ;
      UINT32 objSize = 0 ;
      CHAR *pObjBuff = NULL ;

      if ( !pDetail || 0 == ossStrcmp( pErr, pDetail ) )
      {
         pDetail = "" ;
      }
      detailLen = ossStrlen( pDetail ) ;

      objSize = 4 +  /// size
                1 + ossStrlen( OP_ERRNOFIELD ) + 1 + 4 +   /// OP_ERRNOFIELD
                1 + ossStrlen( OP_ERRDESP_FIELD ) + 1 + 4 + errLen + 1 +  /// OP_ERRDESP_FIELD
                1 + ossStrlen( OP_ERR_DETAIL ) + 1 + 4 + detailLen + 1 +  /// OP_ERR_DETAIL
                1 ;  /// EOO

      if ( __errobjSize__ > 0 && __errobjSize__ < objSize )
      {
         SDB_OSS_FREE( __errobj__ ) ;
         __errobj__ = NULL ;
         __errobjSize__ = 0 ;
      }

      if ( __errobjSize__ < objSize )
      {
         UINT32 tmpSize = objSize + 50 ;
         __errobj__ = ( CHAR* )SDB_OSS_MALLOC( tmpSize ) ;
         if ( __errobj__ )
         {
            __errobjSize__ = tmpSize ;
            pObjBuff = __errobj__ ;
         }
      }
      else
      {
         pObjBuff = __errobj__ ;
      }

      if ( pObjBuff )
      {
         UINT32 offset = 0 ;

         /// size
         *(INT32*)&pObjBuff[offset] = objSize ;
         offset += 4 ;

         /// OP_ERRNOFIELD
         pObjBuff[ offset ] = (CHAR)16 ;
         offset += 1 ;
         ossStrcpy( &pObjBuff[ offset ], OP_ERRNOFIELD ) ;
         offset += ( ossStrlen( OP_ERRNOFIELD ) + 1 ) ;
         *(INT32*)&pObjBuff[offset] = flag ;
         offset += 4 ;

         /// OP_ERRDESP_FIELD
         pObjBuff[ offset ] = (CHAR)2 ;
         offset += 1 ;
         ossStrcpy( &pObjBuff[ offset ], OP_ERRDESP_FIELD ) ;
         offset += ( ossStrlen( OP_ERRDESP_FIELD ) + 1 ) ;
         *(INT32*)&pObjBuff[offset] = errLen + 1 ;
         offset += 4 ;
         ossStrcpy( &pObjBuff[ offset ], pErr ) ;
         offset += ( errLen + 1 ) ;

         /// OP_ERR_DETAIL
         pObjBuff[ offset ] = (CHAR)2 ;
         offset += 1 ;
         ossStrcpy( &pObjBuff[ offset ], OP_ERR_DETAIL ) ;
         offset += ( ossStrlen( OP_ERR_DETAIL ) + 1 ) ;
         *(INT32*)&pObjBuff[offset] = detailLen + 1 ;
         offset += 4 ;
         ossStrcpy( &pObjBuff[ offset ], pDetail ) ;
         offset += ( detailLen + 1 ) ;

         /// EOO
         pObjBuff[ offset ] = 0 ;
      }

      return pObjBuff ;
   }

   const CHAR *sdbGetErrMsg()
   {
      return __errmsg__ ? __errmsg__ : "" ;
   }

   void sdbSetErrMsg( const CHAR *err, BOOLEAN replace )
   {
      if ( !replace && __hasSetErrMsg__ )
      {
         /// not replace
         return ;
      }

      UINT32 len              = 0 ;
      __hasSetErrMsg__        = FALSE ;

      if ( err && *err )
      {
         len = ossStrlen( err ) ;
      }

      if ( len > 0 )
      {
         /// make sure space
         if ( __errmsgSize__ > 0 && __errmsgSize__ < len + 1 )
         {
            SDB_OSS_FREE( __errmsg__ ) ;
            __errmsg__ = NULL ;
            __errmsgSize__ = 0 ;
         }

         if ( __errmsgSize__ < len + 1 )
         {
            UINT32 tmpSize = len + 10 ;
            __errmsg__ = ( CHAR* )SDB_OSS_MALLOC( tmpSize ) ;
            if ( __errmsg__ )
            {
               __errmsgSize__ = tmpSize ;
            }
         }

         /// copy data
         if ( __errmsgSize__ >= len + 1 )
         {
            ossStrcpy( __errmsg__, err ) ;
            __hasSetErrMsg__ = TRUE ;
         }
      }
      else if ( __errmsgSize__ > 0 )
      {
         __errmsg__[ 0 ] = '\0' ;
      }
   }

   void sdbSetErrMsgWithDetail( const CHAR *err, const CHAR *detail,
                                BOOLEAN replace )
   {
      if ( !replace && __hasSetErrMsg__ )
      {
         /// not replace
         return ;
      }

      BOOLEAN errValid = ( err && *err ) ? TRUE : FALSE ;
      BOOLEAN detailValid = ( detail && *detail ) ? TRUE : FALSE ;

      if ( ( errValid ^ detailValid ) || ( !errValid & !detailValid ) )
      {
         sdbSetErrMsg( errValid ? err : detail ) ;
      }
      else if ( errValid && detailValid )
      {
         UINT32 len              = 0 ;
         __hasSetErrMsg__        = FALSE ;

         len = ossStrlen( err ) + ossStrlen( detail ) + 10 ;

         /// make sure space
         if ( __errmsgSize__ > 0 && __errmsgSize__ < len + 1 )
         {
            SDB_OSS_FREE( __errmsg__ ) ;
            __errmsg__ = NULL ;
            __errmsgSize__ = 0 ;
         }

         if ( __errmsgSize__ < len + 1 )
         {
            UINT32 tmpSize = len + 10 ;
            __errmsg__ = ( CHAR* )SDB_OSS_MALLOC( tmpSize ) ;
            if ( __errmsg__ )
            {
               __errmsgSize__ = tmpSize ;
            }
         }

         if ( __errmsgSize__ >= len + 1 )
         {
            ossSnprintf( __errmsg__, len, "%s:\n%s", err, detail ) ;
            __errmsg__[ len ] = '\0' ;
            __hasSetErrMsg__ = TRUE ;
         }
      }
   }

   BOOLEAN sdbIsErrMsgEmpty()
   {
      if ( __errmsg__ && *__errmsg__ )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   INT32 sdbGetErrno()
   {
      return __errno__ ;
   }

   void sdbSetErrno( INT32 errNum, BOOLEAN replace )
   {
      if ( !replace && __hasSetErrNo__ )
      {
         /// not replace
         return ;
      }
      __errno__ = errNum ;
      __hasSetErrNo__ = errNum ? TRUE : FALSE ;
   }

   void sdbSetErrorObj( const CHAR *pObjData, UINT32 objSize )
   {
      __hasSetErrObj__ = FALSE ;

      if ( __errobjSize__ > 0 && __errobjSize__ < objSize )
      {
         SDB_OSS_FREE( __errobj__ ) ;
         __errobj__ = NULL ;
         __errobjSize__ = 0 ;
      }

      if ( __errobjSize__ < objSize )
      {
         UINT32 tmpSize = objSize + 50 ;
         __errobj__ = ( CHAR* )SDB_OSS_MALLOC( tmpSize ) ;
         if ( __errobj__ )
         {
            __errobjSize__ = tmpSize ;
         }
      }

      if ( objSize > 0 )
      {
         __hasSetErrObj__ = TRUE ;
         ossMemcpy( __errobj__, pObjData, objSize ) ;
      }
      else if ( __errobjSize__ >= sizeof( INT32 ) )
      {
         ossMemset( __errobj__, 0, sizeof( INT32 ) ) ;
      }
   }

   BOOLEAN sdbIsErrObjEmpty()
   {
      if ( __errobjSize__ < 5 || *(INT32*)__errobj__ < 5 )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   const CHAR* sdbGetErrorObj()
   {
      if ( !sdbIsErrObjEmpty() )
      {
         return __errobj__ ;
      }
      return NULL ;
   }

   void sdbHookFuncOnThreadExit()
   {
      if ( __errmsgSize__ > 0 )
      {
         SDB_OSS_FREE( __errmsg__ ) ;
         __errmsg__ = NULL ;
         __errmsgSize__ = 0 ;
      }
      if ( __errobjSize__ > 0 )
      {
         SDB_OSS_FREE( __errobj__ ) ;
         __errobj__ = NULL ;
         __errobjSize__ = 0 ;
      }
   }

   void sdbErrorCallback( const CHAR *pErrorObj,
                          UINT32 objSize,
                          INT32 flag,
                          const CHAR *pDescription,
                          const CHAR *pDetail )
   {
      sdbSetErrorObj( pErrorObj, objSize ) ;
      sdbSetErrno( flag, FALSE ) ;

      if ( pDescription && pDetail &&
           0 != ossStrcmp( pDescription, pDetail ) )
      {
         sdbSetErrMsgWithDetail( pDescription, pDetail, FALSE ) ;
      }
      else
      {
         sdbSetErrMsg( pDescription, FALSE ) ;
      }
   }

   void sdbClearErrorInfo()
   {
      sdbSetErrno( SDB_OK ) ;
      sdbSetErrMsg( NULL ) ;
      sdbSetErrorObj( NULL, 0 ) ;
   }

   BOOLEAN sdbNeedPrintError()
   {
      return __printError__ ;
   }

   void sdbSetPrintError( BOOLEAN print )
   {
      __printError__ = print ;
   }

   BOOLEAN sdbNeedIgnoreErrorPrefix()
   {
      return __ignoreErrorPrefix__ ;
   }

   void sdbSetIgnoreErrorPrefix( BOOLEAN ignore )
   {
      __ignoreErrorPrefix__ = ignore ;
   }

   void sdbSetReadData( BOOLEAN hasRead )
   {
      __hasReadData__ = hasRead ;
   }

   BOOLEAN sdbHasReadData()
   {
      return __hasReadData__ ;
   }

   void sdbSetNeedClearErrorInfo( BOOLEAN need )
   {
      __needClearErrorInfo__ = need ;
   }

   BOOLEAN sdbIsNeedClearErrorInfo()
   {
      return __needClearErrorInfo__ ;
   }

   void sdbReportError( JSContext *cx, const char *msg,
                        JSErrorReport *report )
   {
      const CHAR* filename = NULL ;
      UINT32 lineno = 0 ;

      // get privateData to get exception filename and lineno
      sptPrivateData *privateData = ( sptPrivateData* ) JS_GetContextPrivate( cx ) ;
      if( NULL != privateData && privateData->isSetErrInfo() )
      {
         filename = privateData->getErrFileName() ;
         lineno = privateData->getErrLineno() ;
      }
      else
      {
         filename = report->filename ;
         lineno = report->lineno ;
      }

      sdbReportError( filename , lineno, msg,
                      JSREPORT_IS_EXCEPTION( report->flags ) ) ;
      if( NULL != privateData )
      {
         privateData->clearErrInfo() ;
      }
   }

   void sdbReportError( const CHAR *filename, UINT32 lineno,
                        const CHAR *msg, BOOLEAN isException )
   {
      BOOLEAN add = FALSE ;

      if ( SDB_OK == sdbGetErrno() || !__hasSetErrNo__ )
      {
         const CHAR *p = NULL ;
         if ( isException && msg &&
              NULL != ( p = ossStrstr( msg, ":" ) ) &&
              0 != ossAtoi( p + 1 ) )
         {
            sdbSetErrno( ossAtoi( p + 1 ) ) ;
         }
         else
         {
            sdbSetErrno( SDB_SPT_EVAL_FAIL ) ;
         }
      }

      if ( ( sdbIsErrMsgEmpty() || !__hasSetErrMsg__ ) && msg )
      {
         sdbSetErrMsg( msg ) ;
         add = TRUE ;
      }

      if ( ( sdbIsErrObjEmpty() || !__hasSetErrObj__ ) &&
           SDB_OK != __errno__ )
      {
         _buildObjByErrno( __errno__, __errmsg__ ) ;
      }

      if ( sdbNeedPrintError() )
      {
         ossPrintf( "%s:%d %s\n",
                    filename ? filename : "(nofile)" ,
                    lineno, msg ) ;

         if ( !add && !sdbIsErrMsgEmpty() )
         {
            ossPrintf( "%s\n", sdbGetErrMsg() ) ;
         }
      }
      __hasSetErrMsg__ = FALSE ;
      __hasSetErrNo__  = FALSE ;
      __hasSetErrObj__ = FALSE ;

   }

   UINT32 sdbGetGlobalID()
   {
      static UINT32 _gid = 0 ;
      return ++_gid ;
   }

   BOOLEAN sptIsOpGetProperty( UINT32 opcode )
   {
      if ( SPT_JSOP_GETPROP == opcode ||
           SPT_JSOP_GETXPROP == opcode ||
           SPT_JSOP_GETLOCALPROP == opcode ||
           SPT_JSOP_GETARGPROP == opcode ||
           SPT_JSOP_GETTHISPROP == opcode ||
           SPT_JSOP_LENGTH == opcode )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN sptIsOpSetProperty( UINT32 opcode )
   {
      if ( SPT_JSOP_SETPROP == opcode ||
           SPT_JSOP_SETGNAME == opcode ||
           SPT_JSOP_SETNAME == opcode ||
           SPT_JSOP_SETMETHOD == opcode )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN sptIsOpCallProperty( UINT32 opcode )
   {
      if ( SPT_JSOP_CALL == opcode ||
           SPT_JSOP_FUNAPPLY == opcode ||
           SPT_JSOP_FUNCALL == opcode ||
           SPT_JSOP_CALLPROP == opcode )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void sdbSetIsNeedSaveHistory( BOOLEAN isNeedSaveHistory )
   {
      __isNeedSaveHistory__ = isNeedSaveHistory ;
   }

   BOOLEAN sdbIsNeedSaveHistory()
   {
      return __isNeedSaveHistory__ ;
   }

}

