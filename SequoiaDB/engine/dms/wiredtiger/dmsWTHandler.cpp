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

   Source File Name = dmsWTHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTHandler.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "../util/fromjson.hpp"

using namespace bson ;

namespace engine
{
namespace wiredtiger
{

   static PDLEVEL _dmsGetWTLogPDLevel( const BSONObj& obj )
   {
      const CHAR *s_field = "verbose_level_id" ;

      if ( !obj.hasField( s_field ) )
      {
         PD_LOG( PDDEBUG, "Failed to get field [%s] from WiredTiger log "
                 "object [%s], field is not found",
                 s_field, obj.toPoolString().c_str() ) ;
         return PDERROR ;
      }

      BSONElement logLevelEle = obj.getField( s_field ) ;
      if ( !logLevelEle.isNumber() )
      {
         PD_LOG( PDDEBUG, "Failed to get field [%s] from WiredTiger log "
                 "object [%s], field is not a number",
                 s_field, obj.toPoolString().c_str() ) ;
         return PDERROR ;
      }

      switch ( logLevelEle.Int() )
      {
        case WT_VERBOSE_ERROR:
            return PDERROR ;
        case WT_VERBOSE_WARNING:
            return PDWARNING ;
        case WT_VERBOSE_NOTICE:
            return PDEVENT ;
        case WT_VERBOSE_INFO:
            return PDINFO ;
        case WT_VERBOSE_DEBUG:
            return PDDEBUG ;
        default:
            return PDERROR ;
      }
   }

   static int _dmsWTHandleError( WT_EVENT_HANDLER *handler,
                                 WT_SESSION *session,
                                 int errorCode,
                                 const char *message )
   {
      PD_LOG( PDERROR, "[WiredTiger] error: %d, message: %s",
              errorCode, message ) ;

      // TODO HGM: Don't abort on WT_PANIC when repairing,
      // as the error will be handled at a higher layer.
      if ( errorCode == WT_PANIC )
      {
         ossPanic() ;
      }
      return 0 ;
   }

   static int _dmsWTHandleMessage( WT_EVENT_HANDLER *handler,
                                   WT_SESSION *session,
                                   const char *message )
   {
      PDLEVEL level = PDERROR ;

      try {
         // Parse the WT JSON message string.
         BSONObj msgObj ;
         INT32 tmpRC = fromjson( message, msgObj ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDDEBUG, "Failed to parsr WiredTiger log message [%s], "
                    "rc: %d", message, tmpRC ) ;
            level = PDERROR ;
         }
         else
         {
            level = _dmsGetWTLogPDLevel( msgObj ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDDEBUG, "Failed to parsr WiredTiger log message [%s], "
                 "occur exception: %s", message, e.what() ) ;
         level = PDERROR ;
      }

      PD_LOG( level, "[WiredTiger] message: %s", message ) ;

      return 0 ;
   }

   static int _dmsWTHandleProgress( WT_EVENT_HANDLER *handler,
                                    WT_SESSION *session,
                                    const char *operation,
                                    uint64_t progress )
   {
      PD_LOG( PDEVENT, "[WiredTiger] %s progress: %llu",
              operation, progress ) ;
      return 0 ;
   }

   /*
      _dmsWTHandler implement
    */
   _dmsWTHandler::_dmsWTHandler()
   {
      _handler.handle_error = _dmsWTHandleError ;
      _handler.handle_message = _dmsWTHandleMessage ;
      _handler.handle_progress = _dmsWTHandleProgress ;
      _handler.handle_close = nullptr ;
   }

}
}
