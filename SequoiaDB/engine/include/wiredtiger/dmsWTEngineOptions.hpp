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

   Source File Name = dmsWTEngineOptions.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_ENGINE_OPTIONS_HPP_
#define DMS_WT_ENGINE_OPTIONS_HPP_

#include "dmsDef.hpp"
#include "wiredtiger/dmsWTDef.hpp"
#include "utilPooledObject.hpp"
#include <boost/filesystem/path.hpp>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTEngineOptions define
    */
   class _dmsWTEngineOptions : public _utilPooledObject
   {
   public:
      _dmsWTEngineOptions() = default ;
      virtual ~_dmsWTEngineOptions() = default ;
      _dmsWTEngineOptions( const _dmsWTEngineOptions &o ) = default ;
      _dmsWTEngineOptions &operator =( const _dmsWTEngineOptions & ) = default ;

   public:
      const boost::filesystem::path &getDBPath() const
      {
         return _dbPath ;
      }

      void setDBPath( const boost::filesystem::path &dbPath )
      {
         _dbPath = dbPath ;
      }

      UINT32 getCacheSizeMB() const
      {
         return _cacheSizeMB ;
      }

      void setCacheSizeMB( UINT32 cacheSizeMB )
      {
         _cacheSizeMB = cacheSizeMB ;
      }

      UINT32 getEvictTarget() const
      {
         return _evictTarget ;
      }

      void setEvictTarget( UINT32 evictTarget )
      {
         _evictTarget = evictTarget ;
      }

      UINT32 getEvictTrigger() const
      {
         return _evictTrigger ;
      }

      void setEvictTrigger( UINT32 evictTrigger )
      {
         _evictTrigger = evictTrigger ;
      }

      UINT32 getEvictDirtyTarget() const
      {
         return _evictDirtyTarget ;
      }

      void setEvictDirtyTarget( UINT32 evictDirtyTarget )
      {
         _evictDirtyTarget = evictDirtyTarget ;
      }

      UINT32 getEvictDirtyTrigger() const
      {
         return _evictDirtyTrigger ;
      }

      void setEvictDirtyTrigger( UINT32 evictDirtyTrigger )
      {
         _evictDirtyTrigger = evictDirtyTrigger ;
      }

      UINT32 getEvictUpdatesTarget() const
      {
         return _evictUpdatesTarget ;
      }

      void setEvictUpdatesTarget( UINT32 evictUpdatesTarget )
      {
         _evictUpdatesTarget = evictUpdatesTarget ;
      }

      UINT32 getEvictUpdatesTrigger() const
      {
         return _evictUpdatesTrigger ;
      }

      void setEvictUpdatesTrigger( UINT32 evictUpdatesTrigger )
      {
         _evictUpdatesTrigger = evictUpdatesTrigger ;
      }

      UINT32 getEvictThreadsMin() const
      {
         return _evictThreadsMin ;
      }

      void setEvictThreadsMin( UINT32 evictThreadsMin )
      {
         _evictThreadsMin = evictThreadsMin ;
      }

      UINT32 getEvictThreadsMax() const
      {
         return _evictThreadsMax ;
      }

      void setEvictThreadsMax( UINT32 evictThreadsMax )
      {
         _evictThreadsMax = evictThreadsMax ;
      }

      UINT32 getCheckPointInterval() const
      {
         return _checkPointInterval ;
      }

      void setCheckPointInterval( UINT32 checkPointInterval )
      {
         _checkPointInterval = checkPointInterval ;
      }

      void fixOptions()
      {
         if ( _evictTarget > _evictTrigger )
         {
            _evictTarget = _evictTrigger ;
         }
         if ( _evictDirtyTarget > _evictDirtyTrigger )
         {
            _evictDirtyTarget = _evictDirtyTrigger ;
         }
         if ( _evictDirtyTarget > _evictTarget )
         {
            _evictDirtyTarget = _evictTarget ;
         }
         if ( _evictDirtyTrigger > _evictTrigger )
         {
            _evictDirtyTrigger = _evictTrigger ;
         }
         if ( _evictUpdatesTarget > _evictUpdatesTrigger )
         {
            _evictUpdatesTarget = _evictUpdatesTrigger ;
         }
         if ( _evictUpdatesTarget > _evictDirtyTarget )
         {
            _evictUpdatesTarget = _evictDirtyTarget ;
         }
         if ( _evictUpdatesTrigger > _evictDirtyTrigger )
         {
            _evictUpdatesTrigger = _evictDirtyTrigger ;
         }
      }

   protected:
      boost::filesystem::path _dbPath ;
      UINT32 _cacheSizeMB = DMS_DFT_WT_CACHE_SIZE ;
      UINT32 _evictTarget = DMS_DFT_WT_EVICT_TARGET ;
      UINT32 _evictTrigger = DMS_DFT_WT_EVICT_TRIGGER ;
      UINT32 _evictDirtyTarget = DMS_DFT_WT_EVICT_DIRTY_TARGET ;
      UINT32 _evictDirtyTrigger = DMS_DFT_WT_EVICT_DIRTY_TRIGGER ;
      UINT32 _evictUpdatesTarget = DMS_DFT_WT_EVICT_UPDATES_TARGET ;
      UINT32 _evictUpdatesTrigger = DMS_DFT_WT_EVICT_UPDATES_TRIGGER ;
      UINT32 _evictThreadsMin = DMS_DFT_WT_EVICT_THREADS_MIN ;
      UINT32 _evictThreadsMax = DMS_DFT_WT_EVICT_THREADS_MAX ;
      UINT32 _checkPointInterval = DMS_DFT_WT_CHECK_POINT_INTERVAL ;
   } ;

   typedef class _dmsWTEngineOptions dmsWTEngineOptions ;

}
}

#endif // DMS_WT_ENGINE_OPTIONS_HPP_
