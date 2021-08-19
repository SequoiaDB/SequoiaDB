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

   Source File Name = sptCommon.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPTCOMMON_HPP__
#define SPTCOMMON_HPP__

#include "core.h"
#include "jsapi.h"

#define CMD_HELP           "help"
#define CMD_QUIT           "quit"
#define CMD_QUIT1          "quit;"
#define CMD_CLEAR          "clear"
#define CMD_CLEAR1         "clear;"
#define CMD_CLEARHISTORY   "history-c"
#define CMD_CLEARHISTORY1  "history-c;"

#define SPT_OBJ_CNAME_PROPNAME      "__className__"
#define SPT_OBJ_ID_PROPNAME         "__id__"
#define SPT_SPE_OBJSTART            '$'
namespace engine
{
   /*
      Global function define
   */
   const CHAR *sdbGetErrMsg() ;
   void  sdbSetErrMsg( const CHAR *err, BOOLEAN replace = TRUE ) ;
   void  sdbSetErrMsgWithDetail( const CHAR *err, const CHAR *detail,
                                 BOOLEAN replace = TRUE ) ;
   BOOLEAN sdbIsErrMsgEmpty() ;

   INT32 sdbGetErrno() ;
   void  sdbSetErrno( INT32 errNum, BOOLEAN replace = TRUE ) ;

   /*
      The use CHAR* for object data, because the function will called in
      c-bson and c++ bson, c-bson and c++ bson can't compatiable
   */
   const CHAR* sdbGetErrorObj() ;
   void  sdbSetErrorObj( const CHAR *pObjData, UINT32 objSize ) ;

   /*
      The hook function is registered to thread's exit hook func
   */
   void  sdbHookFuncOnThreadExit() ;

   /*
      The callback function is registered to driver
   */
   void  sdbErrorCallback( const CHAR *pErrorObj,
                           UINT32 objSize,
                           INT32 flag,
                           const CHAR *pDescription,
                           const CHAR *pDetail ) ;

   // clear msg and errno
   void  sdbClearErrorInfo() ;

   BOOLEAN  sdbNeedPrintError() ;
   void     sdbSetPrintError( BOOLEAN print ) ;

   BOOLEAN  sdbNeedIgnoreErrorPrefix() ;
   void     sdbSetIgnoreErrorPrefix( BOOLEAN ignore ) ;

   void     sdbSetReadData( BOOLEAN hasRead ) ;
   BOOLEAN  sdbHasReadData() ;

   void     sdbSetNeedClearErrorInfo( BOOLEAN need ) ;
   BOOLEAN  sdbIsNeedClearErrorInfo() ;

   void     sdbReportError( JSContext *cx, const char *msg,
                            JSErrorReport *report ) ;

   void     sdbReportError( const CHAR *filename, UINT32 lineno,
                            const CHAR *msg, BOOLEAN isException ) ;

   UINT32   sdbGetGlobalID() ;

   BOOLEAN sptIsOpGetProperty( UINT32 opcode ) ;
   BOOLEAN sptIsOpSetProperty( UINT32 opcode ) ;
   BOOLEAN sptIsOpCallProperty( UINT32 opcode ) ;

   void    sdbSetIsNeedSaveHistory( BOOLEAN isNeedSaveHistory ) ;
   BOOLEAN sdbIsNeedSaveHistory() ;

}

#endif //SPTCOMMON_HPP__

