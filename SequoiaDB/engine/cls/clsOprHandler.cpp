/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = clsOprHandler.cpp

   Descriptive Name = Operation Handler

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/05/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsOprHandler.hpp"
#include "clsCatalogAgent.hpp"
#include "clsMgr.hpp"
#include "ixmIndexKey.hpp"
#include "pmdEDU.hpp"
#include "pdSecure.hpp"
#include "clsTrace.hpp"

namespace engine
{
   _clsOprHandler::_clsOprHandler( _clsShdSession *shdSession,
                                   const CHAR *clName,
                                   BOOLEAN changeShardingKey )
   : _checkShardingKey( changeShardingKey ),
     _isInit( FALSE ),
     _shardSession( shdSession )
   {
      ossMemset( _mainCLName, 0, sizeof( _mainCLName ) ) ;
      ossStrncpy( _clName, clName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clName[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
   }

   _clsOprHandler::_clsOprHandler( _clsShdSession *shdSession,
                                   const CHAR *mainCLName, const CHAR *clName,
                                   BOOLEAN changeShardingKey )
   : _checkShardingKey( changeShardingKey ),
     _isInit( FALSE ),
     _shardSession( shdSession )
   {
      ossStrncpy( _mainCLName, mainCLName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _mainCLName[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
      ossStrncpy( _clName, clName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clName[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
   }

   INT32 _clsOprHandler::getShardingKey( const CHAR *clName,
                                         BSONObj &shardingKey )
   {
      return _shardSession->_getShardingKey( clName, shardingKey ) ;
   }

   void _clsOprHandler::onCSClosed( INT32 csID )
   {
   }

   void _clsOprHandler::onCLTruncated( INT32 csID, UINT16 clID )
   {
   }

   INT32 _clsOprHandler::onCreateIndex( _dmsMBContext *context,
                                        const ixmIndexCB *indexCB,
                                        _pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onDropIndex( _dmsMBContext *context,
                                      const ixmIndexCB *indexCB,
                                      _pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onRebuildIndex( _dmsMBContext *context,
                                         const ixmIndexCB *indexCB,
                                         _pmdEDUCB *cb,
                                         utilWriteResult *pResult )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onInsertRecord( _dmsMBContext *context,
                                         const BSONObj &object,
                                         const dmsRecordID &rid,
                                         const _dmsRecordRW *pRecordRW,
                                         _pmdEDUCB* cb )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onDeleteRecord( _dmsMBContext *context,
                                         const BSONObj &object,
                                         const dmsRecordID &rid,
                                         const _dmsRecordRW *pRecordRW,
                                         BOOLEAN markDeleting,
                                         _pmdEDUCB* cb )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSOPRHANDLER_ONUPDATERECORD, "_clsOprHandler::onUpdateRecord" )
   INT32 _clsOprHandler::onUpdateRecord( _dmsMBContext *context,
                                         const BSONObj &originalObj,
                                         const BSONObj &newObj,
                                         const dmsRecordID &rid,
                                         const _dmsRecordRW *pRecordRW,
                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSOPRHANDLER_ONUPDATERECORD ) ;

      BSONObj shardingKey ;

      if ( !_checkShardingKey )
      {
         goto done ;
      }

      if ( !_isInit )
      {
         rc = _init() ;
         PD_RC_CHECK( rc, PDERROR, "Init operation handler for collection[%s] "
                      "failed[%d]", context->mb()->_collectionName, rc ) ;
      }

      if ( _clKeygen.isInit() )
      {
         rc = _getAndValidShardKeyChange( _clKeygen, originalObj, newObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sharding key changing failed[%d]. "
                      "Old record: %s. New record: %s", rc,
                      PD_SECURE_OBJ( originalObj ), PD_SECURE_OBJ( newObj ) ) ;
      }

      if ( _mainCLKeygen.isInit() )
      {
         rc = _getAndValidShardKeyChange( _mainCLKeygen, originalObj,
                                          newObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sharding key of main collection "
                      "changing failed[%d]. Old record: %s. New record: %s", rc,
                      PD_SECURE_OBJ( originalObj ), PD_SECURE_OBJ( newObj ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSOPRHANDLER_ONUPDATERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsOprHandler::onInsertIndex( _dmsMBContext *context,
                                        const ixmIndexCB *indexCB,
                                        BOOLEAN isUnique,
                                        BOOLEAN isEnforce,
                                        const BSONObjSet &keySet,
                                        const dmsRecordID &rid,
                                        _pmdEDUCB* cb,
                                        utilWriteResult *pResult )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onInsertIndex( _dmsMBContext *context,
                                        const ixmIndexCB *indexCB,
                                        BOOLEAN isUnique,
                                        BOOLEAN isEnforce,
                                        const BSONObj &keyObj,
                                        const dmsRecordID &rid,
                                        _pmdEDUCB* cb,
                                        utilWriteResult *pResult )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onUpdateIndex( _dmsMBContext *context,
                                        const ixmIndexCB *indexCB,
                                        BOOLEAN isUnique,
                                        BOOLEAN isEnforce,
                                        const BSONObjSet &oldKeySet,
                                        const BSONObjSet &newKeySet,
                                        const dmsRecordID &rid,
                                        BOOLEAN isRollback,
                                        _pmdEDUCB* cb,
                                        utilWriteResult *pResult )
   {
      return SDB_OK ;
   }

   INT32 _clsOprHandler::onDeleteIndex( _dmsMBContext *context,
                                        const ixmIndexCB *indexCB,
                                        BOOLEAN isUnique,
                                        const BSONObjSet &keySet,
                                        const dmsRecordID &rid,
                                        _pmdEDUCB* cb )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSOPRHANDLER__INIT, "_clsOprHandler::_init" )
   INT32 _clsOprHandler::_init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSOPRHANDLER__INIT ) ;

      if ( _isInit )
      {
         goto done ;
      }

      if ( ossStrlen( _mainCLName ) > 0 )
      {
         rc = _prepareShardKeyKeygen( _mainCLName, _mainCLKeygen ) ;
         PD_RC_CHECK( rc, PDERROR, "Prepare sharding key keygen for main "
                      "collection[%s] failed[%d]", _mainCLName, rc ) ;
      }

      if ( ossStrlen( _clName ) > 0 )
      {
         rc = _prepareShardKeyKeygen( _clName, _clKeygen ) ;
         PD_RC_CHECK( rc, PDERROR, "Prepare sharding key keygen for "
                      "collection[%s] failed[%d]", _clName, rc ) ;
      }

      _isInit = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CLSOPRHANDLER__INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSOPRHANDLER__PREPARESHARDKEYKEYGEN, "_clsOprHandler::_prepareShardKeyKeygen" )
   INT32 _clsOprHandler::_prepareShardKeyKeygen( const CHAR *clName,
                                                 _ixmIndexKeyGen &keygen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSOPRHANDLER__PREPARESHARDKEYKEYGEN ) ;

      BSONObj shardingKey ;

      rc = getShardingKey( clName, shardingKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Get sharding key of collection[%s] failed[%d]",
                   clName, rc ) ;

      // If the sharding key is empty, but the keygen has been initialized
      // already, need to set it with empty sharding key.
      if ( !shardingKey.isEmpty() )
      {
         rc = keygen.setKeyPattern( shardingKey ) ;
         PD_RC_CHECK( rc, PDERROR, "Init sharding key keygen for "
                      "collection[%s] failed[%d]", clName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSOPRHANDLER__PREPARESHARDKEYKEYGEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSOPRHANDLER__GETANDVALIDSHARDKEYCHANGE, "_clsOprHandler::_getAndValidShardKeyChange" )
   INT32 _clsOprHandler::_getAndValidShardKeyChange( ixmIndexKeyGen &keygen,
                                                     const BSONObj &originalObj,
                                                     const BSONObj &newObj,
                                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSOPRHANDLER__GETANDVALIDSHARDKEYCHANGE ) ;

      try
      {
         BSONObjSet oldKeySet ;
         BSONObjSet newKeySet ;
         rc = keygen.getKeys( originalObj, oldKeySet, NULL, TRUE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get sharding key from record[%s] "
                      "failed[%d]", PD_SECURE_OBJ( originalObj), rc ) ;
         rc = keygen.getKeys( newObj, newKeySet, NULL, TRUE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get sharding key from record[%s] "
                      "failed[%d]", PD_SECURE_OBJ( newObj ), rc ) ;

         if ( 1 != oldKeySet.size() || 1 != newKeySet.size() )
         {
            SDB_ASSERT( FALSE, "Key set size is wrong" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Sharding key from record is invalid[%d]",
                    rc ) ;
            goto error ;
         }

         if ( 0 != oldKeySet.begin()->woCompare( *newKeySet.begin() ) )
         {
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "Updating sharding key to a different value is "
                    "not allowed[%d]", rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSOPRHANDLER__GETANDVALIDSHARDKEYCHANGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
