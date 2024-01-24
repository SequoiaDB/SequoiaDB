/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = seAdptSEAssist.cpp

   Descriptive Name = Search engine assistant for search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/14/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/

#include "seAdptSEAssist.hpp"
#include "pd.hpp"

namespace seadapter
{
   _seAdptSEAssist::_seAdptSEAssist()
   : _bulkBuffSz( 0 )
   {
      _esCltMgr = utilGetESCltMgr() ;
      ossMemset( _index, 0, SEADPT_MAX_IDXNAME_SZ + 1 ) ;
      ossMemset( _type, 0, SEADPT_MAX_TYPE_SZ + 1 ) ;
   }

   _seAdptSEAssist::~_seAdptSEAssist()
   {
   }

   INT32 _seAdptSEAssist::init( UINT32 bulkBuffSz )
   {
      _bulkBuffSz = bulkBuffSz ;
      return SDB_OK ;
   }

   INT32 _seAdptSEAssist::createIndex( const CHAR *name, const CHAR *mapping )
   {
      utilESClt *client = NULL ;
      INT32 rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      rc = client->createIndex( name, mapping ) ;
      PD_RC_CHECK( rc, PDERROR, "Create index[%s] on search engine failed[%d]",
                   name, rc ) ;

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::dropIndex( const CHAR *name )
   {
      utilESClt *client = NULL ;
      INT32 rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      rc = client->dropIndex( name ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop index[%s] on search engine failed[%d]",
                   name, rc ) ;

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::indexExist( const CHAR *name, BOOLEAN &exist )
   {
      utilESClt *client = NULL ;
      INT32 rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      rc = client->indexExist( name, exist ) ;
      PD_RC_CHECK( rc, PDERROR, "Check index existance on search engine "
                   "failed[%d]", rc ) ;
   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::getDocument( const CHAR *index, const CHAR *type,
                                       const CHAR *id, BSONObj &result )
   {
      utilESClt *client = NULL ;
      INT32 rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      rc = client->getDocument( index, type, id, result, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Get document from search engine failed[%d]",
                   rc ) ;

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::indexDocument( const CHAR *index, const CHAR *type,
                                         const CHAR *id, const CHAR *jsonData )
   {
      utilESClt *client = NULL ;
      INT32 rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;
      rc = client->indexDocument( index, type, id, jsonData ) ;
      PD_RC_CHECK( rc, PDERROR, "Index document on search engine failed[%d]",
                   rc ) ;
   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::bulkPrepare( const CHAR *index, const CHAR *type )
   {
      INT32 rc = SDB_OK ;
      if ( !_bulkBuilder.isInit() )
      {
         rc = _bulkBuilder.init( _bulkBuffSz ) ;
         PD_RC_CHECK( rc, PDERROR, "Initialize bulk builder failed[ %d ]",
                      rc ) ;
      }
      else
      {
         _bulkBuilder.reset() ;
      }

      ossStrncpy( _index, index, SEADPT_MAX_IDXNAME_SZ + 1 ) ;
      ossStrncpy( _type, type, SEADPT_MAX_TYPE_SZ + 1 ) ;
      _oprMon.reset() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::bulkProcess( const utilESBulkActionBase &actionItem )
   {
      INT32 rc = SDB_OK ;
      utilESClt *client = NULL ;
      rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      // If reaching the size limit of the bulk builder, fire the operation and
      // go on with the next batch of data.
      if ( actionItem.outSizeEstimate() > _bulkBuilder.getFreeSize() )
      {
         if ( _bulkBuilder.getItemNum() > 0 )
         {
            rc = client->bulk( _index, _type, _bulkBuilder.getData() ) ;
            PD_RC_CHECK( rc, PDERROR, "Bulk operation failed[ %d ]" ) ;

            PD_LOG( PDDEBUG, "Index documents in bulk mode successfully. "
                             "Document number[%u], size[%u]. Detail: insert[%u], "
                             "update[%u], delete[%u], ignore[%u], total "
                             "insert[%llu], total update[%llu], "
                             "total delete[%llu], total ignore[%llu]",
                    _bulkBuilder.getItemNum(), _bulkBuilder.getDataLen(),
                    _oprMon._insertCount, _oprMon._updateCount,
                    _oprMon._deleteCount, _oprMon._ignoreCount,
                    _oprMon._totalInsertCount, _oprMon._totalUpdateCount,
                    _oprMon._totalDeleteCount, _oprMon._totalIgnoreCount ) ;

            _bulkBuilder.reset() ;
            _oprMon.reset() ;
         }
         else
         {
            // Buffer is not enough. Process it separately instead of using
            // _bulk.
            rc = processBigItem( actionItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Process big item failed[ %d ]", rc ) ;
            goto done ;
         }
      }
      rc = _bulkBuilder.appendItem( actionItem, FALSE, FALSE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Append item to bulk builder failed[ %d ]",
                   rc ) ;

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32
   _seAdptSEAssist::processBigItem( const utilESBulkActionBase &actionItem )
   {
      INT32 rc = SDB_OK ;
      utilESClt *client = NULL ;
      rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      // Currently only index and update may exceed bulk buffer. They both use
      // the indexDocument interface.
      SDB_ASSERT( UTIL_ES_ACTION_INDEX == actionItem.getActionType(),
                  "Type is not index" ) ;

      rc = client->
           indexDocument( actionItem.getIndexName(),
                           actionItem.getTypeName(),
                           actionItem.getID(),
                           actionItem.getSrcData().toString(false, true).c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Index document failed[ %d ]", rc ) ;

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::bulkFinish()
   {
      INT32 rc = SDB_OK ;
      utilESClt *client = NULL ;
      rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      if ( _bulkBuilder.getDataLen() > 0 )
      {
         rc = client->bulk( _index, _type, _bulkBuilder.getData() ) ;
         PD_RC_CHECK( rc, PDERROR, "Bulk operation failed[%d]", rc ) ;
         PD_LOG( PDDEBUG, "Index documents in bulk mode successfully. "
                          "Document number[%u], size[%u]. Detail: insert[%u], "
                          "update[%u], delete[%u], ignore[%u], total "
                          "insert[%llu], total update[%llu], "
                          "total delete[%llu], total ignore[%llu]",
                 _bulkBuilder.getItemNum(), _bulkBuilder.getDataLen(),
                 _oprMon._insertCount, _oprMon._updateCount,
                 _oprMon._deleteCount, _oprMon._ignoreCount,
                 _oprMon._totalInsertCount, _oprMon._totalUpdateCount,
                 _oprMon._totalDeleteCount, _oprMon._totalIgnoreCount ) ;
         _oprMon.reset() ;
      }

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32
   _seAdptSEAssist::bulkAppendIndex( const CHAR *docID, const BSONObj &doc )
   {
      INT32 rc = SDB_OK ;

      utilESActionIndex item( _index, _type ) ;
      rc = item.setID( docID ) ;
      PD_RC_CHECK( rc, PDERROR, "Set _id for action index failed[%d]", rc ) ;
      item.setSourceData( doc ) ;
      rc = bulkProcess( item ) ;
      PD_RC_CHECK( rc, PDERROR, "Bulk processing item failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptSEAssist::bulkAppendDel( const CHAR *docID )
   {
      INT32 rc = SDB_OK ;

      utilESActionDelete item( _index, _type ) ;
      rc = item.setID( docID ) ;
      PD_RC_CHECK( rc, PDERROR, "Set _id for action delete failed[%d]", rc ) ;
      rc = bulkProcess( item ) ;
      PD_RC_CHECK( rc, PDERROR, "Bulk processing item failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32
   _seAdptSEAssist::bulkAppendReplace( const CHAR *id, const CHAR *newId,
                                       const BSONObj &document )
   {
      INT32 rc = SDB_OK ;

      if ( id )
      {
         rc = bulkAppendDel( id ) ;
         PD_RC_CHECK( rc, PDERROR, "delete document of _id[%s] failed[%d]",
                      id, rc ) ;
      }

      if ( newId )
      {
         rc = bulkAppendIndex( newId, document ) ;
         PD_RC_CHECK( rc, PDERROR, "Index document[%s] with _id[%s] "
                                   "failed[%d]",
                      document.toString(false, true).c_str(), newId,
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
