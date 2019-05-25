/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnIxmKeySorter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnIxmKeySorter.hpp"
#include <algorithm>
#include "ossUtil.h"

namespace engine
{
   _rtnIxmKeySorter::_rtnIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer )
   : _dmsIxmKeySorter( bufSize, comparer )
   {
      _buf = NULL ;
      _headOffset = 0 ;
      _tailOffset = bufSize ;
      _keyNum = 0 ;
      _fetchedNum = 0 ;
      _sorted = FALSE ;
      _inited = FALSE ;
   }

   _rtnIxmKeySorter::~_rtnIxmKeySorter()
   {
      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
      }
   }

   INT32 _rtnIxmKeySorter::init()
   {
      INT32 rc = SDB_OK ;

      if ( !_inited )
      {
         _buf = ( CHAR* )SDB_OSS_MALLOC( _bufSize ) ;
         if ( NULL == _buf )
         {
            PD_LOG( PDERROR, "failed to allocate buffer for index sorting." ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         _inited = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 _rtnIxmKeySorter::reset()
   {
      SDB_ASSERT( _inited, "must be inited" ) ;
      _headOffset = 0 ;
      _tailOffset = _bufSize ;
      _keyNum = 0 ;
      _fetchedNum = 0 ;
      _sorted = FALSE ;
      return SDB_OK ;
   }

   INT32 _rtnIxmKeySorter::push( const ixmKey& key, const dmsRecordID& recordID )
   {
      INT32 keyDataSize = key.dataSize() ;
      INT32 rc = SDB_OK ;
      CHAR* keyPosition ;
      dmsRecordID* rid ;
      CHAR** keySlot ;

      SDB_ASSERT( _inited, "must be inited before pushing" ) ;
      SDB_ASSERT( !_sorted, "already sorted, can't push" ) ;

      if ( (INT64)( keyDataSize + sizeof(dmsRecordID) + sizeof(ixmKey*) ) >
           _tailOffset - _headOffset )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      _tailOffset -= keyDataSize + sizeof(dmsRecordID) ;

      keyPosition = _buf + _tailOffset ;
      ossMemcpy( keyPosition , key.data(), keyDataSize ) ;

      rid = (dmsRecordID*)( keyPosition + keyDataSize ) ;
      *rid = recordID ;

      keySlot = (CHAR**)( _buf + _headOffset ) ;
      *keySlot = keyPosition ;
      _headOffset += sizeof(ixmKey*) ;

      _keyNum++ ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnIxmKeySorter::sort()
   {
      SDB_ASSERT( _inited, "must be inited" ) ;
      SDB_ASSERT( !_sorted, "already sorted") ;

      if ( _keyNum > 0 )
      {
         CHAR** keyStart = (CHAR**)_buf ;
         CHAR** keyEnd = (CHAR**)( _buf + _keyNum * sizeof(ixmKey*) ) ;
         std::sort( keyStart, keyEnd, _comparer ) ;
      }

      _sorted = TRUE ;
      return SDB_OK ;
   }

   INT32 _rtnIxmKeySorter::fetch( ixmKey& key, dmsRecordID& recordID )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "must be inited" ) ;

      if( _fetchedNum < _keyNum )
      {
         CHAR* keyData = *(CHAR**)( _buf + sizeof(ixmKey*) * _fetchedNum ) ;
         key.assign( ixmKey( keyData ) );
         recordID = *(dmsRecordID*)( keyData + key.dataSize() ) ;
         _fetchedNum++ ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 _rtnIxmKeySorter::usedBufferSize() const
   {
      return ( _bufSize - _tailOffset + _headOffset ) ;
   }

   _dmsIxmKeySorter* _rtnIxmKeySorterCreator::createSorter(  INT64 bufSize, const _dmsIxmKeyComparer& comparer )
   {
      INT32 rc = SDB_OK ;
      _rtnIxmKeySorter* sorter = NULL ;

      sorter = SDB_OSS_NEW _rtnIxmKeySorter( bufSize, comparer ) ;
      if ( NULL == sorter )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "failed to create _rtnIxmKeySorter, rc: %d", rc ) ;
         goto error ;
      }

      rc = sorter->init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init _rtnIxmKeySorter, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return sorter ;
   error:
      if ( NULL != sorter )
      {
         SDB_OSS_DEL( sorter ) ;
         sorter = NULL ;
      }
      goto done ;
   }

   void _rtnIxmKeySorterCreator::releaseSorter( _dmsIxmKeySorter* sorter )
   {
      SDB_ASSERT( NULL != sorter, "sorter can't be NULL" ) ;

      SDB_OSS_DEL( sorter ) ;
   }
}

