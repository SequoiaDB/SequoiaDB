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

   Source File Name = ixm.cpp

   Descriptive Name = Index Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for
   Index Manager Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ixm.hpp"
#include "dmsStorageIndex.hpp"
#include "ixmIndexKey.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"
#include "pdSecure.hpp"

using namespace bson;

namespace engine
{
   // Before using ixmIndexCB, after create the object user must check
   // isInitialized ()
   // create index details from existing extent
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB1, "_ixmIndexCB::_ixmIndexCB" )
   _ixmIndexCB::_ixmIndexCB ( dmsExtentID extentID,
                              _dmsStorageIndex *pIndexSu,
                              _dmsContext *context )
   :_extent( NULL )
   {
      SDB_ASSERT ( pIndexSu, "index su can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__IXMINXCB1 );
      _isGlobalIndex = FALSE;
      _indexCLUID = UTIL_UNIQUEID_NULL ;
      _idxUniqID = UTIL_UNIQUEID_NULL ;
      _isInitialized = FALSE ;
      _pIndexSu = pIndexSu ;
      _pContext = context ;
      _extentID = extentID ;
      _pageSize = _pIndexSu->pageSize () ;
      _extent = (const ixmIndexCBExtent*)pIndexSu->beginFixedAddr( extentID,
                                                                   1 ) ;
      _indexObjVersion = 0 ;
      _name = NULL ;
      _unique = FALSE ;
      _enforced = FALSE ;
      _notNull = FALSE ;
      _notArray = FALSE ;
      _dropDups = FALSE ;
      _isIDIndex = FALSE ;
      _nameExtData = NULL ;
      _fieldInitedFlag = 0 ;
      _init() ;
      PD_TRACE_EXIT( SDB__IXMINXCB1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB2, "_ixmIndexCB::_ixmIndexCB" )
   _ixmIndexCB::_ixmIndexCB ( dmsExtentID extentID,
                              const BSONObj &infoObj,
                              UINT16 mbID ,
                              _dmsStorageIndex *pIndexSu,
                              _dmsContext *context )
   {
      SDB_ASSERT ( pIndexSu, "index su can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__IXMINXCB2 );

      _isGlobalIndex = FALSE;
      _indexCLUID = UTIL_UNIQUEID_NULL ;
      _idxUniqID = UTIL_UNIQUEID_NULL ;
      ixmIndexCBExtent *pExtent = NULL ;
      _isInitialized = FALSE ;
      dmsExtRW extRW ;
      _pIndexSu = pIndexSu ;
      _pContext = context ;
      _extentID = extentID ;
      _pageSize = _pIndexSu->pageSize() ;
      _indexObjVersion = 0 ;
      _name = NULL ;
      _unique = FALSE ;
      _enforced = FALSE ;
      _notNull = FALSE ;
      _notArray = FALSE ;
      _dropDups = FALSE ;
      _isIDIndex = FALSE ;
      _nameExtData = NULL ;
      _fieldInitedFlag = 0 ;

      _extent = (const ixmIndexCBExtent*)pIndexSu->beginFixedAddr ( extentID,
                                                                    1 ) ;
      extRW = pIndexSu->extent2RW( extentID, context->mbID() ) ;
      pExtent = extRW.writePtr<ixmIndexCBExtent>( 0, _pageSize ) ;

      // make sure the index object is not too big
      if ( infoObj.objsize() + IXM_INDEX_CB_EXTENT_METADATA_SIZE >=
           (UINT32)_pageSize )
      {
         PD_LOG ( PDERROR, "index object is too big: %s",
                  infoObj.toString().c_str() ) ;
         goto error ;
      }

      pExtent->_type = IXM_EXTENT_TYPE_NONE ;
      if ( SDB_OK != generateIndexType( infoObj, pExtent->_type ) )
      {
         goto error ;
      }

      // Caller must make sure the given extentID is free and can be used
      // In ixmIndexDetails we don't bother to check whether the extent is
      // freed or not

      // write stuff into extent
      pExtent->_flag           = DMS_EXTENT_FLAG_INUSE ;
      pExtent->_eyeCatcher [0] = IXM_EXTENT_CB_EYECATCHER0 ;
      pExtent->_eyeCatcher [1] = IXM_EXTENT_CB_EYECATCHER1 ;
      pExtent->_indexFlag      = IXM_INDEX_FLAG_INVALID ;
      pExtent->_mbID           = mbID ;
      pExtent->_version        = DMS_EXTENT_CURRENT_V ;
      pExtent->_logicID        = DMS_INVALID_EXTENT ;
      pExtent->_scanExtLID     = DMS_INVALID_EXTENT ;
      // not creating index root page yet
      pExtent->_rootExtentID   = DMS_INVALID_EXTENT ;
      ossMemset( pExtent->_reserved, 0, sizeof( pExtent->_reserved ) ) ;
      pExtent->_scanExtOffset  = DMS_INVALID_OFFSET ;
      // copy index def into extent. when it is replay op(has oid already),
      // no need to add oid.
      if ( !infoObj.hasField (DMS_ID_KEY_NAME) )
      {
         _IDToInsert oid ;
         oid._oid.init() ;
         *(INT32*)(((CHAR*)pExtent) +IXM_INDEX_CB_EXTENT_METADATA_SIZE) =
               infoObj.objsize() + sizeof(_IDToInsert) ;
         ossMemcpy ( ((CHAR*)pExtent) +
                     IXM_INDEX_CB_EXTENT_METADATA_SIZE +
                     sizeof(INT32),
                     (CHAR*)(&oid),
                     sizeof(_IDToInsert)) ;
         ossMemcpy ( ((CHAR*)pExtent) +
                     IXM_INDEX_CB_EXTENT_METADATA_SIZE +
                     sizeof(INT32) +
                     sizeof(_IDToInsert),
                     infoObj.objdata()+sizeof(INT32),
                     infoObj.objsize()-sizeof(INT32) ) ;
      }
      else
      {
         ossMemcpy ( ((CHAR*)pExtent) +
                     IXM_INDEX_CB_EXTENT_METADATA_SIZE,
                     infoObj.objdata(),
                     infoObj.objsize() ) ;
      }
      // call _init() to load things back from page
      _init() ;

   done :
      PD_TRACE_EXIT ( SDB__IXMINXCB2 );
      return ;
   error :
      goto done ;
   }

   _ixmIndexCB::~_ixmIndexCB()
   {
      if ( _extent )
      {
         _pIndexSu->endFixedAddr( (const ossValuePtr)_extent ) ;
      }
      _pIndexSu = NULL ;
      _pContext = NULL ;
   }

   void _ixmIndexCB::setFlag( UINT16 flag )
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID,
                                             _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_indexFlag = flag ;
   }

   void _ixmIndexCB::setLogicalID( dmsExtentID logicalID )
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID,
                                             _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_logicID = logicalID ;
   }

   void _ixmIndexCB::clearLogicID()
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID,
                                             _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_logicID = DMS_INVALID_EXTENT ;
   }

   void _ixmIndexCB::setScanRID( const dmsRecordID &rid )
   {
      SDB_ASSERT( _isInitialized, "index details must be initialized first" ) ;
      if ( rid.isValid() )
      {
         dmsExtRW extRW = _pIndexSu->extent2RW( _extentID, _pContext->mbID() ) ;
         ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
         pExtent->_scanExtLID = rid._extent ;
         pExtent->_scanExtOffset = rid._offset ;
      }
   }

   void _ixmIndexCB::scanExtLID ( UINT32 extLID )
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID,
                                             _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_scanExtLID = extLID ;
   }

   void _ixmIndexCB::setScanExtOffset( UINT32 offset )
   {
      SDB_ASSERT( _isInitialized, "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID, _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_scanExtOffset = offset ;
   }

   void _ixmIndexCB::setRoot ( dmsExtentID rootExtentID )
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID,
                                             _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_rootExtentID = rootExtentID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB_GETKEY, "_ixmIndexCB::getKeysFromObject" )
   INT32 _ixmIndexCB::getKeysFromObject ( const BSONObj &obj,
                                          BSONObjSet &keys,
                                          BOOLEAN *pAllUndefined,
                                          BOOLEAN checkValid,
                                          utilWriteResult *pResult ) const
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      PD_TRACE_ENTRY ( SDB__IXMINXCB_GETKEY );
      BSONElement arrEle ;
      ixmIndexKeyGen keyGen(this) ;
      rc = keyGen.getKeys ( obj, keys, &arrEle, FALSE, FALSE, pAllUndefined ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to generate key from object, rc: %d", rc ) ;
         goto error ;
      }

      if ( checkValid )
      {
         rc = checkKeys( obj, keys, arrEle, pResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check keys for object [%s], "
                      "rc: %d", PD_SECURE_OBJ( obj ), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMINXCB_GETKEY, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _ixmIndexCB::_checkNullKeys( const BSONObjSet &keys,
                                      utilWriteResult *pResult ) const
   {
      INT32 rc = SDB_OK ;

      for ( BSONObjSet::const_iterator iter = keys.begin() ;
            iter != keys.end() ;
            ++ iter )
      {
         try
         {
            BSONObjIterator bIter( *iter ) ;
            while ( bIter.more() )
            {
               BSONElement ele = bIter.next() ;
               if ( Undefined == ele.type() ||
                    jstNULL == ele.type() )
               {
                  rc = SDB_IXM_KEY_NOTNULL ;
                  PD_LOG( PDERROR, "Failed to check keys, index not support NULL" ) ;
                  if ( NULL != pResult )
                  {
                     INT32 tmpRC = pResult->setIndexErrInfo( getName(), keyPattern(), *iter ) ;
                     if ( tmpRC )
                     {
                        rc = tmpRC ;
                     }
                  }
                  goto error ;
               }
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse object, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB_CHECKKEYS, "_ixmIndexCB::checkKeys" )
   INT32 _ixmIndexCB::checkKeys( const BSONObj &obj,
                                 const BSONObjSet &keys,
                                 const BSONElement &arrEle,
                                 utilWriteResult *pResult ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMINXCB_CHECKKEYS ) ;

      if ( notArray() )
      {
         rc = _checkArrayKeys( obj, keys, arrEle, pResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check array keys, rc: %d", rc ) ;
      }

      if ( notNull() )
      {
         rc = _checkNullKeys( keys, pResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check NULL keys, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMINXCB_CHECKKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   static INT32 _ixmBuildArrayKeys( const BSONElement &arrEle,
                                    const CHAR *arrEleName,
                                    BSONArrayBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj arrObj = arrEle.embeddedObject() ;

         /// the element must be an empty array key when it is a empty array
         if ( arrObj.firstElement().eoo() )
         {
            // the matched field is empty array, save as empty array
            builder.append( arrEle ) ;
         }
         else if ( '\0' == *arrEleName )
         {
            // hit the end of name, but still an array
            // we need to unpack the array, each element will be a key
            BSONObjIterator itr( arrObj ) ;
            while ( itr.more() )
            {
               BSONElement e = itr.next() ;

               // on found the array key
               builder.append( e ) ;
            }
         }
         else
         {
            // unpack the array to find the field
            BSONObjIterator itr( arrObj ) ;
            while ( itr.more() )
            {
               const CHAR *curEleName = arrEleName ;
               BSONElement next = itr.next() ;
               BSONElement subEle ;
               BOOLEAN found = FALSE ;
               if ( Object == next.type() )
               {
                  // search deeper into the sub-object
                  subEle =
                        next.embeddedObject().getFieldDottedOrArray( curEleName ) ;
                  if ( Array == subEle.type() )
                  {
                     // still got an array, should recursively extract
                     // from sub-object with sub-fields
                     rc = _ixmBuildArrayKeys( subEle, curEleName, builder ) ;
                     PD_RC_CHECK( rc, PDERROR, "Failed to build key from array element, rc: %d",
                                  rc ) ;
                     continue ;
                  }
                  // else if not an array, means we finally found the element
                  // inside the sub-object
                  found = TRUE ;
               }
               // else if not an object, means not matched
               // use the EOO element, which will build as undefined

               if ( found )
               {
                  // on found the array key
                  builder.append( subEle ) ;
               }
               else
               {
                  builder.appendUndefined() ;
               }
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build array keys, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _ixmIndexCB::_checkArrayKeys( const BSONObj &obj,
                                       const BSONObjSet &keys,
                                       const BSONElement &arrEle,
                                       utilWriteResult *pResult ) const
   {
      INT32 rc = SDB_OK ;

      if ( !arrEle.eoo() )
      {
         rc = SDB_IXM_KEY_NOT_SUPPORT_ARRAY ;
         PD_LOG( PDERROR, "Failed to check keys, index not support array" ) ;
         if ( NULL != pResult )
         {
            // build error result info
            INT32 tmpRC = SDB_OK ;

            try
            {
               // build keys, e.g. { a: 1, b: [1, 2, 3] }
               BSONObjBuilder builder( arrEle.size() + keyPattern().objsize() + 64 ) ;
               BSONObjIterator iter( keyPattern() ) ;

               while ( iter.more() )
               {
                  BSONElement beIdxKey = iter.next() ;
                  const CHAR *fieldName = beIdxKey.fieldName() ;
                  const CHAR *curName = fieldName ;
                  BSONObj curObj = obj ;
                  while ( NULL != curName )
                  {
                     BSONElement subEle ;
                     const CHAR *p = strchr( curName, '.' ) ;
                     if ( NULL != p )
                     {
                        subEle = curObj.getField( StringData( curName, p - curName ) ) ;
                        curName = p + 1 ;
                     }
                     else
                     {
                        subEle = curObj.getField( curName ) ;
                        curName = NULL ;
                     }

                     if ( curName == NULL || '\0' == curName[ 0 ] )
                     {
                        builder.appendAs( subEle, "" ) ;
                        break ;
                     }
                     else if ( Object == subEle.type() )
                     {
                        curObj = subEle.embeddedObject() ;
                        continue ;
                     }
                     else if ( Array == subEle.type() )
                     {
                        // build from current array element
                        BSONArrayBuilder arrBuilder( builder.subarrayStart( "" ) ) ;
                        tmpRC = _ixmBuildArrayKeys( subEle, curName, arrBuilder ) ;
                        if ( SDB_OK != tmpRC )
                        {
                           PD_LOG( PDERROR, "Failed to build key from array element, rc: %d",
                                   tmpRC ) ;
                           break ;
                        }
                        arrBuilder.doneFast() ;
                        break ;
                     }
                     else
                     {
                        SDB_ASSERT( FALSE, "should not be here" ) ;
                        tmpRC = SDB_SYS ;
                        break ;
                     }
                  }
                  if ( SDB_OK != tmpRC )
                  {
                     break ;
                  }
               }

               if ( SDB_OK == tmpRC )
               {
                  tmpRC = pResult->setIndexErrInfo( getName(), keyPattern(), builder.obj() ) ;
               }
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build error info, occur exception %s", e.what() ) ;
               tmpRC = ossException2RC( &e ) ;
            }
            if ( tmpRC )
            {
               rc = tmpRC ;
            }
         }
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // if the field exist in the index object, returns the ith position
   INT32 _ixmIndexCB::keyPatternOffset( const CHAR *key ) const
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      BSONObjIterator i ( keyPattern() ) ;
      INT32 n = 0 ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         if ( ossStrcmp ( key, e.fieldName() ) == 0 )
            return n ;
         n++ ;
      }
      return -1 ;
   }

   // allocate an extent for the index
   INT32 _ixmIndexCB::allocExtent ( dmsExtentID &extentID )
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      return _pIndexSu->reserveExtent ( _extent->_mbID, extentID,
                                        _pContext ) ;
   }
   INT32 _ixmIndexCB::freeExtent ( dmsExtentID extentID )
   {
      return _pIndexSu->releaseExtent ( extentID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB_TRUNC, "_ixmIndexCB::truncate" )
   INT32 _ixmIndexCB::truncate ( BOOLEAN removeRoot, UINT16 indexFlag,
                                 ossAtomic64* pDelKeyCnt )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMINXCB_TRUNC );
      PD_TRACE1 ( SDB__IXMINXCB_TRUNC, PD_PACK_INT(removeRoot) ) ;

      setFlag ( indexFlag ) ;
      scanExtLID ( DMS_INVALID_EXTENT ) ;
      PD_TRACE_EXITRC ( SDB__IXMINXCB_TRUNC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB_ISSAMEDEF, "_ixmIndexCB::isSameDef" )
   BOOLEAN _ixmIndexCB::isSameDef( const BSONObj &defObj,
                                   BOOLEAN strict ) const
   {
      //PD_TRACE_ENTRY ( SDB__IXMINXCB_ISSAMEDEF );
      SDB_ASSERT( TRUE == _isInitialized, "indexCB must be intialized!" );
      BOOLEAN rs = ixmIsSameDef( _infoObj, defObj, strict ) ;
      //PD_TRACE_EXIT( SDB__IXMINXCB_ISSAMEDEF );
      return rs ;
   }

   // Append extra fields to index definition. This is added in version 3.0.1,
   // in order to append an external data name into indexCB(For text index).
   // As in version 3.0, the name is not stored and was generated every time
   // when used.
   // Note: Only append, no existing part of the definition should be modified.
   INT32 _ixmIndexCB::extendDef( const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj newDef ;
      dmsExtRW extRW ;
      ixmIndexCBExtent *pExtent = NULL ;

      try
      {
         builder.appendElements( _infoObj ) ;
         builder.append( ele ) ;
         newDef = builder.done() ;

         extRW = _pIndexSu->extent2RW( _extentID, _extent->_mbID ) ;
         pExtent = extRW.writePtr<ixmIndexCBExtent>( 0, (UINT32)_pageSize ) ;

         ossMemcpy( ((CHAR *) pExtent) + IXM_INDEX_CB_EXTENT_METADATA_SIZE,
                    newDef.objdata(), (size_t)newDef.objsize() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "occur unexpected error(%s)", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _ixmIndexCB::_initGlobIndexInfo() const
   {
      INT32 rc = SDB_OK ;

      if ( GLOB_INDEX_IS_INITED() )
      {
         goto done ;
      }

      try
      {
         _isGlobalIndex = _infoObj.getBoolField( IXM_GLOBAL_FIELD ) ;
         if ( _isGlobalIndex )
         {
            BSONObj globalOptions ;
            BSONElement ele ;

            ele = _infoObj.getField( IXM_GLOBAL_OPTION_FIELD ) ;
            PD_CHECK( Object == ele.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s] from index [%s]",
                       IXM_GLOBAL_OPTION_FIELD,
                       _infoObj.toString().c_str() ) ;
            globalOptions = ele.embeddedObject() ;

            ele = globalOptions.getField( FIELD_NAME_CL_UNIQUEID ) ;
            PD_CHECK( NumberLong == ele.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s] from global option [%s]",
                      FIELD_NAME_CL_UNIQUEID,
                      globalOptions.toString().c_str() ) ;
            _indexCLUID = (utilCLUniqueID) ele.numberLong() ;

            ele = globalOptions.getField( FIELD_NAME_COLLECTION ) ;
            PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s] from global option [%s]",
                      FIELD_NAME_COLLECTION,
                      globalOptions.toString().c_str() ) ;
            _indexCLName = ele.valuestr() ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to extract global index info from "
                 "index pattern, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      SET_GLOB_INDEX_INITED() ;

   done:
      return rc ;

   error:
      // reset on error
      _isGlobalIndex = FALSE ;
      _indexCLUID = UTIL_UNIQUEID_NULL ;
      _indexCLName = NULL ;
   goto done ;
   }

   INT32 _ixmIndexCB::changeUniqueID( utilIdxUniqueID uniqueID )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj newDef ;
      dmsExtRW extRW ;
      ixmIndexCBExtent *pExtent = NULL ;

      try
      {
         // build new index definition
         BSONObjIterator it( _infoObj ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 != ossStrcmp( e.fieldName(), IXM_FIELD_NAME_UNIQUEID ) )
            {
               builder.append( e ) ;
            }
         }
         builder.append( IXM_FIELD_NAME_UNIQUEID, (INT64)uniqueID ) ;
         newDef = builder.done() ;

         // check new index definition
         if ( newDef.objsize() + IXM_INDEX_CB_EXTENT_METADATA_SIZE >=
              (UINT32)_pageSize )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "index object is too big, size: %d, object: %s",
                    newDef.objsize(), newDef.toString().c_str() ) ;
            goto error ;
         }

         // get extent
         extRW = _pIndexSu->extent2RW( _extentID, _extent->_mbID ) ;
         pExtent = extRW.writePtr<ixmIndexCBExtent>( 0, (UINT32)_pageSize ) ;

         // write index definition
         ossMemcpy( ((CHAR*)pExtent) + IXM_INDEX_CB_EXTENT_METADATA_SIZE,
                    newDef.objdata(), (size_t)newDef.objsize() ) ;
         _infoObj = BSONObj( ((const CHAR*)pExtent) +
                             IXM_INDEX_CB_EXTENT_METADATA_SIZE ) ;
         _fieldInitedFlag = 0 ;

         _idxUniqID = uniqueID ;
         SET_UNIQUEID_INITED() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "occur unexpected error(%s)", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      Local static variable define
   */
   static BSONObj s_idKeyObj = BSON( IXM_FIELD_NAME_KEY << BSON( DMS_ID_KEY_NAME << 1 ) <<
                                     IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME <<
                                     IXM_FIELD_NAME_UNIQUE << true <<
                                     IXM_FIELD_NAME_V << 0 <<
                                     IXM_FIELD_NAME_ENFORCED << true <<
                                     IXM_FIELD_NAME_NOTARRAY << true ) ;

   BSONObj ixmGetIDIndexDefine ()
   {
      return s_idKeyObj ;
   }

   BSONObj ixmGetIDIndexDefine( UINT64 idxUniqueID )
   {
      BSONObj obj ;

      try
      {
         if ( UTIL_UNIQUEID_NULL == idxUniqueID )
         {
            obj = s_idKeyObj ;
         }
         else
         {
            BSONObjBuilder builder ;
            builder.appendElements( s_idKeyObj ) ;
            builder.append( IXM_FIELD_NAME_UNIQUEID, (INT64)idxUniqueID ) ;
            obj = builder.obj() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return obj ;
   }

   /*
      Common function
   */
   const CHAR* ixmGetIndexFlagDesp(UINT16 indexFlag)
   {
      switch ( indexFlag )
      {
      case IXM_INDEX_FLAG_NORMAL :
         return "Normal" ;
         break ;
      case IXM_INDEX_FLAG_CREATING :
         return "Creating" ;
         break ;
      case IXM_INDEX_FLAG_DROPPING :
         return "Dropping" ;
         break ;
      case IXM_INDEX_FLAG_INVALID :
         return "Invalid" ;
         break ;
      case IXM_INDEX_FLAG_TRUNCATING :
         return "Truncating" ;
         break ;
      default :
         break ;
      }
      return "Unknown" ;
   }
}
