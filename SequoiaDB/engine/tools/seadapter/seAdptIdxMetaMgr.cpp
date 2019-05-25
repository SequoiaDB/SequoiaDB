/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = seAdptIdxMetaMgr.cpp

   Descriptive Name = Search Engine Adapter Index Metadata Manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/09/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptIdxMetaMgr.hpp"
#include "ossUtil.hpp"

namespace seadapter
{
   _seIndexMeta::_seIndexMeta()
   {
      _version = -1 ;
      _csLogicalID = 0 ;
      _clLogicalID = 0 ;
      _idxLogicalID = 0 ;
   }

   _seIndexMeta::_seIndexMeta( const CHAR *clFullName, const CHAR *idxName )
   {
      SDB_ASSERT( clFullName, "Collection name is NULL" ) ;
      SDB_ASSERT( idxName, "Index name is NULL" ) ;

      _version = -1 ;
      _origCLName = clFullName ;
      _origIdxName = idxName ;
      _clLogicalID = 0 ;
      _idxLogicalID = 0 ;
   }

   _seIndexMeta::~_seIndexMeta()
   {
   }

   void _seIndexMeta::setVersion( INT64 version )
   {
      _version = version ;
   }

   void _seIndexMeta::setCLName( const CHAR *clFullName )
   {
      SDB_ASSERT( clFullName, "Collection name is NULL" ) ;
      _origCLName = clFullName ;
   }

   void _seIndexMeta::setIdxName( const CHAR *idxName )
   {
      SDB_ASSERT( idxName, "Index name is NULL" ) ;
      _origIdxName = idxName ;
   }

   void _seIndexMeta::setCSLogicalID( UINT32 logicalID )
   {
      _csLogicalID = logicalID ;
   }

   void _seIndexMeta::setCLLogicalID( UINT32 logicalID )
   {
      _clLogicalID = logicalID ;
   }

   void _seIndexMeta::setIdxLogicalID( UINT32 logicalID )
   {
      _idxLogicalID = logicalID ;
   }

   void _seIndexMeta::setCappedCLName( const CHAR *cappedCLFullName )
   {
      SDB_ASSERT( cappedCLFullName, "Capped collection name is NULL" ) ;
      _cappedCLName = cappedCLFullName ;
   }

   INT32 _seIndexMeta::setIdxDef( BSONObj &idxDef )
   {
      INT32 rc = SDB_OK ;
      try
      {
         if ( idxDef.isEmpty() || !idxDef.isValid() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Index definition is invalid" ) ;
            goto error ;
         }
         _indexDef = idxDef.copy() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seIndexMeta::setESIdxName( const CHAR *idxName )
   {
      SDB_ASSERT( idxName, "Index name for ES is NULL" ) ;
      _esIdxName = idxName ;
   }

   void _seIndexMeta::setESIdxType( const CHAR *type )
   {
      SDB_ASSERT( type, "Index type is NULL" ) ;
      _esTypeName = type ;
   }

   INT32 _seIdxMetaMgr::lock( OSS_LATCH_MODE mode )
   {
      return ( SHARED == mode ) ? _lock.lock_r() : _lock.lock_w() ;
   }

   INT32 _seIdxMetaMgr::unlock( OSS_LATCH_MODE mode )
   {
      return ( SHARED == mode ) ? _lock.release_r() : _lock.release_w() ;
   }

   INT32 _seIdxMetaMgr::addIdxMeta( const CHAR *clFullName,
                                    const seIndexMeta &idxMeta )
   {
      SDB_ASSERT( clFullName, "Collection name should not be NULL" ) ;
      _idxMetaMap[ clFullName ].push_back( idxMeta ) ;

      return SDB_OK ;
   }

   INT32 _seIdxMetaMgr::rmIdxMeta( const CHAR *clFullName,
                                   const CHAR *idxName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN found = FALSE ;

      IDX_META_MAP_ITR itr = _idxMetaMap.find( clFullName ) ;
      if ( _idxMetaMap.end() == itr )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( IDX_META_VEC_ITR vecItr = itr->second.begin();
            vecItr != itr->second.end(); ++vecItr )
      {
         if ( 0 == ossStrcmp( vecItr->getOrigIdxName().c_str(), idxName ) )
         {
            itr->second.erase( vecItr ) ;
            found = TRUE ;
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Index[ %s ] of collection[ %s ] not found in "
                 "the cache", idxName, clFullName ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   seIndexMeta* _seIdxMetaMgr::getIdxMeta( const seIndexMeta& idxMeta )
   {
      seIndexMeta *meta = NULL ;
      IDX_META_MAP_ITR mItr ;
      const string& clFullName = idxMeta.getOrigCLName() ;

      mItr = _idxMetaMap.find( clFullName ) ;
      if ( _idxMetaMap.end() != mItr )
      {
         for ( IDX_META_VEC_ITR vItr = mItr->second.begin();
               vItr != mItr->second.end(); ++vItr )
         {
            if ( *vItr == idxMeta )
            {
               meta = &*vItr ;
               break ;
            }
         }
      }

      return meta ;
   }

   INT32 _seIdxMetaMgr::getIdxMetas( const CHAR *clFullName,
                                     IDX_META_VEC &idxMetaVec )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( clFullName, "Collection name is NULL" ) ;

      IDX_META_MAP_ITR itr = _idxMetaMap.find( clFullName ) ;
      if ( _idxMetaMap.end() != itr )
      {
         idxMetaVec = itr->second ;
      }
      else
      {
         rc = SDB_RTN_INDEX_NOTEXIST  ;
         PD_LOG( PDERROR, "No index found for collection[ %s ]", clFullName ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seIdxMetaMgr::cleanByVer( INT64 excludeVer )
   {
      for ( IDX_META_MAP_ITR mItr = _idxMetaMap.begin();
            mItr != _idxMetaMap.end(); ++mItr )
      {
         IDX_META_VEC &metaVec = mItr->second ;
         for ( IDX_META_VEC_ITR vItr = metaVec.begin();
               vItr != metaVec.end();  )
         {
            if ( vItr->getVersion() != excludeVer )
            {
               mItr->second.erase( vItr ) ;
            }
            else
            {
               ++vItr ;
            }
         }

         if ( 0 == metaVec.size() )
         {
            _idxMetaMap.erase( mItr ) ;
         }
      }
   }

   void _seIdxMetaMgr::clear()
   {
      _idxMetaMap.clear() ;
   }
}

