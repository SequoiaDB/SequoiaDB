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

   Source File Name = coordCommandRecycleBin.cpp

   Descriptive Name = Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Init
   Last Changed =

*******************************************************************************/

#include "coordCommandRecycleBin.hpp"
#include "coordCB.hpp"
#include "coordUtil.hpp"
#include "msgMessage.hpp"
#include "catDef.hpp"
#include "utilRecycleBinConf.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordGetRecycleBinDetail implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordGetRecycleBinDetail,
                                      CMD_NAME_GET_RECYCLEBIN_DETAIL,
                                      TRUE ) ;
   _coordGetRecycleBinDetail::_coordGetRecycleBinDetail()
   {
   }

   _coordGetRecycleBinDetail::~_coordGetRecycleBinDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDGETRECYCLEBINDETAIL__PREPROCESS, "_coordGetRecycleBinDetail::_preProcess" )
   INT32 _coordGetRecycleBinDetail::_preProcess( rtnQueryOptions &queryOpt,
                                                 string &clName,
                                                 BSONObj &outSelector )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDGETRECYCLEBINDETAIL__PREPROCESS ) ;

      // do nothing

      PD_TRACE_EXITRC( SDB_COORDGETRECYCLEBINDETAIL__PREPROCESS, rc ) ;

      return rc ;
   }

   /*
      _coordAlterRecycleBin implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordAlterRecycleBin,
                                      CMD_NAME_ALTER_RECYCLEBIN,
                                      TRUE ) ;
   _coordAlterRecycleBin::_coordAlterRecycleBin()
   {
   }

   _coordAlterRecycleBin::~_coordAlterRecycleBin()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDALTERRECYCLEBIN_EXECUTE, "_coordAlterRecycleBin::execute" )
   INT32 _coordAlterRecycleBin::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDALTERRECYCLEBIN_EXECUTE ) ;

      CoordGroupList groupList ;

      rc = executeOnCataGroup( pMsg, cb, NULL, NULL, TRUE, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on catalog, "
                   "rc: %d", getName(), rc ) ;

      // update all groups
      rc = _pResource->updateGroupList( groupList, cb, NULL, TRUE, TRUE, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update all data group, "
                   "rc: %d", rc ) ;

      rc = executeOnDataGroup( pMsg, cb, groupList, TRUE, NULL, NULL, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute %s on data nodes, rc: %d",
                   getName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_COORDALTERRECYCLEBIN_EXECUTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordGetRecycleBinCount implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordGetRecycleBinCount,
                                      CMD_NAME_GET_RECYCLEBIN_COUNT,
                                      TRUE ) ;
   _coordGetRecycleBinCount::_coordGetRecycleBinCount()
   {
   }

   _coordGetRecycleBinCount::~_coordGetRecycleBinCount()
   {
   }

   /*
      _coordDropRecycleBinBase implement
    */
   _coordDropRecycleBinBase::_coordDropRecycleBinBase( BOOLEAN isDropAll )
   : _isDropAll( isDropAll ),
     _recycleItemName( NULL )
   {
   }

   _coordDropRecycleBinBase::~_coordDropRecycleBinBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDDROPRECYBINBASE_EXECUTE, "_coordDropRecycleBinBase::execute" )
   INT32 _coordDropRecycleBinBase::execute( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDDROPRECYBINBASE_EXECUTE ) ;

      CoordGroupList groupList, sucGroupList ;
      BOOLEAN needAllGroups = _isDropAll ;

      rc = _parseMessage( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message for command [%s], "
                   "rc: %d", getName(), rc ) ;

      rc = executeOnCataGroup( pMsg, cb, &groupList, NULL, TRUE, NULL, buf ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         // recycle bin item does not exist in CATALOG, send to all groups
         // to drop remaining items in DATA nodes
         rc = SDB_OK ;
         needAllGroups = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on catalog, "
                   "rc: %d", getName(), rc ) ;

      if ( needAllGroups )
      {
         // use all groups
         rc = _pResource->updateGroupList( groupList, cb, NULL, TRUE, TRUE, TRUE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update all data group for "
                      "command [%s], rc: %d", getName(), rc ) ;
      }

      rc = executeOnDataGroup( pMsg, cb, groupList, TRUE, NULL, &sucGroupList,
                               NULL, buf ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc && sucGroupList.size() > 0 )
      {
         // at least one group found the item, we can ignore the error
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on data, "
                   "rc: %d", getName(), rc ) ;

   done:
      _doAudit( rc ) ;

      PD_TRACE_EXITRC( SDB_COORDDROPRECYBINBASE_EXECUTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDDROPRECYBINBASE__PARSEMSG "_coordDropRecycleBinBase::_parseMessage" )
   INT32 _coordDropRecycleBinBase::_parseMessage( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDDROPRECYBINBASE__PARSEMSG ) ;

      try
      {
         const CHAR *pQuery = NULL ;

         rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL,
                               &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse message for command [%s], "
                      "rc: %d", getName(), rc ) ;

         _options.init( pQuery ) ;

         if ( _isDropAll )
         {
            BSONElement element = _options.getField( FIELD_NAME_RECYCLE_NAME ) ;
            PD_CHECK( EOO == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse message, should not get field [%s] "
                      "from options", FIELD_NAME_RECYCLE_NAME ) ;
         }
         else
         {
            utilRecycleItem dummyItem ;

            BSONElement element = _options.getField( FIELD_NAME_RECYCLE_NAME ) ;
            PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse message, failed to get field [%s]",
                      FIELD_NAME_RECYCLE_NAME ) ;
            _recycleItemName = element.valuestr() ;

            rc = dummyItem.fromRecycleName( _recycleItemName ) ;
            PD_LOG_MSG_CHECK( SDB_OK == rc, rc, error, PDERROR,
                              "Failed to parse recycle item name [%s], "
                              "rc: %d", _recycleItemName, rc ) ;

            BSONElement e1  = _options.getField(  FIELD_NAME_RECURSIVE ) ;
            PD_CHECK( Bool == e1.type() || EOO == e1.type(), SDB_INVALIDARG,
                      error, PDERROR, "Failed to parse message, failed to "
                      "get field [%s] from options", FIELD_NAME_RECURSIVE ) ;
         }
         BSONElement e2 = _options.getField( FIELD_NAME_ASYNC ) ;
         PD_CHECK( Bool == e2.type() || EOO == e2.type(), SDB_INVALIDARG,
                   error, PDERROR, "Failed to parse message, failed to "
                   "get field [%s] from options", FIELD_NAME_ASYNC ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse message, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_COORDDROPRECYBINBASE__PARSEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDDROPRECYBINBASE__DOAUDIT "_coordDropRecycleBinBase::_doAudit" )
   INT32 _coordDropRecycleBinBase::_doAudit( INT32 rc )
   {
      PD_TRACE_ENTRY( SDB_COORDDROPRECYBINBASE__DOAUDIT ) ;

      PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_RECYCLEBIN,
                        ( NULL != _recycleItemName ? _recycleItemName : "" ),
                        rc, "Option: %s", _options.toPoolString().c_str() ) ;

      PD_TRACE_EXIT( SDB_COORDDROPRECYBINBASE__DOAUDIT ) ;

      return SDB_OK ;
   }

   /*
      _coordDropRecycleBinItem implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordDropRecycleBinItem,
                                      CMD_NAME_DROP_RECYCLEBIN_ITEM,
                                      TRUE ) ;

   _coordDropRecycleBinItem::_coordDropRecycleBinItem()
   : _coordDropRecycleBinBase( FALSE )
   {
   }

   _coordDropRecycleBinItem::~_coordDropRecycleBinItem()
   {
   }

   /*
      _coordDropRecycleBin implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordDropRecycleBinAll,
                                      CMD_NAME_DROP_RECYCLEBIN_ALL,
                                      TRUE ) ;

   _coordDropRecycleBinAll::_coordDropRecycleBinAll()
   : _coordDropRecycleBinBase( TRUE )
   {
   }

   _coordDropRecycleBinAll::~_coordDropRecycleBinAll()
   {
   }

   /*
      _coordReturnRecycleBinBase implement
    */
   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDRTRNRECYCLEBINBASE_REGEVENTHANDLERS, "_coordReturnRecycleBinBase::_regEventHandlers" )
   INT32 _coordReturnRecycleBinBase::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDRTRNRECYCLEBINBASE_REGEVENTHANDLERS ) ;

      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_returnHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register return handler, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_COORDRTRNRECYCLEBINBASE_REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDRTRNRECYBINBASE__DOAUDIT "_coordReturnRecycleBinBase::_doAudit" )
   INT32 _coordReturnRecycleBinBase::_doAudit( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY( SDB_COORDRTRNRECYBINBASE__DOAUDIT ) ;

      PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_RECYCLEBIN,
                        pArgs->_targetName.c_str(), rc, "Option: %s",
                        pArgs->_boQuery.toString().c_str() ) ;

      PD_TRACE_EXIT( SDB_COORDRTRNRECYBINBASE__DOAUDIT ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RTRNRECYBINBASE__DOOUTPUT "_coordReturnRecycleBinBase::_doOutput" )
   INT32 _coordReturnRecycleBinBase::_doOutput( rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_RTRNRECYBINBASE__DOOUTPUT ) ;

      utilRecycleItem recycleItem ;

      if ( NULL == buf )
      {
         goto done ;
      }

      if ( SDB_OK == recycleItem.fromBSON(
                                 _recycleHandler.getRecycleOptions() ) )
      {
         utilReturnNameInfo info( recycleItem.getOriginName() ) ;
         if ( UTIL_RECYCLE_CS == recycleItem.getType() )
         {

            rc = _returnHandler.getReturnInfo().getReturnCSName( info ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                         "collection space [%s], rc: %d",
                         info.getOriginName(), rc ) ;

         }
         else if ( UTIL_RECYCLE_CL == recycleItem.getType() )
         {
            rc = _returnHandler.getReturnInfo().getReturnCLName( info ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                         "collection [%s], rc: %d",
                         info.getOriginName(), rc ) ;
         }

         if ( info.isRenamed() )
         {
            PD_LOG( PDDEBUG, "Return recycle item renamed from [%s] to [%s]",
                    info.getOriginName(), info.getReturnName() ) ;
         }

         try
         {
            BSONObjBuilder builder ;
            builder.append( FIELD_NAME_RETURN_NAME, info.getReturnName() ) ;
            *buf = rtnContextBuf( builder.obj() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build reply BSON object, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_RTRNRECYBINBASE__DOOUTPUT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordReturnRecycleBinItem implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordReturnRecycleBinItem,
                                      CMD_NAME_RETURN_RECYCLEBIN_ITEM,
                                      TRUE ) ;

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDRTRNRECYCLEBINITEM__PARSEMSG, "_coordReturnRecycleBinItem::_parseMsg" )
   INT32 _coordReturnRecycleBinItem::_parseMsg( MsgHeader *pMsg,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDRTRNRECYCLEBINITEM__PARSEMSG ) ;

      try
      {
         utilRecycleItem dummyItem ;
         const CHAR *recycleName = NULL ;

         BSONElement element = pArgs->_boQuery.getField( FIELD_NAME_RECYCLE_NAME ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s] from command",
                   FIELD_NAME_RECYCLE_NAME ) ;
         recycleName = element.valuestr() ;

         rc = dummyItem.fromRecycleName( recycleName ) ;
         PD_LOG_MSG_CHECK( SDB_OK == rc, rc, error, PDERROR,
                           "Failed to parse recycle item name [%s], "
                           "rc: %d", recycleName, rc ) ;

         pArgs->_targetName.assign( recycleName ) ;

         PD_LOG_MSG_CHECK( !pArgs->_boQuery.hasField( FIELD_NAME_RETURN_NAME ),
                           SDB_INVALIDARG, error, PDERROR,
                           "Failed to parse message, should not have "
                           "field [%s]", FIELD_NAME_RETURN_NAME ) ;

         // as rename, retry when lock failed in data node
         pArgs->_retryRCList.insert( SDB_LOCK_FAILED ) ;
      }
      catch( exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_COORDRTRNRECYCLEBINITEM__PARSEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordReturnRecycleBinItemToName implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordReturnRecycleBinItemToName,
                                      CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME,
                                      TRUE ) ;

   // PD_TRACE_DECLARE_FUNCTION( SDB_COORDRTRNRECYCLEBINITEMTONAME__PARSEMSG, "_coordReturnRecycleBinItemToName::_parseMsg" )
   INT32 _coordReturnRecycleBinItemToName::_parseMsg( MsgHeader *pMsg,
                                                      coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDRTRNRECYCLEBINITEMTONAME__PARSEMSG ) ;

      try
      {
         utilRecycleItem dummyItem ;
         const CHAR *recycleName = NULL ;

         BSONElement element ;

         element = pArgs->_boQuery.getField( FIELD_NAME_RECYCLE_NAME ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s] from command",
                   FIELD_NAME_RECYCLE_NAME ) ;
         recycleName = element.valuestr() ;

         rc = dummyItem.fromRecycleName( recycleName ) ;
         PD_LOG_MSG_CHECK( SDB_OK == rc, rc, error, PDERROR,
                           "Failed to parse recycle item name [%s], "
                           "rc: %d", recycleName, rc ) ;

         pArgs->_targetName.assign( recycleName ) ;

         element = pArgs->_boQuery.getField( FIELD_NAME_RETURN_NAME ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s] from command",
                   FIELD_NAME_RETURN_NAME ) ;

         PD_LOG_MSG_CHECK( !pArgs->_boQuery.hasField( FIELD_NAME_ENFORCED ),
                           SDB_INVALIDARG, error, PDERROR,
                           "Failed to parse message, should not have "
                           "field [%s]", FIELD_NAME_ENFORCED ) ;
         PD_LOG_MSG_CHECK( !pArgs->_boQuery.hasField( FIELD_NAME_ENFORCED1 ),
                           SDB_INVALIDARG, error, PDERROR,
                           "Failed to parse message, should not have "
                           "field [%s]", FIELD_NAME_ENFORCED1 ) ;

         // as rename, retry when lock failed in data node
         pArgs->_retryRCList.insert( SDB_LOCK_FAILED ) ;
      }
      catch( exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_COORDRTRNRECYCLEBINITEMTONAME__PARSEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
