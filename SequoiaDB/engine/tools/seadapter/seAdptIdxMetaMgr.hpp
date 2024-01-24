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

   Source File Name = seAdptIdxMetaMgr.hpp

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
#ifndef SEADPT_IDXMETAMGR_HPP__
#define SEADPT_IDXMETAMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include "ossRWMutex.hpp"
#include "utilUniqueID.hpp"
#include "seAdptDef.hpp"

#include <string>

using bson::BSONObj ;
using namespace engine ;

namespace seadapter
{
   #define SEADPT_MAX_CL_NAME_SZ          255
   #define SEADPT_MAX_DBIDX_NAME_SZ       127
   #define SEADPT_INVALID_IMID            65535
   #define SEADPT_MAX_CTX_NUM             1000
   #define SEADPT_INVALID_IMID            65535

   enum _seIdxMetaStat
   {
      SEADPT_IM_STAT_INVALID = 0,
      // The index meta is valid, but not processed by an indexing session.
      SEADPT_IM_STAT_PENDING,
      // The index meta is valid, and is processing by an indexing session.
      SEADPT_IM_STAT_NORMAL
   } ;
   typedef _seIdxMetaStat seIdxMetaStat ;

   class _seIndexMeta : public SDBObject
   {
      public:
         _seIndexMeta() ;
         ~_seIndexMeta() ;

         void lock( OSS_LATCH_MODE mode )
         {
            ossLatch( &_latch, mode) ;
         }

         void unlock( OSS_LATCH_MODE mode )
         {
            ossUnlatch( &_latch, mode ) ;
         }

         void setID( UINT16 id )
         {
            _id = id ;
         }

         UINT16 getID() const
         {
            return _id ;
         }

         void setPrior( UINT16 prior )
         {
            SDB_ASSERT( prior != _id, "Self and prior are equal" ) ;
            _prior = prior ;
         }

         UINT16 prior() const
         {
            return _prior ;
         }

         void setNext( UINT16 next )
         {
            SDB_ASSERT( next != _id, "Self and next are equal" ) ;
            _next = next ;
         }

         UINT16 next() const
         {
            return _next ;
         }

         void setStat( seIdxMetaStat stat )
         {
            _stat = stat ;
         }

         seIdxMetaStat getStat() const
         {
            return _stat ;
         }

         BOOLEAN inUse() const
         {
            return ( SEADPT_IM_STAT_PENDING == _stat ||
                     SEADPT_IM_STAT_NORMAL == _stat ) ;
         }

         BOOLEAN isMe( utilCLUniqueID origCLUID, UINT32 origCLLID,
                       UINT32 idxLID ) const
         {
            return ( origCLUID == _clUniqID
                     && origCLLID == _clLogicalID
                     && idxLID == _idxLogicalID ) ;
         }

         void reset() ;

         void setVersion( INT64 version ) ;
         INT64 getVersion() const
         {
            return _version ;
         }

         void setCLName( const CHAR *clFullName ) ;
         void setIdxName( const CHAR *idxName ) ;
         void setCLUniqID( utilCLUniqueID clUniqID ) ;
         void setCLLogicalID( UINT32 logicalID ) ;
         void setIdxLogicalID( UINT32 logicalID ) ;
         void setCappedCLName( const CHAR *cappedCLFullName ) ;
         void setESIdxName( const CHAR *esIdxName ) ;
         void setESTypeName( const CHAR *esTypeName ) ;
         INT32 setIdxDef( BSONObj &idxDef ) ;


         const CHAR* getOrigCLName() const
         {
            return _origCLName ;
         }

         const CHAR* getCappedCLName() const
         {
            return _cappedCLName ;
         }

         const CHAR* getOrigIdxName() const
         {
            return _origIdxName ;
         }

         const CHAR* getESIdxName() const
         {
            return _esIdxName ;
         }

         const CHAR* getESTypeName() const
         {
            return _esTypeName ;
         }

         const BSONObj& getIdxDef() const
         {
            return _indexDef ;
         }

         utilCLUniqueID getCLUID() const
         {
            return _clUniqID ;
         }

         UINT32 getCLLID() const
         {
            return _clLogicalID ;
         }

         UINT32 getIdxLID() const
         {
            return _idxLogicalID ;
         }

         _seIndexMeta& operator=( _seIndexMeta &right ) ;

         std::string toString() const
         {
            try
            {
               std::stringstream ss ;
               ss << "id[" << _id << "], "
                  << "prior[" << _prior << "], "
                  << "next[" << _next << "], "
                  << "stat[" << _stat << "], "
                  << "Version[" << _version << "], "
                  << "original cl[" << _origCLName << "], "
                  << "capped cl[" << _cappedCLName << "], "
                  << "CL unique id[" << _clUniqID << "], "
                  << "CL logical id[" << _clLogicalID << "], "
                  << "es index[" << _esIdxName << "], "
                  << "es type[" << _esTypeName << "], "
                  << "original index[" << _origIdxName << ", "
                  << _indexDef.toString() << "], "
                  << "original index logicalID[" << _idxLogicalID << "]" ;
               return ss.str() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Unexpected exception happened: %s",
                       e.what() ) ;
               return "" ;
            }
         }

      private:
         UINT16         _id ;
         UINT16         _prior ;
         UINT16         _next ;
         seIdxMetaStat  _stat ;
         ossSpinSLatch  _latch ;    // Latch for reading/writting index meta.

         // Index information version.
         INT64          _version ;

         CHAR           _origCLName[ SEADPT_MAX_CL_NAME_SZ + 1 ] ;
         CHAR           _origIdxName[ SEADPT_MAX_DBIDX_NAME_SZ + 1 ] ;
         CHAR           _cappedCLName[ SEADPT_MAX_CL_NAME_SZ + 1 ] ;
         CHAR           _esIdxName[ SEADPT_MAX_IDXNAME_SZ + 1 ] ;
         CHAR           _esTypeName[ SEADPT_MAX_TYPE_SZ + 1 ] ;
         utilCLUniqueID _clUniqID ;
         UINT32         _clLogicalID ;
         UINT32         _idxLogicalID ;
         BSONObj        _indexDef ;  // Used for fetching data from original collection.
   } ;
   typedef _seIndexMeta seIndexMeta ;

   /**
    * @brief Context for the text index metadata. User should hold the meta by
    * context interface before using it.
    */
   class _seIdxMetaContext : public SDBObject
   {
      friend class _seIdxMetaMgr ;
   public:
      _seIdxMetaContext() ;
      ~_seIdxMetaContext() ;

      OSS_INLINE INT32 metaLock( INT32 lockType ) ;
      OSS_INLINE INT32 metaUnlock() ;
      OSS_INLINE BOOLEAN isMetaLocked() const ;
      INT32 pause() ;
      INT32 resume() ;

      seIndexMeta* meta() const
      {
         return _meta ;
      }

      utilCLUniqueID getCLUID() const
      {
         return _clUniqID ;
      }

      UINT32 getCLLID() const
      {
         return _clLogicalID ;
      }

      UINT32 getIdxLID() const
      {
         return _idxLogicalID ;
      }

   private:
      void _reset() ;

   private:
      seIndexMeta   *_meta ;
      utilCLUniqueID _clUniqID ;
      UINT32         _clLogicalID ;
      UINT32         _idxLogicalID ;
      INT32          _lockType ;
      INT32          _resumeType ;
   } ;
   typedef _seIdxMetaContext seIdxMetaContext ;

   OSS_INLINE INT32 _seIdxMetaContext::metaLock( INT32 lockType )
   {
      INT32 rc = SDB_OK ;

      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         return SDB_INVALIDARG ;
      }

      if ( _lockType == lockType )
      {
         // Has taken the lock of the same type already.
         return SDB_OK ;
      }

      // Already taken lock of different type, need to unlock first.
      if ( -1 != _lockType && ( SDB_OK != (rc = pause() ) ) )
      {
         return rc ;
      }

      // Check before the lock. If we can be sure the meta is not what we want,
      // it can avoid the cost of the lock.
      if ( !_meta->inUse() ||
           !_meta->isMe( _clUniqID, _clLogicalID, _idxLogicalID ) )
      {
         PD_LOG( PDDEBUG, "Meta is not in use or not me: %s. My CL unique "
                          "id: %llu, CL LID: %u, IDX LID: %u",
                 _meta->toString().c_str(), _clUniqID,
                 _clLogicalID, _idxLogicalID ) ;
         return SDB_RTN_INDEX_NOTEXIST ;
      }

      // Lock the meta, and check the data.
      _meta->lock( (OSS_LATCH_MODE)lockType ) ;

      // Double check after lock.
      if ( !_meta->inUse() ||
           !_meta->isMe( _clUniqID, _clLogicalID, _idxLogicalID ) )
      {
         PD_LOG( PDDEBUG, "Meta is not in use or not me: %s. My CL unique "
                          "id: %llu, CL LID: %u, IDX LID: %u",
                 _meta->toString().c_str(), _clUniqID,
                 _clLogicalID, _idxLogicalID ) ;
         _meta->unlock( (OSS_LATCH_MODE)_lockType ) ;
         return SDB_RTN_INDEX_NOTEXIST ;
      }

      _lockType = lockType ;
      _resumeType = -1 ;
      return SDB_OK ;
   }

   OSS_INLINE INT32 _seIdxMetaContext::metaUnlock()
   {
      if ( SHARED == _lockType || EXCLUSIVE == _lockType )
      {
         _meta->unlock( (OSS_LATCH_MODE)_lockType ) ;
         _resumeType = _lockType ;
         _lockType = -1 ;
      }
      return SDB_OK ;
   }

   OSS_INLINE BOOLEAN _seIdxMetaContext::isMetaLocked() const
   {
      return ( SHARED == _lockType || EXCLUSIVE == _lockType ) ;
   }

   // Text index metadata manager.
   class _seIdxMetaMgr : public SDBObject
   {
      struct comp_str
      {
         bool operator()( const string& left, const string& right)
         {
            return ossStrcmp( left.c_str(), right.c_str() ) < 0 ;
         }
      } ;
      typedef map<utilCLUniqueID, UINT16> CLUID_MAP ;
      typedef CLUID_MAP::const_iterator CLUID_MAP_CIT ;

      typedef map<string, UINT16, comp_str> CLNAME_MAP ;
      typedef CLNAME_MAP::const_iterator CLNAME_MAP_CIT ;

   public:
      _seIdxMetaMgr() ;
      ~_seIdxMetaMgr() ;

      void setFixTypeName( const CHAR *type ) ;

      // In concurrent operation, always use lock to protect.
      INT32 lock( OSS_LATCH_MODE mode = SHARED ) ;
      INT32 unlock( OSS_LATCH_MODE mode = SHARED ) ;

      void setMetaVersion( INT64 version )
      {
         _version = version ;
      }

      INT64 getMetaVersion() const
      {
         return _version ;
      }

      /**
       * @brief Add metadata of a text index into the metadata manager.
       * @param idxMeta Index metadata object.
       * @param replace Whether replace if the same index metadata exists.
       */
      INT32 addIdxMeta( const BSONObj &idxMeta, UINT16 *imID = NULL,
                        BOOLEAN replace = TRUE ) ;

      void rmIdxMeta( UINT16 imID ) ;

      void getIdxNamesByCL( const CHAR *clName,
                            map<string, UINT16> &idxNameMap ) ;

      void getCurrentIMIDs( vector<UINT16> &imIDs ) ;

      /**
       * Clean all index meta whose version is less than the current.
       */
      void clearObsoleteMeta() ;

      INT32 getIMContext( seIdxMetaContext **context, UINT16 imID,
                          INT32 lockType = -1 ) ;
      OSS_INLINE void releaseIMContext( seIdxMetaContext *&context ) ;

      // for debug
      void   printAllIdxMetas() ;

   private:
      UINT16 _clUIDLookup( utilCLUniqueID clUID ) ;
      UINT16 _clNameLookup( const CHAR *name ) ;
      void   _clUIDInsert( utilCLUniqueID clUID, UINT16 imID ) ;
      void   _clNameInsert( const CHAR *name, UINT16 imID ) ;
      INT32  _addIdxMeta( utilCLUniqueID clUID, const BSONObj &idxMeta,
                          UINT16 *idxMetaID = NULL ) ;
      INT32  _parseIdxMetaData( const BSONObj &indexInfo, seIndexMeta &meta ) ;
      INT32  _updateIdxMeta( UINT16 idxMetaID, const BSONObj &idxMeta ) ;

   private:
      INT64         _version ;
      CHAR          _typeName[ SEADPT_MAX_TYPE_SZ + 1 ] ;
      seIndexMeta   _metas[ SEADPT_MAX_IDX_NUM ] ;

      ossSpinSLatch _mapLatch ;
      CLUID_MAP     _clUIDMap ;
      CLNAME_MAP    _clNameMap ;

      ossRWMutex    _lock ;

      vector<seIdxMetaContext *>    _vecContext ;
      ossSpinXLatch _vecCtxLatch ;
   } ;
   typedef _seIdxMetaMgr seIdxMetaMgr ;

   OSS_INLINE
   void _seIdxMetaMgr::releaseIMContext( seIdxMetaContext *&context )
   {
      if ( !context )
      {
         return ;
      }

      context->metaUnlock() ;

      _vecCtxLatch.get() ;
      if ( _vecContext.size() < SEADPT_MAX_CTX_NUM )
      {
         context->_reset() ;
         _vecContext.push_back( context ) ;
      }
      else
      {
         SDB_OSS_DEL context ;
      }

      _vecCtxLatch.release() ;
      context = NULL ;
   }
}

#endif /* SEADPT_IDXMETAMGR_HPP__ */

