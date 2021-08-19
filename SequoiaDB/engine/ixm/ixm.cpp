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
#include "ixmExtent.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"

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
      _isInitialized = FALSE ;
      _pIndexSu = pIndexSu ;
      _pContext = context ;
      _extentID = extentID ;
      _pageSize = _pIndexSu->pageSize () ;
      _extent = (const ixmIndexCBExtent*)pIndexSu->beginFixedAddr( extentID,
                                                                   1 ) ;
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

      ixmIndexCBExtent *pExtent = NULL ;
      _isInitialized = FALSE ;
      dmsExtRW extRW ;
      _pIndexSu = pIndexSu ;
      _pContext = context ;
      _extentID = extentID ;
      _pageSize = _pIndexSu->pageSize() ;

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
      if ( !generateIndexType( infoObj, pExtent->_type ) )
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

   void _ixmIndexCB::scanExtLID ( UINT32 extLID )
   {
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      dmsExtRW extRW = _pIndexSu->extent2RW( _extentID,
                                             _pContext->mbID() ) ;
      ixmIndexCBExtent *pExtent = extRW.writePtr<ixmIndexCBExtent>() ;
      pExtent->_scanExtLID = extLID ;
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
                                          BSONObjSet &keys ) const
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( _isInitialized,
                   "index details must be initialized first" ) ;
      PD_TRACE_ENTRY ( SDB__IXMINXCB_GETKEY );
      ixmIndexKeyGen keyGen(this) ;
      rc = keyGen.getKeys ( obj, keys ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to generate key from object, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMINXCB_GETKEY, rc );
      return rc ;
   error :
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
   INT32 _ixmIndexCB::truncate ( BOOLEAN removeRoot, UINT16 indexFlag )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMINXCB_TRUNC );
      PD_TRACE1 ( SDB__IXMINXCB_TRUNC, PD_PACK_INT(removeRoot) ) ;

      setFlag ( IXM_INDEX_FLAG_TRUNCATING ) ;
      dmsExtentID root = getRoot() ;
      if ( DMS_INVALID_EXTENT != root )
      {
         BOOLEAN valid = TRUE ;
         ixmExtent rootExtent ( root, _pIndexSu ) ;
         rootExtent.truncate ( this, DMS_INVALID_EXTENT, valid ) ;
         if ( valid && removeRoot )
         {
            UINT16 mbID = rootExtent.getMBID() ;
            UINT16 freeSize = rootExtent.getFreeSize() ;
            // we need to set _totalIndexFreeSpace before freeExtent()
            _pIndexSu->decStatFreeSpace( mbID, freeSize ) ;
            rc = freeExtent ( root ) ;
            if ( rc )
            {
               _pIndexSu->addStatFreeSpace( mbID, freeSize ) ;
               PD_LOG ( PDERROR, "Failed to free extent %d", root ) ;
               goto error ;
            }
         }
      }
      setFlag ( indexFlag ) ;

   done :
      scanExtLID ( DMS_INVALID_EXTENT ) ;
      PD_TRACE_EXITRC ( SDB__IXMINXCB_TRUNC, rc );
      return rc ;
   error :
      setFlag ( IXM_INDEX_FLAG_INVALID ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXCB_ISSAMEDEF, "_ixmIndexCB::isSameDef" )
   BOOLEAN _ixmIndexCB::isSameDef( const BSONObj &defObj,
                                   BOOLEAN strict ) const
   {
      //PD_TRACE_ENTRY ( SDB__IXMINXCB_ISSAMEDEF );
      BOOLEAN rs = TRUE;
      SDB_ASSERT( TRUE == _isInitialized, "indexCB must be intialized!" );
      try
      {
         BSONElement beLKey = _infoObj.getField( IXM_KEY_FIELD );
         BSONElement beRKey = defObj.getField( IXM_KEY_FIELD );
         if ( 0 != beLKey.woCompare( beRKey, false ) )
         {
            rs = FALSE;
            goto done;
         }

         BOOLEAN lIsUnique = FALSE;
         BOOLEAN rIsUnique = FALSE;
         BSONElement beLUnique = _infoObj.getField( IXM_UNIQUE_FIELD );
         BSONElement beRUnique = defObj.getField( IXM_UNIQUE_FIELD );
         if ( beLUnique.booleanSafe() )
         {
            lIsUnique = TRUE;
         }
         if ( beRUnique.booleanSafe() )
         {
            rIsUnique = TRUE;
         }

         if ( !strict )
         {
            if ( lIsUnique )
            {
               /// it is useless to create any same defined index
               /// when an unique index exists.
               rs = TRUE ;
               goto done ;
            }
            else if ( lIsUnique != rIsUnique )
            {
               rs = FALSE;
               goto done;
            }
            else
            {
               /// do nothing.
            }
         }
         else
         {
            if ( lIsUnique != rIsUnique )
            {
                rs = FALSE;
                goto done ;
            }
         }

         BOOLEAN lEnforced = FALSE;
         BOOLEAN rEnforced = FALSE;
         BSONElement beLEnforced = _infoObj.getField( IXM_ENFORCED_FIELD );
         BSONElement beREnforced = defObj.getField( IXM_ENFORCED_FIELD );
         if ( beLEnforced.booleanSafe() )
         {
            lEnforced = TRUE;
         }
         if ( beREnforced.booleanSafe() )
         {
            rEnforced = TRUE;
         }

         if ( lEnforced != rEnforced )
         {
            rs = FALSE;
            goto done;
         }

         BOOLEAN lNotNull = FALSE;
         BOOLEAN rNotNull = FALSE;
         BSONElement beLNotNull = _infoObj.getField( IXM_NOTNULL_FIELD );
         BSONElement beRNotNull = defObj.getField( IXM_NOTNULL_FIELD );
         if ( beLNotNull.booleanSafe() )
         {
            lNotNull = TRUE;
         }
         if ( beRNotNull.booleanSafe() )
         {
            rNotNull = TRUE;
         }
         if ( lNotNull != rNotNull )
         {
            rs = FALSE;
            goto done;
         }
      }
      catch( std::exception &e )
      {
         rs = FALSE;
         PD_LOG( PDERROR, "occur unexpected error(%s)", e.what() );
      }

   done:
      //PD_TRACE_EXIT( SDB__IXMINXCB_ISSAMEDEF );
      return rs;
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

   /*
      Local static variable define
   */
   static BSONObj s_idKeyObj = BSON( IXM_FIELD_NAME_KEY << BSON( DMS_ID_KEY_NAME << 1 ) <<
                                     IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME <<
                                     IXM_FIELD_NAME_UNIQUE << true <<
                                     IXM_FIELD_NAME_V << 0 <<
                                     IXM_FIELD_NAME_ENFORCED << true ) ;

   BSONObj ixmGetIDIndexDefine ()
   {
      return s_idKeyObj ;
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
