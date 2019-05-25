/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobDataPool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLobDataPool.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"
#include <algorithm>

using namespace std ;

namespace engine
{
   _rtnLobDataPool::_rtnLobDataPool()
   :_buf( NULL ),
    _bufSz( 0 ),
    _lastDataSz( 0 ),
    _dataSz( 0 ),
    _current( -1 )
   {

   }

   _rtnLobDataPool::~_rtnLobDataPool()
   {
      clear() ;

      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
         _bufSz = 0 ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBDATAPOOL_MATCH, "_rtnLobDataPool::match" )
   BOOLEAN _rtnLobDataPool::match( SINT64 offset )
   {
      PD_TRACE_ENTRY( SDB_RTNLOBDATAPOOL_MATCH ) ;
      BOOLEAN matched = FALSE ;
      if ( _pool.empty() )
      {
      }
      else if ( offset == _currentTuple.offset )
      {
         matched = TRUE ;
      }
      else
      {
         const tuple &first = _pool[0] ;
         if ( first.offset <= offset &&
              offset < ( first.offset + _dataSz ) )
         {
            _seek( offset ) ;
            matched = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB_RTNLOBDATAPOOL_MATCH ) ;
      return matched ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBDATAPOOL_ALLOCATE, "_rtnLobDataPool::allocate" )
   INT32 _rtnLobDataPool::allocate( UINT32 len, CHAR **buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBDATAPOOL_ALLOCATE ) ;
      SDB_ASSERT( NULL != buf, "can not be null" ) ;
      
      if ( len <= _bufSz )
      {
         *buf = _buf ;
         goto done ;
      }
      else if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _bufSz = 0 ;
      }

      _buf = ( CHAR * )SDB_OSS_MALLOC( len ) ;
      if ( NULL == _buf )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ; 
         rc = SDB_OOM ;
         goto error ;
      }
      _bufSz = len ;
      *buf = _buf ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOBDATAPOOL_ALLOCATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnLobDataPool::entrust( CHAR *buf )
   {
      _toBeFreed.push_back( buf ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBDATAPOOL_NEXT, "_rtnLobDataPool::next" )
   BOOLEAN _rtnLobDataPool::next( UINT32 len, const CHAR **buf, UINT32 &read )
   {
      BOOLEAN res = FALSE ;
      PD_TRACE_ENTRY( SDB_RTNLOBDATAPOOL_NEXT ) ;

      if ( 0 <= _current )
      {
         SDB_ASSERT( _pool.size() > ( UINT32 )_current, "impossible" ) ;
         UINT32 realLen = len <= _currentTuple.len ? len : _currentTuple.len ;
         *buf = _currentTuple.data ;
         read = realLen ;
         _currentTuple.len -= realLen ;
         _lastDataSz -= realLen ;
         res = TRUE ;

         if ( 0 == _currentTuple.len )
         {
            ++_current ;
            if ( _pool.size() == ( UINT32 )_current )
            {
               _current = -1 ;
               _currentTuple.clear() ;
               SDB_ASSERT( 0 == _lastDataSz, "must be zero" ) ;
            }
            else
            {
               _currentTuple = _pool.at(_current) ;
            }
         }
         else
         {
            _currentTuple.data += realLen ;
            _currentTuple.offset += realLen ;
         }
      }

      PD_TRACE_EXIT( SDB_RTNLOBDATAPOOL_NEXT ) ;
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBDATAPOOL_PUSH, "_rtnLobDataPool::push" )
   INT32 _rtnLobDataPool::push( const CHAR *data, UINT32 len,
                                SINT64 offset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBDATAPOOL_PUSH ) ;
      SDB_ASSERT( NULL != data, "can not be null" ) ;
      _pool.push_back( tuple( offset, len, data ) ) ;
      _lastDataSz += len ;
      _dataSz += len ;
      PD_TRACE_EXITRC( SDB_RTNLOBDATAPOOL_PUSH, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBDATAPOOL_PUSHDONE, "_rtnLobDataPool::pushDone" )
   void _rtnLobDataPool::pushDone()
   {
      PD_TRACE_ENTRY( SDB_RTNLOBDATAPOOL_PUSHDONE ) ;
      if ( 1 < _pool.size() )
      {
         std::sort( _pool.begin(), _pool.end(), compare() ) ;
      }
#if defined (_DEBUG)
      SINT64 offset = -1 ;
      vector<tuple>::const_iterator itr = _pool.begin() ;
      for ( ; itr != _pool.end(); ++itr )
      {
         if ( -1 == offset )
         {
            offset = itr->offset + itr->len ;
         }
         else
         {
            SDB_ASSERT( offset == itr->offset, "impossible" ) ;
            offset += itr->len ;
         }
      }
#endif
      _current = 0 ;
      _currentTuple = _pool.at(_current) ;
      PD_TRACE_EXIT( SDB_RTNLOBDATAPOOL_PUSHDONE ) ;
      return ;
   }

   void _rtnLobDataPool::clear()
   {
      _pool.clear() ;
      _lastDataSz = 0 ;
      _dataSz = 0 ;
      _current = -1 ;
      _currentTuple.clear() ;
      std::list<CHAR *>::iterator itr = _toBeFreed.begin() ;
      for ( ; itr != _toBeFreed.end(); ++itr )
      {
         SDB_OSS_FREE( *itr ) ;
      }
      _toBeFreed.clear() ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBDATAPOOL__SEEK, "_rtnLobDataPool::_seek" )
   void _rtnLobDataPool::_seek( SINT64 offset )
   {
      PD_TRACE_ENTRY( SDB_RTNLOBDATAPOOL__SEEK ) ;
      UINT32 totalSeek = 0 ;
      std::vector<tuple>::const_iterator itr = _pool.begin() ;
      for ( UINT32 i = 0 ; itr != _pool.end(); ++itr, ++i )
      {
         _currentTuple = *itr ;
         _current = i ;
         SDB_ASSERT( _currentTuple.offset <= offset, "impossible" ) ;
         SINT64 seekLen = offset - _currentTuple.offset ;
         if ( seekLen < ( SINT64 )( _currentTuple.len ) )
         {
            _currentTuple.offset += seekLen ;
            _currentTuple.len -= seekLen ;
            _currentTuple.data += seekLen ;
            totalSeek += seekLen ;
            break ;
         }
         else
         {
            totalSeek += _currentTuple.len ;
         }
      }

      _lastDataSz = _dataSz - totalSeek ;
      PD_TRACE_EXIT( SDB_RTNLOBDATAPOOL__SEEK ) ;
      return ;
   }
}

