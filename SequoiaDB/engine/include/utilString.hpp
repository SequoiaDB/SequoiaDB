/******************************************************************************

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

   Source File Name = utilString.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_STRING_HPP_
#define UTIL_STRING_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include <boost/noncopyable.hpp>

#define UTIL_STRING_STAITC_LEN 256 
#define UTIL_STRING_INT_LEN    11
#define UTIL_STRING_INT64_LEN  22
#define UTIL_STRING_DOUBLE_LEN 25

namespace engine
{
   template < UINT32 BUFFERSIZE = UTIL_STRING_STAITC_LEN > 
   class _utilString : public SDBObject, boost::noncopyable
   {
   public:
      _utilString() :_dynamic( NULL ), _buf( _static ), 
                     _bufLen( BUFFERSIZE ), _len( 0 )
      {
         _buf[0] = '\0' ;
      }

      ~_utilString()
      {
         SAFE_OSS_FREE( _dynamic ) ;
      }

   public:
      INT32 append( const CHAR *str, UINT32 len )
      {
         INT32 rc = SDB_OK ;
         SDB_ASSERT( NULL != str, "can not be null" ) ;

         if ( NULL == str || 0 == len )
         {
            goto done ;
         }

         if ( !enough( len ) )
         {
            UINT32 dLen = _bufLen * 2 ;
            UINT32 minSize = _len + len + 1 ;
            rc = resize( dLen <= minSize ?
                         minSize : dLen ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extend buf:%d", rc ) ;
               goto error ;
            }
         }

         ossMemcpy( _buf + _len, str, len ) ;
         _len += len ;
         _buf[_len] = '\0' ;
      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 append( CHAR c )
      {
         return append( &c, 1 ) ;
      }

      INT32 resize( UINT32 size )
      {
         INT32 rc = SDB_OK ;
         if ( UTIL_STRING_STAITC_LEN < _bufLen &&
              _bufLen < size )
         {
            CHAR *p = ( CHAR * )SDB_OSS_REALLOC( _dynamic, size ) ;
            if ( NULL == p )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            _dynamic = p ;
            _bufLen = size ;
            _buf = _dynamic ;
         }
         else if ( UTIL_STRING_STAITC_LEN == _bufLen &&
                   _bufLen < size )
         {
            _dynamic = ( CHAR * )SDB_OSS_MALLOC( size ) ;
            if ( NULL == _dynamic )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            _bufLen = size ;
            if ( 0 < _len )
            {
               ossMemcpy( _dynamic, _static, _len ) ;
            }
            _dynamic[_len] = '\0' ; 
            _buf = _dynamic ;
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE const CHAR *str() const
      {
         return _buf ;
      }

      OSS_INLINE UINT32 len() const
      {
         return _len ;
      }

      OSS_INLINE BOOLEAN enough( UINT32 len ) const
      {
         return _len + len + 1 <= _bufLen ;
      }

      INT32 appendINT32( INT32 v )
      {
         return appendNumber( v, UTIL_STRING_INT_LEN, "%d" ) ;
      }

      INT32 appendDouble( FLOAT64 v )
      {
         return appendNumber( v, UTIL_STRING_DOUBLE_LEN, "%g" ) ;
      }

      INT32 appendINT64( INT64 v )
      {
         return appendNumber( v, UTIL_STRING_INT64_LEN, "%lld" ) ;
      }

      template <typename T>
      INT32 appendNumber( T v, UINT32 size, const CHAR *macro )
      {
         INT32 rc = SDB_OK ;
         if ( !enough( size ) )
         {
            rc = resize( size + _bufLen ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         rc = sprintf( &(_buf[_len]), macro, v ) ;
         if ( rc < SDB_OK )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         _len += rc ;
         rc = SDB_OK ;
         _buf[_len] = '\0';
      done:
         return rc ;
      error:
         goto done ;
      }
   private:
      CHAR _static[ BUFFERSIZE ] ;
      CHAR *_dynamic ;
      CHAR *_buf ;
      UINT32 _bufLen ;
      UINT32 _len ; 
   } ;
}

#endif

