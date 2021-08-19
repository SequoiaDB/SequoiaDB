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

   Source File Name = spdFMPMgr.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "spdFMPMgr.hpp"
#include "spdFMP.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "fmpDef.hpp"

#define SPD_POOL_HIGH_WATERMARK           ( 5 )

namespace engine
{
   _spdFMPMgr::_spdFMPMgr()
   :_startBuf(NULL),
    _allocated( 0 )
   {
      _hwSeqID = 0 ;
   }

   _spdFMPMgr::~_spdFMPMgr()
   {
      while ( TRUE )
      {
         _mtx.get() ;
         INT32 allocated = _allocated ;
         if ( 0 == allocated )
         {
            _mtx.release() ;
            break ;
         }
         else if ( (UINT32)allocated != _pool.size() )
         {
            _mtx.release() ;
            PD_LOG( PDINFO, "not all fmp are returned:%d", allocated ) ;
            ossSleepmillis(500) ;
         }
         else
         {
            while ( !_pool.empty() )
            {
               _spdFMP *fmp = _pool.back() ;
               _pool.pop_back() ;
               SAFE_OSS_DELETE( fmp ) ;
               --_allocated ;
            }
            _mtx.release() ;
            break ;
         }
      }

      SDB_ASSERT( 0 == _allocated, "impossible" ) ;

      if ( NULL != _startBuf )
      {
         SDB_OSS_FREE( _startBuf ) ;
      }
   }

   INT32 _spdFMPMgr::init()
   {
      INT32 rc = SDB_OK ;
      CHAR path[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      UINT32 pathLen = 0 ;
      UINT32 appendLen = 0 ;
      INT32 bufSize = 0 ;
      string fullPath ;
      std::list<const CHAR *> argv ;

      rc = ossGetEWD( path, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get file path:%d",rc ) ;
         goto error ;
      }

      appendLen = ossStrlen(SPD_PROCESS_NAME) + 1 ;
      pathLen = ossStrlen( path ) ;
      if ( OSS_MAX_PATHSIZE < appendLen + pathLen )
      {
         PD_LOG( PDERROR, "file path is too long:%s", path ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _startBuf = (CHAR *)SDB_OSS_MALLOC(pathLen + appendLen + 1) ; // +1 for '\0'
      if ( NULL == _startBuf )
      {
         PD_LOG( PDERROR, "failed to allocate mem.") ;
         rc = SDB_OOM ;
         goto error ;
      }

      fullPath.append( path ) ;
      fullPath.append( OSS_FILE_SEP ) ;
      fullPath.append( SPD_PROCESS_NAME ) ;

      argv.push_back( fullPath.c_str() ) ;

      rc = ossBuildArguments( &_startBuf, bufSize, argv ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build arguments for fmp:%d", rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "sub process path:%s", fullPath.c_str() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMPMgr::active ()
   {
      return SDB_OK ;
   }

   INT32 _spdFMPMgr::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _spdFMPMgr::fini ()
   {
      return SDB_OK ;
   }

   INT32 _spdFMPMgr::getFMP( _spdFMP *&fmp )
   {
      INT32 rc = SDB_OK ;
      _spdFMP *got = NULL ;

      _mtx.get() ;
      if ( _pool.empty() )
      {
         _mtx.release() ;

         rc = _createNewFMP( got ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create new fmp:%d",rc ) ;
            goto error ;
         }

         _mtx.get() ;
         ++_allocated ;
         if( _vecFreeSeqID.size() > 0 )
         {
            got->_seqID = _vecFreeSeqID.back() ;
            _vecFreeSeqID.pop_back() ;
         }
         else
         {
            got->_seqID = ++_hwSeqID ;
         }
         _mtx.release() ;
      }
      else
      {
         got = _pool.back() ;
         _pool.pop_back() ;
         _mtx.release() ;
      }

      fmp = got ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMPMgr::_createNewFMP( _spdFMP *&fmp )
   {
      INT32 rc = SDB_OK ;
      ossResultCode result ;
      _spdFMP *fmpNode =  SDB_OSS_NEW _spdFMP() ;
      if ( NULL == fmpNode )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = ossExec ( _startBuf, _startBuf, NULL,
                     0, fmpNode->_id,
                     result, &( fmpNode->_in), &( fmpNode->_out) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to start fmp node:%d", rc ) ;
         goto error ;
      }

      fmp = fmpNode ;
   done:
      return rc ;
   error:
      if ( NULL != fmpNode )
      {
         SDB_OSS_DEL fmpNode ;
      }
      goto done ;
   }

   INT32 _spdFMPMgr::returnFMP( _spdFMP *fmp, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 poolSize = 0 ;
      INT32 allocateSize = 0 ;
      SDB_ASSERT( NULL != fmp && -1 != fmp->id(), "impossible" ) ;

       _mtx.get() ;
      if ( fmp->discarded() ||
           SPD_POOL_HIGH_WATERMARK <= _pool.size() )
      {
         --_allocated ;
         _vecFreeSeqID.push_back( fmp->getSeqID() ) ;
         poolSize = _pool.size() ;
         allocateSize = _allocated ;
         SDB_ASSERT( 0 <= _allocated, "impossible" ) ;
         _mtx.release() ;
         rc = fmp->quit( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to quit fmp:%d",rc ) ;
         }
         SAFE_OSS_DELETE( fmp ) ;
      }
      else
      {
         _mtx.release() ;
         rc = fmp->reset( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to reset fmp:%d",rc ) ;
            _mtx.get() ;
            --_allocated ;
            _vecFreeSeqID.push_back( fmp->getSeqID() ) ;
            poolSize = _pool.size() ;
            allocateSize = _allocated ;
            _mtx.release() ;
            SAFE_OSS_DELETE( fmp ) ;
            goto error ;
         }
         else
         {
            _mtx.get() ;
            _pool.push_back( fmp ) ;
            poolSize = _pool.size() ;
            allocateSize = _allocated ;
            _mtx.release() ;
         }
      }

      PD_LOG( PDDEBUG, "pool size:%d, allocate size:%d",
              poolSize, allocateSize ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      get global fmp cb
   */
   spdFMPMgr* sdbGetFMPCB ()
   {
      static spdFMPMgr s_fmpCB ;
      return &s_fmpCB ;
   }

}
