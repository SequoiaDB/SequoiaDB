/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = utilESCltMgr.cpp

   Descriptive Name = Elasticsearch client manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/08/2019  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilESCltMgr.hpp"

namespace seadapter
{
   _utilESCltMgr::_utilESCltMgr()
   : _limit( UTIL_ESCLT_DFT_MAX_NUM ),
     _cleanInterval( UTIL_ESCLT_DFT_CLEAN_INTERVAL ),
     _opTimeout( SEADPT_DFT_TIMEOUT ),
     _number( 0 ),
     _scrollSize( SEADPT_DFT_SCROLL_SIZE )
   {
      _url[0] = '\0' ;
   }

   _utilESCltMgr::~_utilESCltMgr()
   {
      // Release all clients in the pool.
      CLT_LIST_ITR itr = _cltList.begin() ;
      while ( itr != _cltList.end() )
      {
         if ( *itr )
         {
            SDB_OSS_DEL *itr ;
         }
         ++itr ;
      }
   }

   INT32 _utilESCltMgr::init( const CHAR *url, UINT32 limit,
                              UINT32 cleanInterval, INT32 opTimeout )
   {
      INT32 rc = SDB_OK ;

      if ( !url || ossStrlen( url ) > SEADPT_SE_SVCADDR_MAX_SZ )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Url to init search engine client "
                          "manager is invalid" ) ;
         goto error ;
      }
      ossStrncpy( _url, url, SEADPT_SE_SVCADDR_MAX_SZ + 1 ) ;
      _limit = limit ;
      _cleanInterval = cleanInterval ;
      _opTimeout = opTimeout ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESCltMgr::setScrollSize( UINT16 size )
   {
      _scrollSize = size ;
   }

   INT32 _utilESCltMgr::getClient( utilESClt *& client )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN createNew = FALSE ;

   retry:
      _latch.get() ;
      if ( !_cltList.empty() )
      {
         client = _cltList.back() ;
         _cltList.pop_back() ;
      }
      else if ( _number < _limit )
      {
         // No valid client, and still in the threshold, create a new one.
         // First increase the _number to avoid too many threads to create new
         // clients.
         ++_number ;
         createNew = TRUE ;
      }
      else
      {
         _latch.release() ;
         ossSleep( 10 ) ;
         goto retry ;
      }

      _latch.release() ;

      if ( createNew )
      {
         client = SDB_OSS_NEW utilESClt ;
         if ( !client )
         {
            rc = SDB_OOM ;
         }
         else
         {
            rc = client->init( _url, _opTimeout ) ;
         }
         if ( rc )
         {
            _latch.get() ;
            // Decrease the number so others can create a client.
            --_number ;
            _latch.release() ;
            if ( client )
            {
               SDB_OSS_DEL client ;
               client = NULL ;
            }
            PD_LOG( PDERROR, "Create search engine client failed[%d]", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESCltMgr::releaseClient( utilESClt *&client )
   {
      INT32 rc = SDB_OK ;
      utilESCltStat *stat = NULL ;
      if ( !client )
      {
         goto done ;
      }

      stat = client->getStat() ;
      ossGetCurrentTime( stat->idleTime ) ;

      // Releset the client before putting it back to cache.
      client->reset() ;

      _latch.get() ;
      // Push may fail because of running out of memory. In that case, free the
      // client directly.
      rc = _cltList.push_back( client ) ;
      if ( rc )
      {
         // Decrease the number of client in the protection of the lock.
         --_number ;
      }
      _latch.release() ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Release search engine client into cache "
                 "failed[%d]", rc ) ;
         SDB_OSS_DEL client ;
      }
      client = NULL ;

   done:
      return ;
   }

   void _utilESCltMgr::cleanup()
   {
      ossTimestamp currentTime ;
      ossGetCurrentTime( currentTime ) ;

      _latch.get() ;
      while ( _cltList.size() > 0 )
      {
         utilESClt *client = _cltList.front() ;
         utilESCltStat *stat = client->getStat() ;
         if ( currentTime.time - stat->idleTime.time >= _cleanInterval )
         {
            _cltList.pop_front() ;
            --_number ;
            _latch.release() ;
            SDB_OSS_DEL( client ) ;
            _latch.get() ;
         }
         else
         {
            break ;
         }
      }
      _latch.release() ;
   }

   _utilESCltMgr es_clt_mgr ;
}
