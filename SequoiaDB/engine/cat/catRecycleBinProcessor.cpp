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

   Source File Name = catRecycleBinProcessor.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catRecycleBinProcessor.hpp"
#include "catCommon.hpp"
#include "catalogueCB.hpp"
#include "catRecycleBinManager.hpp"
#include "rtn.hpp"
#include "catGTSDef.hpp"
#include "clsCatalogAgent.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   /*
      _catRecycleBinProcessor implement
    */
   _catRecycleBinProcessor::_catRecycleBinProcessor( _catRecycleBinManager *recyBinMgr,
                                                     utilRecycleItem &item )
   : _recyBinMgr( recyBinMgr ),
     _item( item ),
     _matchedCount( 0 ),
     _processedCount( 0 )
   {
   }

   _catRecycleBinProcessor::~_catRecycleBinProcessor()
   {
   }

   /*
      _catDropItemSubCLChecker implement
    */
   _catDropItemSubCLChecker::_catDropItemSubCLChecker( _catRecycleBinManager *recyBinMgr,
                                                       utilRecycleItem &item )
   : _catRecycleBinProcessor( recyBinMgr, item )
   {
   }

   _catDropItemSubCLChecker::~_catDropItemSubCLChecker()
   {
   }

   const CHAR *_catDropItemSubCLChecker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK_GETMATCHER, "_catDropItemSubCLChecker::getMatcher" )
   INT32 _catDropItemSubCLChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK_GETMATCHER ) ;

      try
      {
         // matched by recycle ID and main-collection name
         BSONObj matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                                 (INT64)( _item.getRecycleID() ) <<
                                 FIELD_NAME_MAINCLNAME <<
                                 _item.getOriginName() ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return checker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATDROPITEMSUBCLCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK_PROCESSOBJ, "_catDropItemSubCLChecker::processObject" )
   INT32 _catDropItemSubCLChecker::processObject( const BSONObj &object,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK_PROCESSOBJ ) ;

      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      utilRecycleItem csItem ;
      const CHAR *subCLName = NULL ;

      rc = catParseCLUniqueID( object, clUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID from object, "
                   "rc: %d", rc ) ;
      csUniqueID = utilGetCSUniqueID( clUniqueID ) ;

      if ( _isChecked( csUniqueID ) )
      {
         // already checked
         goto done ;
      }

      // check if has a recycled collection space
      rc = _recyBinMgr->getItemByOrigID( csUniqueID, cb, csItem ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         // if not found, it is safe to drop
         // save to checked list to avoid duplicated checks
         rc = _saveChecked( csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save checked collection space, "
                      "rc: %d", rc ) ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item of collection "
                   "space, rc: %d", rc ) ;

      // found recycled collection space, report error, we should not drop it
      rc = rtnGetStringElement( object, CAT_COLLECTION_NAME, &subCLName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from object, "
                   "rc: %d", CAT_COLLECTION_NAME, rc ) ;
      PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                        "Failed to drop collection recycle item "
                        "[origin %s, recycle %s] with sub-collection [%s], "
                        "collection space recycle item "
                        "[origin: %s, recycle: %s] is also "
                        "in recycle bin", _item.getOriginName(),
                        _item.getRecycleName(), subCLName,
                        csItem.getOriginName(), csItem.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATDROPITEMSUBCLCHK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK__ISCHECKED, "_catDropItemSubCLChecker::_isChecked" )
   BOOLEAN _catDropItemSubCLChecker::_isChecked( utilCSUniqueID csUniqueID ) const
   {
      BOOLEAN isChecked = FALSE ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK__ISCHECKED ) ;

      if ( _checkedSet.find( csUniqueID ) != _checkedSet.end() )
      {
         isChecked = TRUE ;
      }

      PD_TRACE_EXIT( SDB_CATDROPITEMSUBCLCHK__ISCHECKED ) ;

      return isChecked ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK__SAVECHECKED, "_catDropItemSubCLChecker::_saveChecked" )
   INT32 _catDropItemSubCLChecker::_saveChecked( utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK__SAVECHECKED ) ;

      try
      {
         _checkedSet.insert( csUniqueID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save checked collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXIT( SDB_CATDROPITEMSUBCLCHK__SAVECHECKED ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleProcessor implement
    */
   _catRecycleProcessor::_catRecycleProcessor( _catRecycleBinManager *recyBinMgr,
                                               utilRecycleItem &item,
                                               UTIL_RECYCLE_TYPE type )
   : _catRecycleBinProcessor( recyBinMgr, item ),
     _type( type )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _dmsCB = krcb->getDMSCB() ;
      _dpsCB = krcb->getDPSCB() ;
   }

   _catRecycleProcessor::~_catRecycleProcessor()
   {
   }

   const CHAR *_catRecycleProcessor::getCollection() const
   {
      return  catGetRecycleBinMetaCL( _type ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYPROCESS_PROCESSOBJ, "_catRecycleProcessor::processObject" )
   INT32 _catRecycleProcessor::processObject( const BSONObj &object,
                                              pmdEDUCB *cb,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYPROCESS_PROCESSOBJ ) ;

      BSONObj returnObject ;

      rc = _buildObject( object, cb, w, returnObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build return object, rc: %d", rc ) ;

      rc = _saveObject( returnObject, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save object, rc: %d", rc ) ;

      rc = _postSaveObject( object, returnObject, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process recycle object after save "
                   "to recycle collection, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYPROCESS_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYPROCESS__SAVEOBJ, "_catRecycleProcessor::_saveObject" )
   INT32 _catRecycleProcessor::_saveObject( const BSONObj &returnObject,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYPROCESS__SAVEOBJ ) ;

      const CHAR *recycleCollection = catGetRecycleBinRecyCL( _type ) ;

      rc = rtnInsert( recycleCollection, returnObject, 1, 0, cb, _dmsCB,
                      _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert object to collection [%s], "
                   "rc: %d", recycleCollection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYPROCESS__SAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYPROCESS__BLDOBJ, "_catRecycleProcessor::_buildObject" )
   INT32 _catRecycleProcessor::_buildObject( const bson::BSONObj &originObject,
                                             pmdEDUCB *cb,
                                             INT16 w,
                                             BSONObj &recycleObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYPROCESS__BLDOBJ ) ;

      try
      {
         BSONObjBuilder builder ;

         // initialize a new recycle OID
         OID recycleOID = OID::gen() ;
         builder.append( DMS_ID_KEY_NAME, recycleOID ) ;
         builder.append( FIELD_NAME_RECYCLE_ID,
                         (INT64)( _item.getRecycleID() ) ) ;

         BSONObjIterator iter( originObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            if ( 0 == ossStrcmp( DMS_ID_KEY_NAME, element.fieldName() ) )
            {
               continue ;
            }
            else
            {
               builder.append( element ) ;
            }
         }

         recycleObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build recycle object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYPROCESS__BLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleCSProcessor implement
    */
   _catRecycleCSProcessor::_catRecycleCSProcessor( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_CS )
   {
   }

   _catRecycleCSProcessor::~_catRecycleCSProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCSPROCESS_GETMATCHER, "_catRecycleCSProcessor::getMatcher" )
   INT32 _catRecycleCSProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCSPROCESS_GETMATCHER ) ;

      try
      {
         utilCSUniqueID csUniqueID = (utilCSUniqueID)( _item.getOriginID() ) ;
         BSONObj matcher = BSON( FIELD_NAME_UNIQUEID << (INT32)csUniqueID ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCSPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleCLProcessor implement
    */
   _catRecycleCLProcessor::_catRecycleCLProcessor( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_CL )
   {
   }

   _catRecycleCLProcessor::~_catRecycleCLProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCLPROCESS_GETMATCHER, "_catRecycleCLProcessor::getMatcher" )
   INT32 _catRecycleCLProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCLPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CS == _item.getType() )
         {
            utilCSUniqueID csUniqueID =
                              (utilCSUniqueID)( _item.getOriginID() ) ;
            rc = utilGetCSBounds( FIELD_NAME_UNIQUEID, csUniqueID, matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                         "space unique ID [%u], rc: %d", csUniqueID, rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            utilCLUniqueID clUniqueID =
                              (utilCLUniqueID)( _item.getOriginID() ) ;
            matcher = BSON( FIELD_NAME_UNIQUEID << (INT64)clUniqueID ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "collection objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCLPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCLPROCESS__POSTSAVEOBJ, "_catRecycleCLProcessor::_postSaveObject" )
   INT32 _catRecycleCLProcessor::_postSaveObject( const BSONObj &originObject,
                                                  BSONObj &recycleObject,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCLPROCESS__POSTSAVEOBJ ) ;

      const CHAR *collectionName = NULL ;

      if ( UTIL_RECYCLE_CL != _item.getType() )
      {
         goto done ;
      }

      // to recycle main-collection, recycle its sub-collections
      try
      {
         BSONElement element = originObject.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a string" ) ;
         collectionName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get collection name, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      {
         clsCatalogSet originSet( collectionName ) ;
         rc = originSet.updateCatSet( originObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for collection "
                      "[%s], rc: %d", _item.getOriginName(), rc ) ;

         // check if a main-collection
         if ( originSet.isMainCL() )
         {
            _item.setMainCL( TRUE ) ;

            // recycle sub-collections
            catRecycleSubCLProcessor subCLProcessor( _recyBinMgr, _item ) ;

            rc = _recyBinMgr->processObjects( subCLProcessor, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle sub-collections, "
                         "rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCLPROCESS__POSTSAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleSubCLProcessor implement
    */
   _catRecycleSubCLProcessor::_catRecycleSubCLProcessor( _catRecycleBinManager *recyBinMgr,
                                                         utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_CL )
   {
   }

   _catRecycleSubCLProcessor::~_catRecycleSubCLProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYSUBCLPROCESS_GETMATCHER, "_catRecycleSubCLProcessor::getMatcher" )
   INT32 _catRecycleSubCLProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYSUBCLPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            matcher = BSON( CAT_MAINCL_NAME << _item.getOriginName() ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "collection objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYSUBCLPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYSUBCLPROCESS__POSTSAVEOBJ, "_catRecycleSubCLProcessor::_postSaveObject" )
   INT32 _catRecycleSubCLProcessor::_postSaveObject( const BSONObj &originObject,
                                                     BSONObj &recycleObject,
                                                     pmdEDUCB *cb,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYSUBCLPROCESS__POSTSAVEOBJ ) ;

      const CHAR *collectionName = NULL ;

      if ( UTIL_RECYCLE_CL != _item.getType() )
      {
         goto done ;
      }

      try
      {
         BSONElement element = originObject.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a string" ) ;
         collectionName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get collection name, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      {
         clsCatalogSet originSet( collectionName ) ;
         rc = originSet.updateCatSet( originObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for collection "
                      "[%s], rc: %d", _item.getOriginName(), rc ) ;

         SDB_ASSERT( originSet.isSubCL(), "should be sub-collection" ) ;

         if ( originSet.isSubCL() )
         {
            utilRecycleItem subItem ;
            subItem.inherit( _item,
                             collectionName,
                             originSet.clUniqueID() ) ;

            catRecycleSeqProcessor seqProcessor( _recyBinMgr, subItem ) ;

            rc = _recyBinMgr->processObjects( seqProcessor, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle auto-increment "
                         "fields for ""sub-collections, rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYSUBCLPROCESS__POSTSAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleSeqProcessor implement
    */
   _catRecycleSeqProcessor::_catRecycleSeqProcessor( _catRecycleBinManager *recyBinMgr,
                                                     utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_SEQ )
   {
   }

   _catRecycleSeqProcessor::~_catRecycleSeqProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYSEQPROCESS_GETMATCHER, "_catRecycleSeqProcessor::getMatcher" )
   INT32 _catRecycleSeqProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYSEQPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CS == _item.getType() )
         {
            utilCSUniqueID csUniqueID =
                              (utilCSUniqueID)( _item.getOriginID() ) ;
            rc = utilGetCSBounds( FIELD_NAME_CL_UNIQUEID, csUniqueID, matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                         "space unique ID [%u], rc: %d", csUniqueID, rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            utilCLUniqueID clUniqueID =
                              (utilCLUniqueID)( _item.getOriginID() ) ;
            matcher = BSON( FIELD_NAME_CL_UNIQUEID << (INT64)clUniqueID ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "sequence objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYSEQPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleIdxProcessor implement
    */
   _catRecycleIdxProcessor::_catRecycleIdxProcessor( _catRecycleBinManager *recyBinMgr,
                                                     utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_IDX )
   {
   }

   _catRecycleIdxProcessor::~_catRecycleIdxProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYIDXPROCESS_GETMATCHER, "_catRecycleIdxProcessor::getMatcher" )
   INT32 _catRecycleIdxProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYIDXPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CS == _item.getType() )
         {
            utilCSUniqueID csUniqueID =
                              (utilCSUniqueID)( _item.getOriginID() ) ;
            rc = utilGetCSBounds( FIELD_NAME_CL_UNIQUEID, csUniqueID, matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                         "space unique ID [%u], rc: %d", csUniqueID, rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            utilCLUniqueID clUniqueID =
                              (utilCLUniqueID)( _item.getOriginID() ) ;
            matcher = BSON( FIELD_NAME_CL_UNIQUEID << (INT64)clUniqueID ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "index objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYIDXPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnCheckerBase implement
    */
   _catReturnCheckerBase::_catReturnCheckerBase( _catRecycleBinManager *recyBinMgr,
                                                 utilRecycleItem &item,
                                                 const catReturnConfig &conf,
                                                 catRecycleReturnInfo &info )
   : _catRecycleBinProcessor( recyBinMgr, item ),
     _conf( conf ),
     _info( info )
   {
   }

   _catReturnCheckerBase::_catReturnCheckerBase( _catReturnCheckerBase &checker )
   : _catRecycleBinProcessor( checker._recyBinMgr, checker._item ),
     _conf( checker._conf ),
     _info( checker._info )
   {
   }

   _catReturnCheckerBase::~_catReturnCheckerBase()
   {
   }

   /*
      _catReturnChecker implement
    */
   _catReturnChecker::_catReturnChecker( _catRecycleBinManager *recyBinMgr,
                                         utilRecycleItem &item,
                                         const catReturnConfig &conf,
                                         catRecycleReturnInfo &info,
                                         catCtxLockMgr &lockMgr,
                                         catCtxGroupHandler &groupHandler )
   : _catReturnCheckerBase( recyBinMgr, item, conf, info ),
     _lockMgr( lockMgr ),
     _groupHandler( groupHandler )
   {
   }

   _catReturnChecker::_catReturnChecker( _catReturnChecker &checker )
   : _catReturnCheckerBase( checker ),
     _lockMgr( checker._lockMgr ),
     _groupHandler( checker._groupHandler )
   {
   }

   _catReturnChecker::~_catReturnChecker()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCHK_GETMATCHER, "_catReturnChecker::getMatcher" )
   INT32 _catReturnChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCHK_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                         (INT64)( _item.getRecycleID() ) <<
                         _getOrigUIDField() <<
                         (INT64)( _item.getOriginID() ) ) ;

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnCSChecker implement
    */
   _catReturnCSChecker::_catReturnCSChecker( _catRecycleBinManager *recyBinMgr,
                                             utilRecycleItem &item,
                                             const catReturnConfig &conf,
                                             catRecycleReturnInfo &info,
                                             catCtxLockMgr &lockMgr,
                                             catCtxGroupHandler &groupHandler )
   : _catReturnChecker( recyBinMgr, item, conf, info, lockMgr, groupHandler )
   {
   }

   _catReturnCSChecker::~_catReturnCSChecker()
   {
   }

   const CHAR *_catReturnCSChecker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK_PROCESSOBJ, "_catReturnCSChecker::processObject" )
   INT32 _catReturnCSChecker::processObject( const bson::BSONObj &object,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK_PROCESSOBJ ) ;

      const CHAR *csName = NULL ;

      try
      {
         BSONElement element = object.getField( CAT_COLLECTION_SPACE_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   CAT_COLLECTION_SPACE_NAME ) ;
         csName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse collection object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _checkCS( csName, object, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return collection space [%s], "
                   "rc: %d", csName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _catReturnCSChecker::_checkCS( const CHAR *csName,
                                        const bson::BSONObj &object,
                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      utilReturnNameInfo nameInfo( csName ) ;
      BOOLEAN isConflict = FALSE ;

      // check if the same collection space is returning
      PD_CHECK( 0 == ossStrcmp( csName, _item.getOriginName() ),
                SDB_CAT_CORRUPTION, error, PDERROR,
                "Failed to check return collection space [%s], "
                "found a different collection space [%s]",
                _item.getOriginName(), csName ) ;

      // set return name if needed
      if ( _conf.isReturnToName() )
      {
         const CHAR *returnName = _conf.getReturnName() ;

         rc = _checkReturnCSToName( _item, returnName, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check return name [%s] for "
                      "collection space [%s], rc: %d",
                      returnName, csName, rc ) ;

         nameInfo.setReturnName( returnName, UTIL_RETURN_RENAMED_CS ) ;
      }

      // check if collection space exists
      rc = _checkConflictCSByName( _item, nameInfo, cb, isConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict name "
                   "collection [%s], rc: %d", csName, rc ) ;

      if ( isConflict && _conf.isReturnToName() )
      {
         // renamed collection space conflicts with exist collection
         PD_LOG_MSG_CHECK( FALSE, SDB_DMS_CS_EXIST, error, PDERROR,
                           "Failed to return collection space [%s] to [%s], "
                           "collection space [%s] already exists",
                           nameInfo.getOriginName(),
                           nameInfo.getReturnName(),
                           nameInfo.getReturnName() ) ;
      }

      // check domain
      rc = _checkDomain( object, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check domain to return "
                   "collection space [%s], rc: %d", csName, rc ) ;

      // check collections in returning collection space
      rc = _checkReturnCLInCS( _item, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check recycled collections in "
                   "collection space [%s], rc: %d", _item.getOriginName(),
                   rc ) ;

      // check groups
      rc = _checkGroups() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check groups to return "
                   "collection space [%s], rc: %d", csName, rc ) ;

      // lock collection space
      rc = _lockCollectionSpace( nameInfo, isConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space [%s], rc: %d",
                   csName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK__CHKRTRNCSTONAME, "_catReturnCSChecker::_checkReturnCSToName" )
   INT32 _catReturnCSChecker::_checkReturnCSToName( utilRecycleItem &recycleItem,
                                                    const CHAR *returnName,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK__CHKRTRNCSTONAME ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == recycleItem.getType(),
                  "should be recycle collection space" ) ;
      SDB_ASSERT( NULL != returnName, "return name is invalid" ) ;

      const CHAR *originName = recycleItem.getOriginName() ;

      // check return name
      rc = dmsCheckCSName( returnName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return name [%s] for "
                   "collection space, rc: %d", returnName, rc ) ;

      // add rename collection space info
      rc = _info.addRenameCS( originName, returnName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add rename collection space, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK__CHKRTRNCSTONAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK__CHKCONFLICTCSBYNAME, "_catReturnCSChecker::_checkConflictCSByName" )
   INT32 _catReturnCSChecker::_checkConflictCSByName(
                                             const utilRecycleItem &item,
                                             const utilReturnNameInfo &nameInfo,
                                             _pmdEDUCB *cb,
                                             BOOLEAN &isConflict )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK__CHKCONFLICTCSBYNAME ) ;

      const CHAR *rtrnCSName = nameInfo.getReturnName() ;
      BSONObj boSpace ;

      rc = catGetAndLockCollectionSpace( rtrnCSName, boSpace, cb ) ;
      if ( SDB_OK == rc )
      {
         // found conflict collection space against name
         PD_LOG( PDWARNING, "Found conflict to return dropped "
                 "collection space [%s], the collection space is recreated",
                 rtrnCSName ) ;
         rc = _info.addConflictCS( rtrnCSName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add conflict collection space "
                      "[%s], rc: %d", rtrnCSName, rc ) ;
         isConflict = TRUE ;
      }
      else if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // no conflict collection space
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check exists of "
                   "collection space [%s], rc: %d", rtrnCSName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK__CHKCONFLICTCSBYNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK__CHKRTRNCLINCS, "_catReturnCSChecker::_checkReturnCLInCS" )
   INT32 _catReturnCSChecker::_checkReturnCLInCS( utilRecycleItem &recycleItem,
                                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK__CHKRTRNCLINCS ) ;

      // check collections in collection space when return recycle item
      // for collection space
      // we need to exclude below objects
      // - sub-collections and main-collections are not in the same collection
      //   space ( which have been already detached when dropping collection
      //   space )
      // - sub-collections from data source which have been dropped
      //   we can not return it back if the data source is dropped

      catReturnCLInCSChecker checker( *this ) ;

      rc = _recyBinMgr->processObjects( checker, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run checker for collections in "
                   "collection space [%s], rc: %d",
                   recycleItem.getOriginName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK__CHKRTRNCLINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK__LOCKCS, "_catReturnCSChecker::_lockCollectionSpace" )
   INT32 _catReturnCSChecker::_lockCollectionSpace(
                                                const utilReturnNameInfo &nameInfo,
                                                BOOLEAN isConflict )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK__LOCKCS ) ;

      const CHAR *returnName = nameInfo.getReturnName() ;

      if ( !isConflict )
      {
         // lock the collection space name
         // NOTE: if conflict, will lock the space name after it it dropped
         PD_CHECK( _lockMgr.tryLockCollectionSpace( returnName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock origin collection space [%s]",
                   returnName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK__LOCKCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK__CHKDOMAIN, "_catReturnCSChecker::_checkDomain" )
   INT32 _catReturnCSChecker::_checkDomain( const BSONObj &object,
                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK__CHKDOMAIN ) ;

      utilCSUniqueID csUniqueID = (utilCSUniqueID)( _item.getOriginID() ) ;

      rc = _info.checkCSDomain( csUniqueID, object, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check domain for "
                   "collection space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK__CHKDOMAIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCSCHK__CHKGRPS, "_catReturnCSChecker::_checkGroups" )
   INT32 _catReturnCSChecker::_checkGroups()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCSCHK__CHKGRPS ) ;

      utilCSUniqueID csUniqueID = (utilCSUniqueID)( _item.getOriginID() ) ;
      const CAT_GROUP_SET &groupIDSet = _groupHandler.getGroupIDSet() ;

      // domain should be checked
      PD_CHECK( _info.isCSDomainChecked( csUniqueID ),
                SDB_SYS, error, PDERROR,
                "Failed to check groups, the collection space [%u] is not "
                "checked with domain", csUniqueID ) ;

      // check if groups occupied are in the domain
      for ( CAT_GROUP_SET::iterator iter = groupIDSet.begin() ;
            iter != groupIDSet.end() ;
            ++ iter )
      {
         UINT32 groupID = *iter ;
         rc = _info.checkDomainGroup( csUniqueID, groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check group [%u] to return "
                      "collection space [%u], rc: %d", groupID, csUniqueID,
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCSCHK__CHKGRPS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnCLChecker implement
    */
   _catReturnCLChecker::_catReturnCLChecker( _catRecycleBinManager *recyBinMgr,
                                             utilRecycleItem &item,
                                             const catReturnConfig &conf,
                                             catRecycleReturnInfo &info,
                                             catCtxLockMgr &lockMgr,
                                             catCtxGroupHandler &groupHandler )
   : _catReturnChecker( recyBinMgr, item, conf, info, lockMgr, groupHandler )
   {
   }

   _catReturnCLChecker::_catReturnCLChecker( _catReturnChecker &checker )
   : _catReturnChecker( checker )
   {
   }

   _catReturnCLChecker::~_catReturnCLChecker()
   {
   }

   const CHAR *_catReturnCLChecker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK_PROCESSOBJ, "_catReturnCLChecker::processObject" )
   INT32 _catReturnCLChecker::processObject( const BSONObj &object,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK_PROCESSOBJ ) ;

      const CHAR *clName = NULL ;

      try
      {
         BSONElement element = object.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   CAT_COLLECTION_NAME ) ;
         clName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse collection object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      {
         clsCatalogSet catSet( clName ) ;

         rc = catSet.updateCatSet( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update catalog set [%s], "
                      "rc: %d", clName, rc ) ;

         rc = _checkCL( clName, object, catSet, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check return collection [%s], "
                      "rc: %d", clName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKCL, "_catReturnCLChecker::_checkCL" )
   INT32 _catReturnCLChecker::_checkCL( const CHAR *clName,
                                        const BSONObj &object,
                                        clsCatalogSet &catSet,
                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKCL ) ;

      const CHAR *origCLName = _item.getOriginName() ;
      utilCLUniqueID origCLUID = (utilCLUniqueID)( _item.getOriginID() ) ;

      utilReturnNameInfo nameInfo( origCLName ) ;
      BOOLEAN isConflict = FALSE ;

      rc = _info.checkReturnCLToCS( origCLName,
                                    origCLUID,
                                    _conf.isAllowRename(),
                                    cb ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get collection space for "
                     "collection [%s], rc: %d", origCLName, rc ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check return collection [%s] to "
                   "collection space, rc: %d", origCLName, rc ) ;

      if ( _conf.isReturnToName() )
      {
         const CHAR *returnName = _conf.getReturnName() ;

         rc = _checkReturnCLToName( _item, returnName, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check return name [%s] for "
                      "collection [%s], rc: %d", _conf.getReturnName(),
                      origCLName, rc ) ;

         nameInfo.setReturnName( returnName, UTIL_RETURN_RENAMED_CL ) ;
      }
      else
      {
         rc = _info.getReturnCLName( nameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                      "collection [%s], rc: %d", origCLName, rc ) ;
      }

      // just check the existence of collection, no lock is needed
      rc = _checkConflictCLByName( _item, nameInfo, catSet, cb,
                                   isConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict name "
                   "collection [%s], rc: %d", origCLName, rc ) ;

      if ( isConflict && _conf.isReturnToName() )
      {
         // renamed collection conflicts with exist collection
         PD_LOG_MSG_CHECK( FALSE, SDB_DMS_EXIST, error, PDERROR,
                           "Failed to return collection [%s] to [%s], "
                           "collection [%s] already exists",
                           nameInfo.getOriginName(),
                           nameInfo.getReturnName(),
                           nameInfo.getReturnName() ) ;
      }

      rc = _checkConflictCLByUID( _item, nameInfo, catSet, cb, isConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict unique ID "
                   "collection [%s], rc: %d", origCLName, rc ) ;

      if ( _item.isMainCL() )
      {
         CLS_SUBCL_LIST subCLList ;

         PD_CHECK( catSet.isMainCL(), SDB_SYS, error, PDERROR,
                   "Failed to check sub recycle items of main "
                   "recycle item [%s], recycle collection [%s] is not "
                   "main-collection", _item.getRecycleName(),
                   _item.getOriginName() ) ;

         rc = _checkReturnMainCL( catSet, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check return main-collection "
                      "[%s], rc: %d", clName, rc ) ;
      }
      else
      {
         rc = _checkDomain( catSet, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check domain of "
                      "collection [%s], rc: %d", origCLName, rc ) ;
         rc = _groupHandler.addGroups( *( catSet.getAllGroupID() ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save groups for "
                      "sub-collection [%s], rc: %d", origCLName, rc ) ;
      }

      rc = _lockCollection( nameInfo, isConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                   origCLName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKCDOMAIN, "_catReturnCLChecker::_checkDomain" )
   INT32 _catReturnCLChecker::_checkDomain( clsCatalogSet &catSet,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKCDOMAIN ) ;

      SDB_ASSERT( NULL != catSet.getAllGroupID(), "groups should be valid" ) ;

      utilCSUniqueID csUniqueID = utilGetCSUniqueID( catSet.clUniqueID() ) ;

      if ( !_info.isCSDomainChecked( csUniqueID ) )
      {
         BSONObj boSpace ;
         const CHAR *csName = NULL ;
         rc = catGetAndLockCollectionSpace( csUniqueID, boSpace, csName, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection space [%u], "
                      "rc: %d", rc ) ;

         rc = _info.checkCSDomain( csUniqueID, boSpace, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check domain for "
                      "collection space [%u], rc: %d", csUniqueID, rc ) ;
      }

      for ( VEC_GROUP_ID::iterator iter = catSet.getAllGroupID()->begin() ;
            iter != catSet.getAllGroupID()->end() ;
            ++ iter )
      {
         UINT32 groupID = *iter ;
         rc = _info.checkDomainGroup( csUniqueID, groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check group [%u] to return "
                      "collection [%s], rc: %d", groupID, catSet.name(),
                      rc ) ;
      }


   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKCDOMAIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKCONFLICTCLBYNAME, "_catReturnCLChecker::_checkConflictCLByName" )
   INT32 _catReturnCLChecker::_checkConflictCLByName(
                                             const utilRecycleItem &item,
                                             const utilReturnNameInfo &nameInfo,
                                             const clsCatalogSet &recycleSet,
                                             _pmdEDUCB *cb,
                                             BOOLEAN &isConflict )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKCONFLICTCLBYNAME ) ;

      const CHAR *originName = nameInfo.getOriginName() ;
      const CHAR *returnName = nameInfo.getReturnName() ;
      BSONObj boCollection ;

      clsCatalogSet currentSet( returnName ) ;

      // just check the existence of collection, no lock is needed
      rc = catGetCollection( returnName, boCollection, cb ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         // safe to return with origin name
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check exists of collection [%s], "
                   "rc: %d", returnName, rc ) ;

      rc = currentSet.updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse collection object [%s], "
                   "rc: %d", returnName, rc ) ;

      if ( currentSet.clUniqueID() == (utilCLUniqueID)( item.getOriginID() ) &&
           _info.isConflictUIDCL( currentSet.clUniqueID() ) &&
           _conf.isEnforced() )
      {
         // already mark conflict, and enforce mode will drop eventually,
         // don't care
         goto done ;
      }

      if ( _conf.isReturnToName() )
      {
         // for return to name, always conflict for both drop and truncate
         PD_LOG( PDWARNING, "Found conflict to return collection [%s] "
                 "to [%s], the collection [%s] already exists",
                 originName, returnName, returnName ) ;
         isConflict = TRUE ;
      }
      else if ( UTIL_RECYCLE_OP_DROP == item.getOpType() )
      {
         // for drop collection, just mark it conflicts
         PD_LOG( PDWARNING, "Found conflict to return dropped "
                 "collection [%s], the collection is recreated",
                 returnName ) ;
         isConflict = TRUE ;
      }
      else if ( _conf.isEnforced() )
      {
         // for truncate, we need to check if collection is empty in data
         // node later, but if it is enforced, we can skip the check, just
         // drop it directly
         PD_LOG( PDDEBUG, "Directly drop truncated collection [%s] "
                 "in enforced mode", returnName ) ;
         isConflict = TRUE ;
      }
      else
      {
         // for truncated collection, we can replace the collection if
         // nothing changed
         INT64 splitTaskNum = 0 ;
         rc = catGetCLTaskCountByType( returnName, CLS_TASK_SPLIT, cb,
                                       splitTaskNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get split task number for "
                      "collection [%s]", returnName ) ;

         if ( splitTaskNum > 0 )
         {
            // it is under split, version will be changed later
            PD_LOG( PDWARNING, "Found conflict to return truncated "
                    "collection [%s], it is under split", returnName ) ;
            isConflict = TRUE ;
         }
         else if ( currentSet.clUniqueID() != recycleSet.clUniqueID() )
         {
            // unique IDs are different
            PD_LOG( PDWARNING, "Found conflict to return truncated "
                    "collection [%s], current %llu, recycle %llu",
                    returnName, currentSet.clUniqueID(),
                    recycleSet.clUniqueID() ) ;
            isConflict = TRUE ;
         }
         else if ( currentSet.getVersion() != recycleSet.getVersion() )
         {
            // version are different
            PD_LOG( PDWARNING, "Found conflict to return truncated "
                    "collection [%s], current %d, recycle %d",
                    returnName, currentSet.getVersion(),
                    recycleSet.getVersion() ) ;
            isConflict = TRUE ;
         }
      }

      if ( isConflict )
      {
         rc = _info.addConflictNameCL( recycleSet.clUniqueID(), nameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add conflict collection [%s], "
                      "rc: %d", returnName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKCONFLICTCLBYNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKCONFLICTCLBYUID, "_catReturnCLChecker::_checkConflictCLByUID" )
   INT32 _catReturnCLChecker::_checkConflictCLByUID(
                                             const utilRecycleItem &item,
                                             const utilReturnNameInfo &nameInfo,
                                             const clsCatalogSet &recycleSet,
                                             _pmdEDUCB *cb,
                                             BOOLEAN &isConflict )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKCONFLICTCLBYUID ) ;

      const CHAR *originName = nameInfo.getOriginName() ;
      const CHAR *returnName = nameInfo.getReturnName() ;
      utilCLUniqueID originID = recycleSet.clUniqueID() ;
      string conflictName ;
      BSONObj boCollection ;

      // just check the existence of collection, no lock is needed
      rc = catGetCollectionNameByUID( originID, conflictName, boCollection,
                                      cb ) ;
      if ( SDB_DMS_EOC == rc )
      {
         // safe to return with origin name
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check exists of collection "
                   "unique ID [%llu], rc: %d", originID, rc ) ;

      if ( 0 == ossStrcmp( conflictName.c_str(), returnName ) &&
           _info.isConflictNameCL( returnName ) &&
           _conf.isEnforced() )
      {
         // already mark conflict, and enforce mode will drop eventually,
         // don't care
         goto done ;
      }

      if ( _conf.isReturnToName() )
      {
         // for return to name, always conflict for both drop and truncate
         PD_LOG( PDWARNING, "Found conflict to return collection [%s] "
                 "to [%s], unique ID [%llu], the collection is "
                 "renamed to [%s]", originName, returnName, originID,
                 conflictName.c_str() ) ;
         isConflict = TRUE ;
      }
      else if ( UTIL_RECYCLE_OP_DROP == item.getOpType() )
      {
         // for drop collection, just mark it conflicts
         PD_LOG( PDWARNING, "Found conflict to return dropped "
                 "collection [%s], unique ID [%llu], the collection is "
                 "renamed to [%s]", returnName, originID,
                 conflictName.c_str() ) ;
         isConflict = TRUE ;
      }
      else if ( _conf.isEnforced() )
      {
         // for truncate, we need to check if collection is empty in data
         // node later, but if it is enforced, we can skip the check, just
         // drop it directly
         PD_LOG( PDDEBUG, "Directly drop truncated collection [%s] "
                 "in enforced mode", returnName ) ;
         isConflict = TRUE ;
      }
      else
      {
         clsCatalogSet conflictSet( conflictName.c_str() ) ;

         rc = conflictSet.updateCatSet( boCollection ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse collection object [%s], "
                      "rc: %d", conflictName.c_str(), rc ) ;

         // for truncate collection, we can replace the collection if
         // nothing changed
         if ( 0 != ossStrcmp( conflictName.c_str(), returnName ) )
         {
            PD_LOG( PDWARNING, "Found conflict to return truncated "
                    "collection [%s], the collection is "
                    "renamed to [%s]", returnName, conflictName.c_str() ) ;
            isConflict = TRUE ;
         }
         else if ( conflictSet.getVersion() != recycleSet.getVersion() )
         {
            PD_LOG( PDWARNING, "Found conflict to return truncated "
                    "collection [%s], current %d, recycle %d",
                    returnName, conflictSet.getVersion(),
                    recycleSet.getVersion() ) ;
            isConflict = TRUE ;
         }
      }

      if ( isConflict )
      {
         rc = _info.addConflictUIDCL( originID,
                                      originName,
                                      conflictName.c_str() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add conflict UID "
                      "collection [%s], rc: %d", returnName, rc ) ;

         rc = _checkConflictSeqByUID( recycleSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check conflict sequences of "
                      "collection [%s], rc: %d", returnName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKCONFLICTCLBYUID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKCONFLICTSEQBYUID, "_catReturnCLChecker::_checkConflictSeqByUID" )
   INT32 _catReturnCLChecker::_checkConflictSeqByUID( const clsCatalogSet &recycleSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKCONFLICTSEQBYUID ) ;

      utilCLUniqueID originID = recycleSet.clUniqueID() ;

      // need to save conflict sequences
      if ( recycleSet.getAutoIncSet()->itemCount() > 0 )
      {
         const clsAutoIncSet *autoIncSet = recycleSet.getAutoIncSet() ;
         SDB_ASSERT( NULL != autoIncSet, "auto-increment set is invalid" ) ;
         clsAutoIncIterator it( *autoIncSet, clsAutoIncIterator::RECURS ) ;
         while( it.more() )
         {
            const clsAutoIncItem* autoIncItem = it.next() ;
            rc = _info.addConflictSeq( originID,
                                       autoIncItem->sequenceID(),
                                       autoIncItem->sequenceName(),
                                       autoIncItem->fieldFullName() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add conflict sequence, "
                         "rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKCONFLICTSEQBYUID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKRTRNCLTONAME, "_catReturnCLChecker::_checkReturnCLToName" )
   INT32 _catReturnCLChecker::_checkReturnCLToName( utilRecycleItem &item,
                                                    const CHAR *returnName,
                                                    _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKRTRNCLTONAME ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType(),
                  "should be recycle collection" ) ;
      SDB_ASSERT( NULL != returnName, "return name is invalid" ) ;

      const CHAR *originName = item.getOriginName() ;
      const CHAR *recycleName = item.getRecycleName() ;
      CHAR origCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { '\0' } ;
      CHAR rtrnCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { '\0' } ;

      utilReturnNameInfo csNameInfo ;

      rc = dmsCheckFullCLName( returnName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return name [%s] for "
                   "collection, rc: %d", returnName, rc ) ;

      rc = rtnResolveCollectionSpaceName( originName,
                                          ossStrlen( originName ),
                                          origCSName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection space name "
                         "from collection name [%s], rc: %d", returnName, rc ) ;

      rc = rtnResolveCollectionSpaceName( returnName, ossStrlen( returnName ),
                                          rtrnCSName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection space name "
                   "from collection name [%s], rc: %d", returnName, rc ) ;

      csNameInfo.init( origCSName ) ;

      // check if the collection space is renamed
      rc = _info.getReturnCSName( csNameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name of collection "
                   "space [%s], rc: %d", origCSName, rc ) ;

      // check whether the returning collection will be returned to the
      // collection space with the same unique ID which may be renamed
      if ( csNameInfo.isRenamed() )
      {
         PD_LOG_MSG_CHECK( 0 == ossStrncmp( csNameInfo.getReturnName(),
                                            rtrnCSName,
                                            DMS_COLLECTION_SPACE_NAME_SZ ),
                           SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                           "Failed to check return recycle bin item "
                           "[origin %s, recycle %s], return name [%s] is not "
                           "in the same collection space with origin collection "
                           "space [%s], which has been renamed from [%s]",
                           originName, recycleName, returnName,
                           csNameInfo.getReturnName(), origCSName ) ;
      }
      else
      {
         PD_LOG_MSG_CHECK( 0 == ossStrncmp( csNameInfo.getReturnName(),
                                            rtrnCSName,
                                            DMS_COLLECTION_SPACE_NAME_SZ ),
                           SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                           "Failed to check return recycle bin item "
                           "[origin %s, recycle %s], return name [%s] is not "
                           "in the same collection space with origin collection "
                           "space [%s]", originName, recycleName, returnName,
                           csNameInfo.getReturnName() ) ;
      }

      rc = _info.addRenameCL( originName, returnName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add rename collection, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKRTRNCLTONAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__CHKRTRNMAINCL, "_catReturnCLChecker::_checkReturnMainCL" )
   INT32 _catReturnCLChecker::_checkReturnMainCL( const clsCatalogSet &mainCLSet,
                                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__CHKRTRNMAINCL ) ;

      catReturnSubCLChecker checker( *this,
                                     (UINT32)( mainCLSet.getSubCLCount() ) ) ;

      rc = _recyBinMgr->processObjects( checker, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run checker for sub-collections in "
                   "main-collection [%s], rc: %d", mainCLSet.name(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__CHKRTRNMAINCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLCHK__LOCKCL, "_catReturnCLChecker::_lockCollection" )
   INT32 _catReturnCLChecker::_lockCollection(
                                             const utilReturnNameInfo &nameInfo,
                                             BOOLEAN isConflict )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLCHK__LOCKCL ) ;

      const CHAR *returnName = nameInfo.getReturnName() ;

      if ( !isConflict )
      {
         // NOTE: if conflict, will lock the space name after it it dropped
         PD_CHECK( _lockMgr.tryLockCollection( returnName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock collection [%s]", returnName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLCHK__LOCKCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnCLInCSChecker implement
    */
   _catReturnCLInCSChecker::_catReturnCLInCSChecker( catReturnCSChecker &checker )
   : _catReturnCLChecker( checker )
   {
   }

   _catReturnCLInCSChecker::~_catReturnCLInCSChecker()
   {
   }

   const CHAR *_catReturnCLInCSChecker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLINCSCHK_GETMATCHER, "_catReturnCLInCSChecker::getMatcher" )
   INT32 _catReturnCLInCSChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLINCSCHK_GETMATCHER ) ;

      BSONObj matcher ;

      // we need to check collections both normal and recycled
      // so we use the unique ID of collection space instead of
      // recycle ID to do the match
      rc = utilGetCSBounds( FIELD_NAME_UNIQUEID,
                            (utilCSUniqueID)( _item.getOriginID() ),
                            matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build matcher for check "
                   "collections in return collection space [%llu], "
                   "rc: %d", _item.getOriginID(), rc ) ;

      try
      {
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return checker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLINCSCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCLINCSCHK__CHKCL, "_catReturnCLInCSChecker::_checkCL" )
   INT32 _catReturnCLInCSChecker::_checkCL( const CHAR *clName,
                                            const BSONObj &object,
                                            clsCatalogSet &catSet,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCLINCSCHK__CHKCL ) ;

      BOOLEAN isRecycled = FALSE ;

      try
      {
         BSONElement ele = object.getField( FIELD_NAME_RECYCLE_ID ) ;
         PD_CHECK( ele.isNumber(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number",
                   FIELD_NAME_RECYCLE_ID ) ;

         // from different recycle ID, it is recycled before dropping
         // this collection space
         if ( (utilRecycleID)( ele.numberLong() ) != _item.getRecycleID() )
         {
            isRecycled = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get recycle ID, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      // check if the collection had been recycled before dropping this
      // collection space
      if ( isRecycled )
      {
         // we need to collect group IDs, where the collection space were
         // located before, even if all collections are recycled from those
         // groups
         if ( !catSet.isMainCL() )
         {
            // save group ID
            rc = _groupHandler.addGroups( *( catSet.getAllGroupID() ) ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save groups for "
                         "collection [%s], rc: %d", clName, rc ) ;
         }
         goto done ;
      }

      // check if the collection is from data source
      if ( UTIL_INVALID_DS_UID != catSet.getDataSourceID() )
      {
         UTIL_DS_UID dsUID = catSet.getDataSourceID() ;
         BOOLEAN isExist = FALSE ;

         rc = _info.checkDSExist( dsUID, isExist, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check existence of "
                      "data source [%llu], rc: %d", dsUID, rc ) ;

         if ( !isExist )
         {
            PD_LOG( PDDEBUG, "Data source [%llu] for collection [%s] "
                    "is missing", dsUID, clName ) ;

            // data source had been dropped, and it is attached to a
            // main-colleciton, ignore this collection and report it missing
            if ( catSet.isSubCL() )
            {
               const CHAR *mainCLName = catSet.getMainCLName().c_str() ;
               rc = _info.addMissingSubCL( clName, mainCLName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add missing "
                            "sub-collection [%s] from main-collection [%s], "
                            "rc: %d", clName, mainCLName, rc ) ;
            }
            goto done ;
         }
      }

      if ( catSet.isMainCL() )
      {
         UINT32 clNameLen = ossStrlen( clName ) ;
         CLS_SUBCL_LIST subCLList ;

         rc = catSet.getSubCLList( subCLList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collections "
                      "from main-collection [%s], rc: %d",
                      clName, rc ) ;

         for ( CLS_SUBCL_LIST_IT iter = subCLList.begin() ;
               iter != subCLList.end() ;
               ++ iter )
         {
            BOOLEAN inSameSpace = FALSE ;

            const CHAR *subCLName = iter->c_str() ;
            UINT32 subCLNameLen = iter->size() ;

            rc = rtnCollectionsInSameSpace( clName, clNameLen,
                                            subCLName, subCLNameLen,
                                            inSameSpace ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check collection "
                         "space for collections [%s] and [%s], "
                         "rc: %d", clName, subCLName, rc ) ;

            if ( !inSameSpace )
            {
               rc = _info.addMissingSubCL( subCLName, clName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add conflict "
                            "sub-collection [%s] from main-collection "
                            "[%s], rc: %d", subCLName, clName, rc ) ;
            }
         }
      }
      else
      {
         if ( catSet.isSubCL() )
         {
            BOOLEAN inSameSpace = FALSE ;

            const CHAR *mainCLName = catSet.getMainCLName().c_str() ;
            UINT32 mainCLNameLen = catSet.getMainCLName().size() ;

            rc = rtnCollectionsInSameSpace( clName, ossStrlen( clName ),
                                            mainCLName, mainCLNameLen,
                                            inSameSpace ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check collection "
                         "space for collections [%s] and [%s], "
                         "rc: %d", clName, mainCLName, rc ) ;

            if ( !inSameSpace )
            {
               rc = _info.addMissingMainCL( mainCLName, clName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add conflict "
                            "main-collection [%s] for sub-collection "
                            "[%s], rc: %d", mainCLName, clName, rc ) ;
            }
         }

         // save group ID
         rc = _groupHandler.addGroups( *( catSet.getAllGroupID() ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save groups for "
                      "sub-collection [%s], rc: %d", clName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNCLINCSCHK__CHKCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnSubCLChecker implement
    */
   _catReturnSubCLChecker::_catReturnSubCLChecker( catReturnCLChecker &mainCLChecker,
                                                   UINT32 subCLCount )
   : _catReturnCLChecker( mainCLChecker ),
     _subCLCount( subCLCount )
   {
   }

   _catReturnSubCLChecker::~_catReturnSubCLChecker()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNSUBCLCHK_GETMATCHER, "_catReturnSubCLChecker::getMatcher" )
   INT32 _catReturnSubCLChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNSUBCLCHK_GETMATCHER ) ;

      try
      {
         BSONObj matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                                 (INT64)( _item.getRecycleID() ) <<
                                 FIELD_NAME_MAINCLNAME <<
                                 _item.getOriginName() ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return checker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNSUBCLCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNSUBCLCHK__CHKCL, "_catReturnSubCLChecker::_checkCL" )
   INT32 _catReturnSubCLChecker::_checkCL( const CHAR *clName,
                                           const BSONObj &object,
                                           clsCatalogSet &catSet,
                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNSUBCLCHK__CHKCL ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == _item.getType(),
                  "recycle type is invalid" ) ;
      SDB_ASSERT( _item.isMainCL(), "should be main recycle item" ) ;

      const CHAR *origMainCLName = _item.getOriginName() ;
      BSONObj boSubCL ;
      utilReturnNameInfo subCLNameInfo( clName ) ;
      BOOLEAN isSubCLConflict = FALSE ;

      if ( UTIL_INVALID_DS_UID != catSet.getDataSourceID() )
      {
         UTIL_DS_UID dsUID = catSet.getDataSourceID() ;
         BOOLEAN isExist = FALSE ;

         rc = _info.checkDSExist( dsUID, isExist, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check existence of "
                      "data source [%llu], rc: %d", dsUID, rc ) ;

         if ( !isExist )
         {
            PD_LOG( PDDEBUG, "Data source [%llu] for collection [%s] "
                    "is missing", dsUID, clName ) ;
            // not support for dropped data source
            rc = _info.addMissingSubCL( clName, origMainCLName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add missing "
                         "sub-collection [%s], rc: %d", clName, rc ) ;

            goto done ;
         }
      }

      rc = _info.checkReturnCLToCS( clName,
                                    catSet.clUniqueID(),
                                    _conf.isAllowRename(),
                                    cb ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         if ( _conf.isEnforced() )
         {
            rc = _info.addMissingSubCL( clName, origMainCLName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add missing "
                         "sub-collection [%s], rc: %d", clName, rc ) ;

            goto done ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "Failed to get collection space for "
                        "sub-collection [%s]", clName ) ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check return collection "
                   "[%s] to collection space, rc: %d", clName, rc ) ;

      rc = _info.getReturnCLName( subCLNameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                   "collection [%s], rc: %d", clName, rc ) ;

      rc = _checkConflictCLByName( _item, subCLNameInfo, catSet, cb,
                                   isSubCLConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict name collection "
                   "[%s], rc: %d", clName, rc ) ;

      rc = _checkConflictCLByUID( _item, subCLNameInfo, catSet, cb,
                                  isSubCLConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict unique ID "
                   "collection [%s], rc: %d", clName, rc ) ;

      if ( UTIL_INVALID_DS_UID == catSet.getDataSourceID() )
      {
         rc = _checkDomain( catSet, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check domain of "
                      "collection [%s], rc: %d", clName, rc ) ;
      }

      rc = _groupHandler.addGroups( *( catSet.getAllGroupID() ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save groups for "
                   "sub-collection [%s], rc: %d", clName, rc ) ;

      rc = _lockCollection( subCLNameInfo, isSubCLConflict ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                   clName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNSUBCLCHK__CHKCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnIdxChecker implement
    */
   _catReturnIdxChecker::_catReturnIdxChecker( _catReturnChecker &checker,
                                               UINT16 rebuildTypes )
   : _catReturnCheckerBase( checker ),
     _rebuildTypes( rebuildTypes )
   {
   }

   _catReturnIdxChecker::~_catReturnIdxChecker()
   {
   }

   const CHAR *_catReturnIdxChecker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_IDX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNIDXCHK_GETMATCHER, "_catReturnIdxChecker::getMatcher" )
   INT32 _catReturnIdxChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNIDXCHK_GETMATCHER ) ;

      try
      {
         BSONObj matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                                 (INT64)( _item.getRecycleID() ) ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return checker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNIDXCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNIDXCHK_PROCESSOBJ, "_catReturnIdxChecker::processObject" )
   INT32 _catReturnIdxChecker::processObject( const BSONObj &object,
                                              pmdEDUCB *cb,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNIDXCHK_PROCESSOBJ ) ;

      const CHAR *clName = NULL ;
      BSONObj indexOptions ;
      UINT16 indexType = IXM_EXTENT_TYPE_NONE ;

      BOOLEAN needRebuild = FALSE ;

      try
      {
         BSONElement element = object.getField( FIELD_NAME_COLLECTION ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   FIELD_NAME_COLLECTION ) ;
         clName = element.valuestr() ;

         element = object.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not object",
                   IXM_FIELD_NAME_INDEX_DEF ) ;
         indexOptions = element.embeddedObject() ;

         rc = ixmGetIndexType( indexOptions, indexType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index type from [%s], "
                      "rc: %d", indexOptions.toPoolString().c_str(), rc ) ;

         if ( OSS_BIT_TEST( indexType, _rebuildTypes ) )
         {
            // index need rebuild
            needRebuild = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse index object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( needRebuild )
      {
         rc = _info.addRebuildIndex( clName, indexOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add rebuild index, rc: %d",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRTRNIDXCHK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnProcessor implement
    */
   _catReturnProcessor::_catReturnProcessor( _catRecycleBinManager *recyBinMgr,
                                             utilRecycleItem &item,
                                             catRecycleReturnInfo &info,
                                             UTIL_RECYCLE_TYPE type )
   : _catRecycleBinProcessor( recyBinMgr, item ),
     _info( info ),
     _type( type )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _dmsCB = krcb->getDMSCB() ;
      _dpsCB = krcb->getDPSCB() ;
   }

   _catReturnProcessor::~_catReturnProcessor()
   {
   }

   const CHAR *_catReturnProcessor::getCollection() const
   {
      return catGetRecycleBinRecyCL( _type ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNPROCESS_GETMATCHER, "_catReturnProcessor::getMatcher" )
   INT32 _catReturnProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                                 (INT64)( _item.getRecycleID() ) ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return processor, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNPROCESS_PROCESSOBJ, "_catReturnProcessor::processObject" )
   INT32 _catReturnProcessor::processObject( const BSONObj &object,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNPROCESS_PROCESSOBJ ) ;

      BSONObj returnObject ;
      BOOLEAN needProcess = TRUE ;

      rc = _buildObject( object, cb, w, returnObject, needProcess ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build return object, rc: %d", rc ) ;

      if ( needProcess )
      {
         rc = _saveObject( returnObject, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save object, rc: %d", rc ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Ignore object [%s]",
                 object.toPoolString().c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNPROCESS_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNPROCESS__SAVEOBJ, "_catReturnProcessor::_saveObject" )
   INT32 _catReturnProcessor::_saveObject( const BSONObj &returnObject,
                                           pmdEDUCB *cb,
                                           INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNPROCESS__SAVEOBJ ) ;

      const CHAR *returnCollection = catGetRecycleBinMetaCL( _type ) ;
      INT32 flags = _info.isOnSiteReturn() ? FLG_INSERT_REPLACEONDUP : 0 ;
      rc = rtnInsert( returnCollection, returnObject, 1, flags, cb, _dmsCB,
                      _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert object to collection [%s], "
                   "rc: %d", returnCollection, rc ) ;

      rc = _postSaveObject( returnObject, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run post save object, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNPROCESS__SAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnCSProcessor implement
    */
   _catReturnCSProcessor::_catReturnCSProcessor( _catRecycleBinManager *recyBinMgr,
                                                 utilRecycleItem &item,
                                                 catRecycleReturnInfo &info )
   : _catReturnProcessor( recyBinMgr, item, info, UTIL_RECYCLE_CS )
   {
   }

   _catReturnCSProcessor::~_catReturnCSProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCSPROCESS__BLDOBJ, "_catReturnCSProcessor::_buildObject" )
   INT32 _catReturnCSProcessor::_buildObject( const BSONObj &recycleObject,
                                              pmdEDUCB *cb,
                                              INT16 w,
                                              BSONObj &returnObject,
                                              BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCSPROCESS__BLDOBJ ) ;

      needProcess = TRUE ;

      try
      {
         BSONObjBuilder builder ;
         OID returnOID = OID::gen() ;

         builder.append( DMS_ID_KEY_NAME, returnOID ) ;

         BSONObjIterator iter( recycleObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            const CHAR *fieldName = element.fieldName() ;
            if ( 0 == ossStrcmp( DMS_ID_KEY_NAME, fieldName ) ||
                 0 == ossStrcmp( FIELD_NAME_RECYCLE_ID, fieldName ) )
            {
               // append nothing
            }
            else if ( 0 == ossStrcmp( CAT_COLLECTION_SPACE_NAME, fieldName ) )
            {
               utilReturnNameInfo nameInfo ;
               const CHAR *origCSName = NULL ;

               PD_CHECK( String == element.type(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get field [%s] from recycle "
                         "collection space object, it is not string",
                         CAT_COLLECTION_SPACE_NAME ) ;
               origCSName = element.valuestr() ;
               nameInfo.init( origCSName ) ;

               rc = _info.getReturnCSName( nameInfo ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                            "collection space [%s], rc: %d", origCSName,
                            rc ) ;

               builder.append( CAT_COLLECTION_SPACE_NAME,
                               nameInfo.getReturnName() ) ;
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_DATASOURCE_ID, fieldName ) )
            {
               // ignore data source object
               needProcess = FALSE ;
               break ;
            }
            else if ( 0 == ossStrcmp( CAT_DOMAIN_NAME, fieldName ) )
            {
               const CHAR *domainName = NULL ;
               PD_CHECK( String == element.type(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get field [%s], it is not a string",
                         CAT_DOMAIN_NAME ) ;
               domainName = element.valuestr() ;

               // check is domain does still exist, if still exists, return
               // collection space with the domain name, otherwise, ignore it
               if ( !_info.isMissingDomain( domainName ) )
               {
                  builder.append( element ) ;
               }
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_UPDATE_TIME, fieldName ) )
            {
               // refresh update time
               UINT64 currentTime = ossGetCurrentMilliseconds() ;
               CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
               ossMillisecondsToString( currentTime, timestamp ) ;
               builder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;
            }
            else
            {
               builder.append( element ) ;
            }
         }

         returnObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build return object for collection "
                 "space, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCSPROCESS__BLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
         _catReturnCLProcessor implement
    */
   _catReturnCLProcessor::_catReturnCLProcessor( _catRecycleBinManager *recyBinMgr,
                                                 utilRecycleItem &item,
                                                 catRecycleReturnInfo &info )
   : _catReturnProcessor( recyBinMgr, item, info, UTIL_RECYCLE_CL )
   {
   }

   _catReturnCLProcessor::~_catReturnCLProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCLPROCESS__BLDOBJ, "_catReturnCLProcessor::_buildObject" )
   INT32 _catReturnCLProcessor::_buildObject( const BSONObj &recycleObject,
                                              pmdEDUCB *cb,
                                              INT16 w,
                                              BSONObj &returnObject,
                                              BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCLPROCESS__BLDOBJ ) ;

      const CHAR *originName = NULL ;

      try
      {
         BSONElement ele = recycleObject.getField( CAT_CATALOGNAME_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_CAT_CORRUPTION, error, PDERROR,
                   "Failed to get collection name from object" ) ;
         originName = ele.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse recycle object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _buildCLObject( originName, recycleObject, cb, w, returnObject,
                           needProcess ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build collection object [%s] "
                   "for return, rc: %d", originName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCLPROCESS__BLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCLPROCESS__BLDCLOBJ, "_catReturnCLProcessor::_buildCLObject" )
   INT32 _catReturnCLProcessor::_buildCLObject( const CHAR *originName,
                                                const bson::BSONObj &recycleObject,
                                                pmdEDUCB *cb,
                                                INT16 w,
                                                BSONObj &returnObject,
                                                BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCLPROCESS__BLDCLOBJ ) ;

      INT32 clVersion = CAT_VERSION_BEGIN ;
      BSONObj subCLInfo, autoIncInfo ;
      BOOLEAN needRebuildSubCL = FALSE ;
      BOOLEAN needRebuildAutoInc = FALSE ;
      utilReturnNameInfo mainCLNameInfo ;
      BOOLEAN needCheckCS = ( UTIL_RECYCLE_CS == _item.getType() ) ;

      needProcess = TRUE ;

      clsCatalogSet returnSet( originName ) ;

      if ( _info.isMissingCL( originName ) )
      {
         PD_LOG( PDDEBUG, "Collection [%s] is missing", originName ) ;
         needProcess = FALSE ;
         goto done ;
      }

      rc = returnSet.updateCatSet( recycleObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update catalog set [%s], "
                   "rc: %d", originName, rc ) ;

      // double check if data source is still exist
      // NOTE: we don't have a lock on data source
      if ( UTIL_INVALID_DS_UID != returnSet.getDataSourceID() )
      {
         UTIL_DS_UID dsUID = returnSet.getDataSourceID() ;
         BOOLEAN isExist = FALSE ;

         rc = _info.checkDSExist( dsUID, isExist, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check data source [%llu], "
                      "rc: %d", dsUID, rc ) ;

         if ( !isExist )
         {
            PD_LOG( PDDEBUG, "Data source [%llu] for collection [%s] "
                    "is missing", dsUID, originName ) ;

            // not exist, ignore it
            if ( returnSet.isSubCL() )
            {
               const CHAR *mainCLName = returnSet.getMainCLName().c_str() ;
               rc = _info.addMissingSubCL( originName, mainCLName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add missing "
                            "sub-collection [%s] from main-collection [%s], "
                            "rc: %d", originName, mainCLName, rc ) ;
            }
            needProcess = FALSE ;
            goto done ;
         }
      }

      // get return name from return info
      _returnNameInfo.init( originName ) ;
      rc = _info.getReturnCLName( _returnNameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                   "collection [%s], rc: %d", originName, rc ) ;

      // get return unique ID from return info
      _returnUIDInfo.init( returnSet.clUniqueID() ) ;
      rc = _info.getReturnCLUID( _returnUIDInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return unique ID for "
                   "collection [%s], rc: %d", originName, rc ) ;

      // update version
      clVersion = catGetBucketVersion( originName, cb ) ;
      if ( _returnNameInfo.isRenamed() )
      {
         INT32 renamedCLVersion =
               catGetBucketVersion( _returnNameInfo.getReturnName(), cb ) ;
         clVersion = OSS_MAX( clVersion, renamedCLVersion ) ;
      }

      if ( returnSet.isMainCL() )
      {
         CLS_SUBCL_LIST subCLList ;

         rc = returnSet.getSubCLList( subCLList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list, "
                      "rc: %d", rc ) ;

         for ( CLS_SUBCL_LIST_IT iter = subCLList.begin() ;
               iter != subCLList.end() ;
               ++ iter )
         {
            const CHAR *origSubCLName = iter->c_str() ;

            if ( _info.isMissingSubCL( origSubCLName ) )
            {
               // it is already mark missing, just remove it
               returnSet.delSubCL( origSubCLName ) ;
               needRebuildSubCL = TRUE ;
            }
            else
            {
               utilReturnNameInfo subCLNameInfo( origSubCLName ) ;
               BOOLEAN isInSameCS = FALSE ;

               rc = _rebuildCLName( _returnNameInfo.getReturnName(),
                                    subCLNameInfo, needCheckCS, isInSameCS ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to rebuild name for "
                            "sub-collection [%s], rc: %d", origSubCLName,
                            rc ) ;

               if ( needCheckCS && !isInSameCS )
               {
                  // not in the same collection space, we will not return the
                  // relationship of sub-collection from different collection
                  // space, so remove this sub-collection from return
                  // collection
                  returnSet.delSubCL( origSubCLName ) ;
                  needRebuildSubCL = TRUE ;
               }
               else if ( subCLNameInfo.isRenamed() )
               {
                  // sub-collection is renamed
                  returnSet.renameSubCL( origSubCLName,
                                         subCLNameInfo.getReturnName() ) ;
                  needRebuildSubCL = TRUE ;
               }
            }
         }

         if ( needRebuildSubCL )
         {
            subCLInfo = returnSet.toCataInfoBson() ;
         }
      }
      else if ( returnSet.isSubCL() )
      {
         const CHAR *origMainCLName = returnSet.getMainCLName().c_str() ;

         if ( _info.isMissingMainCL( origMainCLName ) )
         {
            // it is already mark missing, just remove it
            mainCLNameInfo.setReturnName( "" ) ;
         }
         else
         {
            BOOLEAN isInSameCS = FALSE ;
            mainCLNameInfo.init( origMainCLName ) ;

            rc = _rebuildCLName( _returnNameInfo.getReturnName(),
                                 mainCLNameInfo, needCheckCS, isInSameCS ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to rebuild name for "
                         "main-collection [%s], rc: %d", origMainCLName, rc ) ;

            if ( needCheckCS && !isInSameCS )
            {
               // not in the same collection space, we will not return the
               // relationship of main-collection from different collection
               // space, so remove main-collection from return collection
               mainCLNameInfo.setReturnName( "" ) ;
            }
         }
      }

      if ( _returnUIDInfo.isChanged() &&
           returnSet.getAutoIncSet()->itemCount() > 0 )
      {
         // auto-incremental fields have reference to unique ID
         rc = _rebuildAutoInc( returnSet, autoIncInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rebuild auto-increment "
                      "info, rc: %d", rc ) ;
         needRebuildAutoInc = TRUE ;
      }

      rc = _rebuildObject( recycleObject, clVersion, _returnUIDInfo,
                           _returnNameInfo, mainCLNameInfo, needRebuildSubCL,
                           subCLInfo, needRebuildAutoInc, autoIncInfo,
                           returnObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to rebuild object for "
                   "collection [return %s, origin %s], rc: %d",
                   _returnNameInfo.getReturnName(), originName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCLPROCESS__BLDCLOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCLPROCESS__REBLDCLNAME, "_catReturnCLProcessor::_rebuildCLName" )
   INT32 _catReturnCLProcessor::_rebuildCLName( const CHAR *targetCLName,
                                                utilReturnNameInfo &nameInfo,
                                                BOOLEAN needCheckCS,
                                                BOOLEAN &isInSameCS )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCLPROCESS__REBLDCLNAME ) ;

      BOOLEAN checkRename = TRUE ;
      const CHAR *originCLName = nameInfo.getOriginName() ;

      if ( needCheckCS )
      {
         rc = rtnCollectionsInSameSpace( targetCLName,
                                         ossStrlen( targetCLName ),
                                         originCLName,
                                         ossStrlen( originCLName ),
                                         isInSameCS ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check collection space "
                      "for collection [%s] and [%s], rc: %d",
                      targetCLName, originCLName, rc ) ;

         if ( !isInSameCS )
         {
            checkRename = FALSE ;
         }
      }

      if ( checkRename )
      {
         rc = _info.getReturnCLName( nameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                      "collection [%s], rc: %d", originCLName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCLPROCESS__REBLDCLNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCLPROCESS__REBLDOBJ, "_catReturnCLProcessor::_rebuildObject" )
   INT32 _catReturnCLProcessor::_rebuildObject( const BSONObj &recycleObject,
                                                INT32 clVersion,
                                                const utilReturnUIDInfo &uidInfo,
                                                const utilReturnNameInfo &nameInfo,
                                                const utilReturnNameInfo &mainCLNameInfo,
                                                BOOLEAN rebuildSubCL,
                                                const BSONObj &subCLInfo,
                                                BOOLEAN rebuildAutoInc,
                                                const BSONObj &autoIncInfo,
                                                BSONObj &returnObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCLPROCESS__REBLDOBJ ) ;

      try
      {
         BSONObjBuilder builder ;
         OID returnOID = OID::gen() ;

         // rebuild origin object
         builder.append( DMS_ID_KEY_NAME, returnOID ) ;

         BSONObjIterator iter( recycleObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            const CHAR *fieldName = element.fieldName() ;
            if ( 0 == ossStrcmp( DMS_ID_KEY_NAME, fieldName ) ||
                 0 == ossStrcmp( FIELD_NAME_RECYCLE_ID, fieldName ) )
            {
               // append nothing
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_VERSION, fieldName ) )
            {
               builder.append( FIELD_NAME_VERSION, clVersion ) ;
            }
            else if ( 0 == ossStrcmp( CAT_COLLECTION_NAME, fieldName ) )
            {
               builder.append( CAT_COLLECTION_NAME, nameInfo.getReturnName() ) ;
            }
            else if ( 0 == ossStrcmp( CAT_CL_UNIQUEID, fieldName ) )
            {
               builder.append( CAT_CL_UNIQUEID,
                               (INT64)( uidInfo.getReturnUID() ) ) ;
            }
            else if ( rebuildSubCL &&
                      0 == ossStrcmp( CAT_CATALOGINFO_NAME, fieldName ) )
            {
               builder.appendElements( subCLInfo ) ;
            }
            else if ( rebuildAutoInc &&
                      0 == ossStrcmp( CAT_AUTOINCREMENT, fieldName ) )
            {
               builder.appendElements( autoIncInfo ) ;
            }
            else if ( mainCLNameInfo.isRenamed() &&
                      0 == ossStrcmp( CAT_MAINCL_NAME, fieldName ) )
            {
               const CHAR *mainCLRtrnName = mainCLNameInfo.getReturnName() ;
               if ( NULL != mainCLRtrnName && '\0' != mainCLRtrnName[ 0 ] )
               {
                  builder.append( CAT_MAINCL_NAME, mainCLRtrnName ) ;
               }
            }
            else if ( 0 == ossStrcmp( CAT_GLOBAL_INDEX, fieldName ) )
            {
               // NOTE: we don't have index definition of global index
               // drop index instead, so don't append the global index field
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_UPDATE_TIME, fieldName ) )
            {
               // refresh update time
               UINT64 currentTime = ossGetCurrentMilliseconds() ;
               CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
               ossMillisecondsToString( currentTime, timestamp ) ;
               builder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;
            }
            else
            {
               builder.append( element ) ;
            }
         }

         returnObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build return object for collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCLPROCESS__REBLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCLPROCESS__REBLDAUTOINFO, "_catReturnCLProcessor::_rebuildAutoInc" )
   INT32 _catReturnCLProcessor::_rebuildAutoInc( const clsCatalogSet &returnSet,
                                                 BSONObj &autoIncInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCLPROCESS__REBLDAUTOINFO ) ;

      try
      {
         BSONObjBuilder builder ;
         const clsAutoIncItem *item = NULL ;
         BSONArrayBuilder arrBuilder(
               builder.subarrayStart( CAT_AUTOINCREMENT ) ) ;

         clsAutoIncIterator it( *( returnSet.getAutoIncSet() ),
                                clsAutoIncIterator::RECURS ) ;
         while ( it.more() )
         {
            item = it.next() ;

            const CHAR *origSeqName = item->sequenceName() ;
            const CHAR *rtrnSeqName = origSeqName ;
            utilSequenceID origSeqUID = item->sequenceID() ;
            utilSequenceID rtrnSeqUID = origSeqUID ;

            _info.getRenameSeq( origSeqName, rtrnSeqName ) ;
            _info.getChangeUIDSeq( origSeqUID, rtrnSeqUID ) ;

            BSONObjBuilder subBuilder( arrBuilder.subobjStart() ) ;
            subBuilder.append( CAT_AUTOINC_SEQ, rtrnSeqName ) ;
            subBuilder.append( CAT_AUTOINC_FIELD, item->fieldFullName() ) ;
            subBuilder.append( CAT_AUTOINC_GENERATED, item->generated() ) ;
            subBuilder.append( CAT_AUTOINC_SEQ_ID, (INT64)( rtrnSeqUID ) ) ;
            subBuilder.done() ;
         }
         arrBuilder.done() ;
         autoIncInfo = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rebuild sequence set, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCLPROCESS__REBLDAUTOINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNCLPROCESS__POSTSAVEOBJ, "_catReturnCLProcessor::_postSaveObject" )
   INT32 _catReturnCLProcessor::_postSaveObject( const BSONObj &recycleObject,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNCLPROCESS__POSTSAVEOBJ ) ;

      if ( UTIL_RECYCLE_CL == _item.getType() )
      {
         // for returning collection, need update collection array in
         // collection space
         CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
         CHAR szCollection[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

         const CHAR *clName = _returnNameInfo.getReturnName() ;
         utilCLUniqueID clUniqueID =
               (utilCLUniqueID)( _returnUIDInfo.getReturnUID() ) ;

         // split collection full name to csname and clname
         rc = rtnResolveCollectionName( clName, ossStrlen( clName ),
                                        szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                        szCollection, DMS_COLLECTION_NAME_SZ ) ;
         PD_RC_CHECK ( rc, PDWARNING, "Failed to resolve collection name: %s, "
                       "rc: %d", _returnNameInfo.getReturnName(), rc ) ;

         // add to collection space
         rc = catAddCL2CS( szSpace, szCollection, clUniqueID, cb, _dmsCB,
                           _dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add collection [%s] into "
                      "collection space [%s], rc: %d",
                      clName, szSpace, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNCLPROCESS__POSTSAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catReturnSeqProcessor implement
    */
   _catReturnSeqProcessor::_catReturnSeqProcessor( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem &item,
                                                   catRecycleReturnInfo &info )
   : _catReturnProcessor( recyBinMgr, item, info, UTIL_RECYCLE_SEQ )
   {
   }

   _catReturnSeqProcessor::~_catReturnSeqProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNSEQPROCESS__BLDOBJ, "_catReturnSeqProcessor::_buildObject" )
   INT32 _catReturnSeqProcessor::_buildObject( const BSONObj &recycleObject,
                                               pmdEDUCB *cb,
                                               INT16 w,
                                               BSONObj &returnObject,
                                               BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNSEQPROCESS__BLDOBJ ) ;

      needProcess = TRUE ;

      try
      {
         BSONObjBuilder builder ;
         OID returnOID = OID::gen() ;

         builder.append( DMS_ID_KEY_NAME, returnOID ) ;

         BSONObjIterator iter( recycleObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            const CHAR *fieldName = element.fieldName() ;
            if ( 0 == ossStrcmp( DMS_ID_KEY_NAME, fieldName ) ||
                 0 == ossStrcmp( FIELD_NAME_RECYCLE_ID, fieldName ) )
            {
               // append nothing
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_SEQUENCE_NAME, fieldName ) )
            {
               // check if sequence is renamed

               const CHAR *origSeqName = NULL ;
               const CHAR *rtrnSeqName = NULL ;
               PD_CHECK( String == element.type(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get field [%s], it is not a string",
                         FIELD_NAME_SEQUENCE_NAME ) ;
               origSeqName = element.valuestr() ;

               if ( _info.getRenameSeq( origSeqName, rtrnSeqName ) )
               {
                  builder.append( FIELD_NAME_SEQUENCE_NAME, rtrnSeqName ) ;
               }
               else
               {
                  builder.append( FIELD_NAME_SEQUENCE_NAME, origSeqName ) ;
               }
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_SEQUENCE_ID, fieldName ) )
            {
               // check if unique ID is changed

               utilSequenceID origSeqUID = UTIL_SEQUENCEID_NULL ;
               utilSequenceID rtrnSeqUID = UTIL_SEQUENCEID_NULL ;

               PD_CHECK( element.isNumber(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get field [%s], it is not a number",
                         FIELD_NAME_SEQUENCE_ID ) ;
               origSeqUID = (utilSequenceID)( element.numberLong() ) ;

               if ( _info.getChangeUIDSeq( origSeqUID, rtrnSeqUID ) )
               {
                  builder.append( FIELD_NAME_SEQUENCE_ID, (INT64)rtrnSeqUID ) ;
               }
               else
               {
                  builder.append( FIELD_NAME_SEQUENCE_ID, (INT64)origSeqUID ) ;
               }
            }
            else
            {
               builder.append( element ) ;
            }
         }

         returnObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build return object for sequence, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNSEQPROCESS__BLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   /*
      _catReturnIdxProcessor implement
    */
   _catReturnIdxProcessor::_catReturnIdxProcessor( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem &item,
                                                   catRecycleReturnInfo &info,
                                                   UINT16 rebuildTypes )
   : _catReturnProcessor( recyBinMgr, item, info, UTIL_RECYCLE_IDX ),
     _rebuildTypes( rebuildTypes )
   {
   }

   _catReturnIdxProcessor::~_catReturnIdxProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRTRNSIDXROCESS__BLDOBJ, "_catReturnIdxProcessor::_buildObject" )
   INT32 _catReturnIdxProcessor::_buildObject( const BSONObj &recycleObject,
                                               pmdEDUCB *cb,
                                               INT16 w,
                                               BSONObj &returnObject,
                                               BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRTRNSIDXROCESS__BLDOBJ ) ;

      needProcess = TRUE ;

      try
      {
         BSONObj indexOptions ;
         UINT16 indexType = IXM_EXTENT_TYPE_NONE ;

         BSONElement element = recycleObject.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not object",
                   IXM_FIELD_NAME_INDEX_DEF ) ;
         indexOptions = element.embeddedObject() ;

         rc = ixmGetIndexType( indexOptions, indexType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index type from [%s], "
                      "rc: %d", indexOptions.toPoolString().c_str(), rc ) ;

         if ( OSS_BIT_TEST( indexType, _rebuildTypes ) )
         {
            // skip index which will be rebuild
            needProcess = FALSE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse index object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( !needProcess )
      {
         goto done ;
      }

      try
      {
         BSONObjBuilder builder ;
         OID returnOID = OID::gen() ;

         builder.append( DMS_ID_KEY_NAME, returnOID ) ;

         BSONObjIterator iter( recycleObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            const CHAR *fieldName = element.fieldName() ;
            if ( 0 == ossStrcmp( DMS_ID_KEY_NAME, fieldName ) ||
                 0 == ossStrcmp( FIELD_NAME_RECYCLE_ID, fieldName ) )
            {
               // append nothing
            }
            else if ( ( _info.hasRenameCS() ||
                        _info.hasRenameCL() ) &&
                      ( 0 == ossStrcmp( FIELD_NAME_COLLECTION, fieldName ) ) )
            {
               // check if collection is renamed

               utilReturnNameInfo returnCLNameInfo ;

               PD_CHECK( String == element.type(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get field [%s], it is not a string",
                         FIELD_NAME_COLLECTION ) ;
               returnCLNameInfo.init( element.valuestr() ) ;

               // get return collection name from return info
               rc = _info.getReturnCLName( returnCLNameInfo ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                            "collection [%s], rc: %d",
                            returnCLNameInfo.getOriginName(), rc ) ;

               builder.append( FIELD_NAME_COLLECTION,
                               returnCLNameInfo.getReturnName() ) ;
            }
            else if ( ( _info.hasChangeUIDCL() ) &&
                      ( 0 == ossStrcmp( FIELD_NAME_CL_UNIQUEID, fieldName ) ) )
            {
               // check if collection is unique ID changed

               utilReturnUIDInfo returnCLUIDInfo ;

               PD_CHECK( element.isNumber(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get field [%s], it is not a number",
                         FIELD_NAME_CL_UNIQUEID ) ;
               returnCLUIDInfo.init(
                     (utilCLUniqueID)( element.numberLong() ) ) ;

               // get return collection unique ID from return info
               rc = _info.getReturnCLUID( returnCLUIDInfo ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get return unique ID for "
                            "collection [%llu], rc: %d",
                            returnCLUIDInfo.getOriginUID(), rc ) ;

               builder.append( FIELD_NAME_CL_UNIQUEID,
                               (INT64)( returnCLUIDInfo.getReturnUID() ) ) ;
            }
            else
            {
               builder.append( element ) ;
            }
         }

         returnObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build return object for index, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRTRNSIDXROCESS__BLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleSubCLLocker implement
    */
   _catRecycleSubCLLocker::_catRecycleSubCLLocker( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem item,
                                                   catCtxLockMgr &lockMgr,
                                                   OSS_LATCH_MODE &mode,
                                                   ossPoolSet< utilCSUniqueID > *lockedCS,
                                                   ossPoolSet< utilCSUniqueID > &lockedSubCLCS )
   : _catRecycleBinProcessor( recyBinMgr, item ),
     _lockMgr( lockMgr ),
     _lockMode( mode ),
     _lockedCS( lockedCS ),
     _lockedSubCLCS( lockedSubCLCS )
   {
   }
   _catRecycleSubCLLocker:: ~_catRecycleSubCLLocker()
   {
   }

   const CHAR *_catRecycleSubCLLocker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCLESUBCLLOCK_GETMATCHER, "_catRecycleSubCLLocker::getMatcher" )
   INT32 _catRecycleSubCLLocker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
       INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCLESUBCLLOCK_GETMATCHER ) ;

      try
      {
         BSONObj matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                                 (INT64)( _item.getRecycleID() ) <<
                                 FIELD_NAME_MAINCLNAME <<
                                 _item.getOriginName() ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for subCL locker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCLESUBCLLOCK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYCLESUBCLLOCK_PROCESSOBJ, "_catRecycleSubCLLocker::processObject" )
   INT32  _catRecycleSubCLLocker::processObject( const BSONObj &object,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYCLESUBCLLOCK_PROCESSOBJ ) ;

      const CHAR *clName = NULL ;

      try
      {
         BSONElement element = object.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   CAT_COLLECTION_NAME ) ;
         clName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse collection object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
      try
      {
         clsCatalogSet originSet( clName ) ;
         rc = originSet.updateCatSet( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for collection "
                      "[%s], rc: %d", _item.getOriginName(), rc ) ;

         SDB_ASSERT( originSet.isSubCL(), "should be sub-collection" ) ;

         if ( originSet.isSubCL() )
         {
            utilRecycleItem subItem ;
            subItem.inherit( _item,
                             clName,
                             originSet.clUniqueID() ) ;

            if ( ( _lockedCS == NULL ||
                  !_lockedCS->count( utilGetCSUniqueID( subItem.getOriginID() ) ) ) &&
                  !_lockedSubCLCS.count( utilGetCSUniqueID( subItem.getOriginID() ) ) )
            {
               PD_CHECK( _lockMgr.tryLockRecycleItem( subItem, _lockMode ),
                         SDB_LOCK_FAILED, error, PDERROR,
                         "Failed to lock recycle item [origin %s, recycle %s]",
                         subItem.getOriginName(), subItem.getRecycleName() ) ;
               _lockedSubCLCS.insert( utilGetCSUniqueID( subItem.getOriginID() ) ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to lock recycle items, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYCLESUBCLLOCK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

    /*
      _catDropCSItemChecker implement
    */
   _catDropCSItemChecker::_catDropCSItemChecker( _catRecycleBinManager *recyBinMgr,
                                                 utilRecycleItem &item )
   : _catRecycleBinProcessor( recyBinMgr, item )
   {
   }

   _catDropCSItemChecker::~_catDropCSItemChecker()
   {
   }

   const CHAR *_catDropCSItemChecker::getCollection() const
   {
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPCSITEMCHK_GETMATCHER, "_catDropCSItemChecker::getMatcher" )
   INT32 _catDropCSItemChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32  rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPCSITEMCHK_GETMATCHER ) ;

      try
      {
         utilCSUniqueID csUniqueID = (utilCSUniqueID)( _item.getOriginID() ) ;
         BSONObjBuilder builder ;
         BSONObj matcher ;

         // check collections in the same collection space but in another
         // recycle item
         rc = utilGetCSBounds( CAT_CL_UNIQUEID, csUniqueID, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                      "space unique ID [%u], rc: %d", csUniqueID, rc ) ;

         builder.append( FIELD_NAME_RECYCLE_ID,
                         BSON( "$ne" << (INT64)( _item.getRecycleID() ) ) ) ;

         matcher = builder.obj() ;

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return checker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATDROPCSITEMCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPCSITEMCHK_PROCESSOBJ, "_catDropCSItemChecker::processObject" )
   INT32 _catDropCSItemChecker::processObject( const BSONObj &object,
                                               pmdEDUCB *cb,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPCSITEMCHK_PROCESSOBJ ) ;

      const CHAR *clName = NULL ;
      utilRecycleID recycleID = UTIL_GLOBAL_NULL ;
      utilRecycleItem item ;

      // found recycled collection space, report error, we should not drop it
      rc = rtnGetStringElement( object, CAT_COLLECTION_NAME, &clName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from object, "
                   "rc: %d", CAT_COLLECTION_NAME, rc ) ;

      rc = rtnGetNumberLongElement( object, FIELD_NAME_RECYCLE_ID,
                                    (INT64 &)( recycleID ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from object, "
                   "rc: %d", FIELD_NAME_RECYCLE_ID, rc ) ;

      // check if has a recycled collection space
      rc = _recyBinMgr->getItemByRecyID( recycleID, cb, item ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         PD_LOG( PDWARNING, "Failed to get recycle item [recycle ID %llu], "
                 "which is missing for collection [%s]", recycleID, clName ) ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item "
                   "[recycle ID %llu], rc: %d", recycleID, rc ) ;

      PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                        "Failed to drop collection space recycle item "
                        "[origin %s, recycle %s], collection [%s] in recycle "
                        "item [origin %s, recycle %s] is also in recycle bin",
                        _item.getOriginName(), _item.getRecycleName(), clName,
                        item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATDROPCSITEMCHK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }


}
