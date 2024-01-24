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

   Source File Name = pmdMemPool.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_MEM_POOL_HPP_
#define PMD_MEM_POOL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "utilCache.hpp"
#include "rtnBackgroundJobBase.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _pmdBuffPool define
   */
   class _pmdBuffPool : public _utilCacheMgr
   {
      typedef ossPoolList<_utilCacheUnit*>            LIST_UNIT ;

      public:
         _pmdBuffPool() ;
         virtual ~_pmdBuffPool() ;

         virtual INT32     init( UINT64 cacheSize = 0 ) ;
         virtual void      fini() ;
         void              setMaxCacheSize( UINT64 maxCacheSize ) ;
         void              setMaxCacheJob( UINT32 maxCacheJob ) ;
         void              enablePerfStat( BOOLEAN enable = TRUE ) ;

         BOOLEAN           isEnabledPerfStat() const { return _perfStat ; }

         void              exitJob( BOOLEAN isControl ) ;

         ossEvent*         getEvent() { return &_wakeUpEvent ; }

      public:
         /*
            CacheUnit Management Functions
         */
         virtual void      registerUnit( _utilCacheUnit *pUnit ) ;
         virtual void      unregUnit( _utilCacheUnit *pUnit ) ;

         _utilCacheUnit*   dispatchUnit() ;
         void              pushBackUnit( _utilCacheUnit *pUnit ) ;

         void              checkLoad() ;

      protected:
         void              _checkAndStartJob( BOOLEAN needLock = TRUE ) ;

      protected:
         /*
            Unit Management Members
         */
         LIST_UNIT            _unitList ;
         ossSpinXLatch        _unitLatch ;
         BOOLEAN              _startCtrlJob ;

         UINT32               _curAgent ;
         UINT32               _idleAgent ;
         UINT32               _maxCacheJob ;
         BOOLEAN              _perfStat ;

         ossEvent             _wakeUpEvent ;
   } ;
   typedef _pmdBuffPool pmdBuffPool ;

   /*
      _pmdCacheJob define
   */
   class _pmdCacheJob : public _rtnBaseJob
   {
      public:
         _pmdCacheJob( pmdBuffPool *pBuffPool, INT32 timeout = -1 ) ;
         virtual ~_pmdCacheJob() ;

         BOOLEAN isControlJob() const ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      protected:
         void           _doUnit( _utilCacheUnit *pUnit ) ;

      protected:
         pmdBuffPool             *_pBuffPool ;
         INT32                   _timeout ;
   } ;
   typedef _pmdCacheJob pmdCacheJob ;

   /*
      Start a Cache Job
   */
   INT32 pmdStartCacheJob( EDUID *pEDUID, pmdBuffPool *pBuffPool,
                           INT32 timeout ) ;

   /*
      _pmdMemPool define
   */
   class _pmdMemPool : public SDBObject
   {
      public:
         _pmdMemPool() ;
         ~_pmdMemPool() ;

         INT32 initialize () ;
         INT32 final () ;
         void  clear() ;

         UINT64 totalSize() { return _totalMemSize.peek() ; }

      public:
         CHAR* alloc( UINT32 size, UINT32 &assignSize ) ;
         void  release ( CHAR* pBuff, UINT32 size ) ;
         CHAR* realloc ( CHAR* pBuff, UINT32 srcSize,
                         UINT32 needSize, UINT32 &assignSize ) ;

      private:
         ossAtomic64    _totalMemSize ;
   } ;
   typedef _pmdMemPool pmdMemPool ;

}

#endif //PMD_MEM_POOL_HPP_

