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

   Source File Name = qgmHashTable.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmHashTable.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "qgmTrace.hpp"
#include "pdTrace.hpp"

#define QGM_BUCKETS_PERCENT 0.05
#define QGM_HASH( a )\
        ( ossHash( (a).value(), (a).valuesize() ) % _buckets )

#define QGM_GET_BUCKET(hash)\
        ( (hashTuple **)\
          (_buf + ( (hash) * sizeof(hashTuple *) )))

namespace engine
{
   _qgmHashTable::_qgmHashTable()
   :_buf(NULL),
    _bufSize( 0 ),
    _written( 0 ),
    _buckets( 0 )
   {

   }

   _qgmHashTable::~_qgmHashTable()
   {
      release() ;
   }

   INT32 _qgmHashTable::init( UINT64 bufSize )
   {
      SDB_ASSERT( sizeof(hashTuple *) / QGM_BUCKETS_PERCENT <= bufSize,
                  "impossible" ) ;
      INT32 rc = SDB_OK ;
      _buf = ( CHAR * )SDB_OSS_MALLOC( bufSize ) ;
      if ( NULL == _buf )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _bufSize = bufSize ;
      _buckets = _bufSize * QGM_BUCKETS_PERCENT / sizeof( hashTuple *) ;
      SDB_ASSERT( 0 != _buckets, "impossible" ) ;
      _written = _buckets * sizeof( hashTuple *) ;
      ossMemset( _buf, 0, _written ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _qgmHashTable::clear()
   {
      _written = _buckets * sizeof( hashTuple *) ;
      ossMemset( _buf, 0, _written ) ;
      return ;
   }

   void _qgmHashTable::release()
   {
      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
      }
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMHASHTBL_PUSH, "_qgmHashTable::push" )
   INT32 _qgmHashTable::push( const CHAR *fieldName,
                              const BSONObj &value )
   {
      PD_TRACE_ENTRY( SDB__QGMHASHTBL_PUSH ) ;
      SDB_ASSERT( NULL != fieldName, "impossible" ) ;
      SDB_ASSERT( NULL != _buf, "impossible" ) ;
      INT32 rc = SDB_OK ;
      if ( (_bufSize - _written) <
           (sizeof( hashTuple ) + value.objsize()) )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      {
      BSONElement key = value.getField( fieldName ) ;
      if ( key.eoo() )
      {
         goto done ;
      }

      {
      ossMemcpy( _buf + _written, &key, sizeof( BSONElement )) ;
      ossMemcpy( _buf + _written + sizeof(hashTuple),
                 value.objdata(), value.objsize() ) ;

      hashTuple *tuple = ( hashTuple * )( _buf + _written ) ;
      tuple->data = _buf + _written + sizeof(hashTuple) +
                    ( key.value() - 1 - tuple->fieldNameSize -/// see value() in BSONElement.
                      value.objdata() ) ;

      SDB_ASSERT( !((BSONElement *)tuple)->eoo(), "can not be eoo" ) ;
      tuple->value = _buf + _written + sizeof(hashTuple);
      _written += sizeof( hashTuple ) + value.objsize() ;

      UINT32 hash = QGM_HASH( key ) ;
      hashTuple ** bucket = QGM_GET_BUCKET( hash ) ;

      if ( NULL == *bucket )
      {
         *bucket = tuple ;
         tuple->next = NULL ;
      }
      else
      {
         tuple->next = (*bucket)->next ;
         (*bucket)->next = tuple ;
      }
      }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMHASHTBL_PUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMHASHTBL_FIND, "_qgmHashTable::find" )
   INT32 _qgmHashTable::find( const BSONElement &key,
                              QGM_HT_CONTEXT &context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMHASHTBL_FIND ) ;
      SDB_ASSERT( NULL != _buf, "impossible" ) ;

      if ( !key.eoo() )
      {
         UINT32 hash = QGM_HASH( key ) ;
         hashTuple **bucket = QGM_GET_BUCKET( hash ) ;
         if ( NULL == *bucket )
         {
            context = QGM_HT_INVALID_CONTEXT ;
         }
         else
         {
            context = ( const CHAR * )(*bucket) - _buf ;
         }
      }
      else
      {
         context = QGM_HT_INVALID_CONTEXT ;
      }
      PD_TRACE_EXITRC( SDB__QGMHASHTBL_FIND, rc ) ;
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMHASHTBL_GETMORE, "_qgmHashTable::getMore")
   INT32 _qgmHashTable::getMore( const BSONElement &key,
                                 QGM_HT_CONTEXT &context,
                                 BSONObj &value )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMHASHTBL_GETMORE ) ;
      hashTuple *tuple = NULL ;
      BSONElement *keyEle = NULL ;

      do
      {
         if ( QGM_HT_INVALID_CONTEXT == context )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }

         SDB_ASSERT( context <= (QGM_HT_CONTEXT)_bufSize, "impossible" ) ;
         tuple = ( hashTuple * )( _buf + context );
         if ( NULL == tuple )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }

         keyEle = ( BSONElement * )tuple ;
         SDB_ASSERT( !keyEle->eoo(), "can not be eoo." ) ;
         if ( 0 == key.woCompare( *keyEle, FALSE ))
         {
            value = BSONObj( tuple->value ) ;
            context = NULL == tuple->next ?
                      QGM_HT_INVALID_CONTEXT :
                      ( const CHAR * )(tuple->next) - _buf ;
            break ;
         }
         else
         {
            context = NULL == tuple->next ?
                      QGM_HT_INVALID_CONTEXT :
                      ( const CHAR * )(tuple->next) - _buf ;
         }
      } while (TRUE) ;
   done:
      PD_TRACE_EXITRC( SDB__QGMHASHTBL_GETMORE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

