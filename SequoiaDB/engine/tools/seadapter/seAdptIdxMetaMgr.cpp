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
#include "msgDef.hpp"
#include "seAdptMgr.hpp"

namespace seadapter
{
   _seIndexMeta::_seIndexMeta()
   {
      _id = SEADPT_INVALID_IMID ;
      reset() ;
   }

   _seIndexMeta::~_seIndexMeta()
   {
   }

   void _seIndexMeta::reset()
   {
      _prior = SEADPT_INVALID_IMID ;
      _next = SEADPT_INVALID_IMID ;
      _stat = SEADPT_IM_STAT_INVALID ;
      _version = -1 ;
      ossMemset( _origCLName, 0, SEADPT_MAX_CL_NAME_SZ + 1 ) ;
      ossMemset( _origIdxName, 0, SEADPT_MAX_DBIDX_NAME_SZ + 1 ) ;
      ossMemset( _cappedCLName, 0, SEADPT_MAX_CL_NAME_SZ + 1 ) ;
      ossMemset( _esIdxName, 0, SEADPT_MAX_IDXNAME_SZ + 1 ) ;
      ossMemset( _esTypeName, 0, SEADPT_MAX_TYPE_SZ + 1 ) ;
      _clUniqID = UTIL_UNIQUEID_NULL ;
      _clLogicalID = 0 ;
      _idxLogicalID = 0 ;
   }

   void _seIndexMeta::setVersion( INT64 version )
   {
      _version = version ;
   }

   void _seIndexMeta::setCLName( const CHAR *clFullName )
   {
      SDB_ASSERT( clFullName &&
                  ossStrlen( clFullName ) <= SEADPT_MAX_CL_NAME_SZ,
                  "Collection name is invalid" ) ;
      ossStrncpy( _origCLName, clFullName, SEADPT_MAX_CL_NAME_SZ ) ;
   }

   void _seIndexMeta::setIdxName( const CHAR *idxName )
   {
      SDB_ASSERT( idxName && ossStrlen( idxName ) <= SEADPT_MAX_DBIDX_NAME_SZ,
                  "Index name is invalid" ) ;
      ossStrncpy( _origIdxName, idxName, SEADPT_MAX_DBIDX_NAME_SZ ) ;
   }

   void _seIndexMeta::setCLUniqID( utilCLUniqueID clUniqID )
   {
      _clUniqID = clUniqID ;
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
      SDB_ASSERT( cappedCLFullName &&
                  ossStrlen( cappedCLFullName ) <= SEADPT_MAX_CL_NAME_SZ,
                  "Capped collection name is invalid" ) ;
      ossStrncpy( _cappedCLName, cappedCLFullName, SEADPT_MAX_CL_NAME_SZ ) ;
   }

   void _seIndexMeta::setESIdxName( const CHAR *esIdxName )
   {
      SDB_ASSERT( esIdxName && ossStrlen( _esIdxName ) <= SEADPT_MAX_IDXNAME_SZ,
                  "Index name on search engine is invalid" ) ;
      ossStrncpy( _esIdxName, esIdxName, SEADPT_MAX_IDXNAME_SZ ) ;
   }

   void _seIndexMeta::setESTypeName( const CHAR *esTypeName )
   {
      SDB_ASSERT( esTypeName && ossStrlen( esTypeName ) <= SEADPT_MAX_TYPE_SZ,
                  "Type name on search engine is invalid" ) ;
      ossStrncpy( _esTypeName, esTypeName, SEADPT_MAX_TYPE_SZ ) ;
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

   _seIndexMeta &_seIndexMeta::operator=( _seIndexMeta &right )
   {
      _version = right._version ;
      if ( 0 != ossStrcmp( _origCLName, right._origCLName ) )
      {
         ossStrncpy( _origCLName, right._origCLName, SEADPT_MAX_CL_NAME_SZ ) ;
      }
      if ( 0 != ossStrcmp( _origIdxName, right._origIdxName ) )
      {
         ossStrncpy( _origIdxName, right._origIdxName,
                     SEADPT_MAX_DBIDX_NAME_SZ ) ;
      }
      if ( 0 != ossStrcmp( _cappedCLName, right._cappedCLName ) )
      {
         ossStrncpy( _cappedCLName, right._cappedCLName,
                     SEADPT_MAX_CL_NAME_SZ ) ;
      }
      if ( 0 != ossStrcmp( _esIdxName, right._esIdxName ) )
      {
         ossStrncpy( _esIdxName, right._esIdxName, SEADPT_MAX_IDXNAME_SZ ) ;
      }
      if ( 0 != ossStrcmp( _esTypeName, right._esTypeName ) )
      {
         ossStrncpy( _esTypeName, right._esTypeName, SEADPT_MAX_TYPE_SZ ) ;
      }
      if ( _clUniqID != right._clUniqID )
      {
         _clUniqID = right._clUniqID ;
      }
      if ( _clLogicalID != right._clLogicalID )
      {
         _clLogicalID = right._clLogicalID ;
      }
      if ( _idxLogicalID != right._idxLogicalID )
      {
         _idxLogicalID = right._idxLogicalID ;
      }
      if ( 0 != _indexDef.woCompare( right._indexDef ) )
      {
         _indexDef = right._indexDef ;
      }

      return *this ;
   }

   _seIdxMetaContext::_seIdxMetaContext()
   {
      _reset() ;
   }

   _seIdxMetaContext::~_seIdxMetaContext()
   {

   }

   INT32 _seIdxMetaContext::pause()
   {
      INT32 rc = SDB_OK ;

      _resumeType = _lockType ;
      if ( SHARED == _lockType || EXCLUSIVE == _lockType )
      {
         rc = metaUnlock() ;
      }

      return rc ;
   }

   INT32 _seIdxMetaContext::resume()
   {
      INT32 rc = SDB_OK ;

      if ( SHARED == _resumeType || EXCLUSIVE == _resumeType )
      {
         INT32 lockType = _resumeType ;
         _resumeType = -1 ;
         rc = metaLock( lockType ) ;
      }

      return rc ;
   }

   void _seIdxMetaContext::_reset()
   {
      _meta = NULL ;
      _clUniqID = UTIL_UNIQUEID_NULL ;
      _clLogicalID = 0 ;
      _idxLogicalID = 0 ;
      _lockType = -1 ;
      _resumeType = -1 ;
   }

   _seIdxMetaMgr::_seIdxMetaMgr()
   {
      _version = -1 ;
      ossMemset( _typeName, 0, SEADPT_MAX_TYPE_SZ + 1 ) ;

      for ( UINT16 i = 0; i < SEADPT_MAX_IDX_NUM; ++i )
      {
         _metas[i].setID( i ) ;
      }
   }

   _seIdxMetaMgr::~_seIdxMetaMgr()
   {
      for ( vector<seIdxMetaContext *>::const_iterator itr = _vecContext.begin();
            itr != _vecContext.end(); ++itr )
      {
         if ( *itr )
         {
            SDB_OSS_DEL *itr ;
         }
      }
   }

   void _seIdxMetaMgr::setFixTypeName( const CHAR *type )
   {
      SDB_ASSERT( type && ossStrlen( type ) <= SEADPT_MAX_TYPE_SZ,
                  "Type name is invalid" ) ;
      ossStrncpy( _typeName, type, SEADPT_MAX_TYPE_SZ + 1 ) ;
   }

   INT32 _seIdxMetaMgr::lock( OSS_LATCH_MODE mode )
   {
      return ( SHARED == mode ) ? _lock.lock_r() : _lock.lock_w() ;
   }

   INT32 _seIdxMetaMgr::unlock( OSS_LATCH_MODE mode )
   {
      return ( SHARED == mode ) ? _lock.release_r() : _lock.release_w() ;
   }

   void _seIdxMetaMgr::clearObsoleteMeta()
   {
      for ( UINT16 i = 0; i < SEADPT_MAX_IDX_NUM; ++i )
      {
         seIndexMeta *meta = &_metas[ i ] ;
         if ( -1 != meta->getVersion() && meta->getVersion() != _version )
         {
            rmIdxMeta( i ) ;
         }
      }
   }

   INT32 _seIdxMetaMgr::addIdxMeta( const BSONObj &idxMeta, UINT16 *imID,
                                    BOOLEAN replace )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      utilCLUniqueID clUID = UTIL_UNIQUEID_NULL ;
      UINT16 idxMetaID = SEADPT_INVALID_IMID ;

      try
      {
         // 1. Get collection unique id.
         ele = idxMeta.getField( FIELD_NAME_UNIQUEID ) ;
         if ( ele.eoo() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Collection unique ID not found in index meta: %s",
                    idxMeta.toString(false, true).c_str() ) ;
            goto error ;
         }

         clUID = (utilCLUniqueID)ele.numberLong() ;
         // 2. Check if the collection exists.
         idxMetaID = _clUIDLookup( clUID ) ;
         if ( SEADPT_INVALID_IMID == idxMetaID )
         {
            // Add new index metadata.
            rc = _addIdxMeta( clUID, idxMeta, &idxMetaID ) ;
            PD_RC_CHECK( rc, PDERROR, "Add index metadata failed[%d]", rc ) ;
         }
         else if ( replace )
         {
            // Update existing index metadata.
            rc = _updateIdxMeta( idxMetaID, idxMeta ) ;
            PD_RC_CHECK( rc, PDERROR, "Update index metadata failed[%d]", rc ) ;
         }
         else
         {
            // do nothing, just ignore
         }
         if ( ( SEADPT_INVALID_IMID != idxMetaID ) && imID )
         {
            *imID = idxMetaID ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seIdxMetaMgr::rmIdxMeta( UINT16 imID )
   {
      if ( imID < SEADPT_MAX_IDX_NUM )
      {
         seIndexMeta *meta = &_metas[ imID ] ;
         UINT16 priorID = meta->prior() ;
         PD_LOG( PDDEBUG, "Remove index meta[%s]", meta->toString().c_str() ) ;

         if ( SEADPT_INVALID_IMID == priorID )
         {
            ossLatch( &_mapLatch, EXCLUSIVE ) ;
            UINT16 nextIMID = meta->next() ;
            if ( SEADPT_INVALID_IMID == nextIMID )
            {
               // If this is the only index meta for the collection, remove the
               // item from the id map. For the name map, only delete it when it
               // it still pointing to this item.
               _clUIDMap.erase( meta->getCLUID() ) ;
               // In case of renaming CS/CL, the name will pointer to the new
               // index meta. So we can not remove it.
               CLNAME_MAP::iterator itr =
                     _clNameMap.find( meta->getOrigCLName() ) ;
               if ( itr->second == meta->getID() )
               {
                  _clNameMap.erase( meta->getOrigCLName() ) ;
               }
            }
            else
            {
               // If it's the first, but not the last meta item,
               _clNameMap.erase( meta->getOrigCLName() ) ;
               seIndexMeta *metaNext = &_metas[ nextIMID ] ;
               metaNext->setPrior( SEADPT_INVALID_IMID ) ;
               _clUIDMap[ metaNext->getCLUID() ] = nextIMID ;
               _clNameMap[ metaNext->getOrigCLName() ] = nextIMID ;
            }
            ossUnlatch( &_mapLatch, EXCLUSIVE ) ;
         }
         else
         {
            seIndexMeta *prior = &_metas[ priorID ] ;
            prior->setNext( meta->next() ) ;
         }

         meta->lock( EXCLUSIVE ) ;
         meta->reset() ;
         meta->unlock( EXCLUSIVE ) ;
      }
   }

   void _seIdxMetaMgr::getIdxNamesByCL( const CHAR *clName,
                                        map<string, UINT16> &idxNameMap )
   {
      seIndexMeta *meta = NULL ;

      ossLatch( &_mapLatch, SHARED ) ;
      CLNAME_MAP_CIT itr = _clNameMap.find( clName ) ;
      if ( itr != _clNameMap.end() )
      {
         UINT16 imID = itr->second ;
         while ( SEADPT_INVALID_IMID != imID )
         {
            meta = &_metas[ imID ] ;
            if ( SEADPT_IM_STAT_INVALID != meta->getStat() )
            {
               idxNameMap[ meta->getOrigIdxName() ] = imID ;
            }
            imID = meta->next() ;
         }
      }

      ossUnlatch( &_mapLatch, SHARED ) ;
   }

   void _seIdxMetaMgr::getCurrentIMIDs( vector<UINT16> &imIDs )
   {
      for ( UINT16 i = 0; i < SEADPT_MAX_IDX_NUM; ++i )
      {
         if ( _metas[ i ].inUse() )
         {
            imIDs.push_back( i ) ;
         }
      }
   }

   INT32 _seIdxMetaMgr::getIMContext( seIdxMetaContext **context, UINT16 imID,
                                      INT32 lockType )
   {
      INT32 rc = SDB_OK ;
      seIndexMeta *meta = NULL ;
      if ( imID >= SEADPT_MAX_IDX_NUM )
      {
         return SDB_INVALIDARG ;
      }

      // Allocate a context.
      _vecCtxLatch.get() ;
      if ( _vecContext.size() > 0 )
      {
         *context = _vecContext.back() ;
         _vecContext.pop_back() ;
      }
      else
      {
         *context = SDB_OSS_NEW seIdxMetaContext ;
      }
      _vecCtxLatch.release() ;

      if ( !(*context) )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      meta = &_metas[ imID ] ;

      (*context)->_meta = &_metas[ imID ] ;
      (*context)->_clUniqID = meta->getCLUID() ;
      (*context)->_clLogicalID = meta->getCLLID() ;
      (*context)->_idxLogicalID = meta->getIdxLID() ;
      if ( SHARED == lockType || EXCLUSIVE == lockType )
      {
         rc = (*context)->metaLock( lockType ) ;
         if ( rc )
         {
            releaseIMContext( *context ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT16 _seIdxMetaMgr::_clUIDLookup( utilCLUniqueID clUID )
   {
      UINT16 idxMetaID = SEADPT_INVALID_IMID ;
      ossLatch( &_mapLatch, SHARED ) ;
      CLUID_MAP_CIT itr = _clUIDMap.find( clUID ) ;
      if ( itr != _clUIDMap.end() )
      {
         idxMetaID = itr->second ;
      }
      ossUnlatch( &_mapLatch, SHARED ) ;

      return idxMetaID ;
   }

   UINT16 _seIdxMetaMgr::_clNameLookup( const CHAR *name )
   {
      UINT16 idxMetaID = SEADPT_INVALID_IMID ;
      ossLatch( &_mapLatch, SHARED ) ;
      CLNAME_MAP_CIT itr = _clNameMap.find( name ) ;
      if ( itr != _clNameMap.end() )
      {
         idxMetaID = itr->second ;
      }
      ossUnlatch( &_mapLatch, SHARED ) ;

      return idxMetaID ;
   }

   void _seIdxMetaMgr::_clUIDInsert( utilCLUniqueID clUID, UINT16 imID )
   {
      _clUIDMap[ clUID ] = imID ;
   }

   void _seIdxMetaMgr::_clNameInsert( const CHAR *name, UINT16 imID )
   {
      SDB_ASSERT( name && ossStrlen(name) > 0, "Name is invalid" ) ;
      _clNameMap[ name ] = imID ;
   }

   INT32
   _seIdxMetaMgr::_addIdxMeta( utilCLUniqueID clUID, const BSONObj &idxMeta,
                               UINT16 *idxMetaID )
   {
      INT32 rc = SDB_OK ;
      UINT16 imID = SEADPT_INVALID_IMID ;
      seIndexMeta *meta = NULL ;
      seIndexMeta metaTmp ;

      rc = _parseIdxMetaData( idxMeta, metaTmp ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse index meta data failed[%d]. Meta: %s",
                   rc, idxMeta.toString(false, true).c_str() ) ;

      // 1. Find a free slot for the metadata.
      // 2. Update information in the slot. The slot can not be used by other
      //    threads, as it can not be found in the id map. So it's safe to
      //    modify it out of any lock.
      // 3. Insert unique id into the UID map with metadata lock protection.

      for ( UINT16 i = 0; i < SEADPT_MAX_IDX_NUM; ++i )
      {
         if ( !_metas[i].inUse() )
         {
            imID = i ;
            meta = &_metas[ imID ] ;
            break ;
         }
      }

      if ( !meta )
      {
         PD_LOG( PDERROR, "Unable to find free index meta id" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      *meta = metaTmp ;
      meta->setStat( SEADPT_IM_STAT_PENDING ) ;

      // Once we insert the unique id into the map, and release the metadata
      // lock, the index meta can be found by any session.
      ossLatch( &_mapLatch, EXCLUSIVE ) ;
      _clUIDInsert( clUID, imID ) ;
      _clNameInsert( _metas[ imID ].getOrigCLName(), imID ) ;
      ossUnlatch( &_mapLatch, EXCLUSIVE ) ;

      if ( idxMetaID )
      {
         *idxMetaID = imID ;
      }

      PD_LOG( PDDEBUG, "Add new meta infor: %s", meta->toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seIdxMetaMgr::_parseIdxMetaData( const BSONObj &indexInfo,
                                           seIndexMeta &meta )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj::iterator itr( indexInfo ) ;

         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_COLLECTION ) )
            {
               meta.setCLName( ele.valuestrsafe() ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      SEADPT_FIELD_NAME_CAPPEDCL ) )
            {
               meta.setCappedCLName( ele.valuestrsafe() ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEX ) )
            {
               BSONObj idxDef = ele.Obj() ;
               BSONObj key ;
               if ( idxDef.isEmpty() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "No valid index definition in index info" ) ;
                  goto error ;
               }

               key = idxDef.getObjectField( IXM_FIELD_NAME_KEY ) ;
               if ( key.isEmpty() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "No valid key definition in index info" ) ;
                  goto error ;
               }
               meta.setIdxDef( key ) ;
               meta.setIdxName( idxDef.getStringField( IXM_FIELD_NAME_NAME ) ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_UNIQUEID ) )
            {
               meta.setCLUniqID( (utilCLUniqueID)ele.numberLong() ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_LOGICAL_ID ) )
            {
               meta.setCLLogicalID( (UINT32)ele.numberInt() ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEXLID ) )
            {
               meta.setIdxLogicalID( (UINT32)ele.numberInt() ) ;
            }
            else
            {
               PD_LOG( PDWARNING, "Ignore unknown field[%s]",
                       ele.toString(false, true).c_str() ) ;
            }
         }

         {
            // ES index name is in the format of [prefix]cappedCLName_groupName.
            const CHAR *dot = ossStrchr( meta.getCappedCLName(), '.' ) ;
            SDB_ASSERT( dot, "No dot found in the capped collection full name" ) ;
            const CHAR *cappedCLName = dot + 1 ;
            const CHAR *idxPrefix = sdbGetSeAdptOptions()->getSEIdxPrefix() ;
            // From ES6.0, one index can contain only one type. So we need to append
            // the group name to the ES index name, to handle index data splited to
            // more than one group.
            std::string esIdx =
                  (( 0 == ossStrlen(idxPrefix) ) ? "" : std::string(idxPrefix))
                  +  std::string(cappedCLName) + "_" + _typeName ;
            // ES index names should be in lower case.
            std::transform( esIdx.begin(), esIdx.end(), esIdx.begin(), ::tolower ) ;
            meta.setESIdxName( esIdx.c_str() ) ;
            meta.setESTypeName( _typeName ) ;
            meta.setVersion( _version ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happed: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32
   _seIdxMetaMgr::_updateIdxMeta( UINT16 idxMetaID, const BSONObj &idxMeta )
   {
      INT32 rc = SDB_OK ;
      seIndexMeta *meta = NULL ;
      utilCLUniqueID clUID = UTIL_UNIQUEID_NULL ;
      UINT16 lastIMID = SEADPT_INVALID_IMID ;
      UINT16 newIMID = SEADPT_INVALID_IMID ;
      CHAR oldCLName[ SEADPT_MAX_CL_NAME_SZ + 1 ] = { 0 } ;
      seIndexMeta metaTmp ;

      rc = _parseIdxMetaData( idxMeta, metaTmp ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse index meta data failed[%d]. Meta: %s",
                   rc, idxMeta.toString(false, true).c_str() ) ;

      // Loop and find if the meta data of the index exists already.
      while ( SEADPT_INVALID_IMID != idxMetaID )
      {
         lastIMID = idxMetaID ;
         meta = &_metas[ idxMetaID ] ;
         if ( 0 == ossStrcmp( meta->getOrigIdxName(),
                               metaTmp.getOrigIdxName() ) )
         {
            break ;
         }

         idxMetaID = meta->next() ;
      }

      if ( SEADPT_INVALID_IMID == idxMetaID )
      {
         rc = _addIdxMeta( clUID, idxMeta, &newIMID ) ;
         PD_RC_CHECK( rc, PDERROR, "Add new index meta failed[%d]", rc ) ;
         SDB_ASSERT( meta, "Current last meta item is invalid" ) ;
         meta->setNext( newIMID ) ;
         _metas[ newIMID ].setPrior( lastIMID ) ;
      }
      else
      {
         BOOLEAN newIndex = FALSE ;
         ossStrncpy( oldCLName, meta->getOrigCLName(),
                     SEADPT_MAX_CL_NAME_SZ ) ;

         meta->lock( EXCLUSIVE ) ;
         if ( meta->getCLUID() != metaTmp.getCLUID() ||
              meta->getCLLID() != metaTmp.getCLLID() ||
              meta->getIdxLID() != metaTmp.getIdxLID() )
         {
            newIndex = TRUE ;
         }

         *meta = metaTmp ;
         if ( newIndex )
         {
            meta->setStat( SEADPT_IM_STAT_PENDING ) ;
         }
         meta->unlock( EXCLUSIVE ) ;
         // If the collection name changed, need update the name map.
         if ( 0 != ossStrcmp( oldCLName, meta->getOrigCLName() ) )
         {
            ossLatch( &_mapLatch, EXCLUSIVE ) ;
            _clNameMap.erase( oldCLName ) ;
            _clNameMap[ meta->getOrigCLName() ] = idxMetaID ;
            PD_LOG( PDDEBUG, "Remove old name[%s] and insert new name[%s] with "
                             "location %u",
                    oldCLName, meta->getOrigCLName(), idxMetaID ) ;
            ossUnlatch( &_mapLatch, EXCLUSIVE ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _seIdxMetaMgr::printAllIdxMetas()
   {
      for ( INT32 i = 0; i < SEADPT_MAX_IDX_NUM; ++i )
      {
         seIndexMeta *meta = &_metas[i] ;
         meta->lock( SHARED ) ;
         if ( SEADPT_IM_STAT_NORMAL == meta->getStat() ||
              SEADPT_IM_STAT_PENDING == meta->getStat() )
         {
            PD_LOG( PDDEBUG, "Index meta: %s", meta->toString().c_str() ) ;
         }
         meta->unlock( SHARED ) ;
      }
   }

}

