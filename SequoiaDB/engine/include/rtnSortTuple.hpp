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

   Source File Name = rtnSortTuple.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSORTTUPLE_HPP_
#define RTNSORTTUPLE_HPP_

#include "rtnSortDef.hpp"
#include "../bson/ordering.h"
#include "utilMinHeap.hpp"
#include "ixm_common.hpp"

namespace engine
{
   class _rtnSortTuple
   {
   public:
      _rtnSortTuple()
      :_len( 0 )
      {
         _hash.hash = 0 ;
      }

      ~_rtnSortTuple() {}

   public:
      OSS_INLINE void setLen( UINT32 keyLen, UINT32 objLen )
      {
         _len = sizeof( _rtnSortTuple ) + keyLen + objLen ;
         return ;
      }

      OSS_INLINE void setHash( UINT32 hash1, UINT32 hash2 )
      {
         _hash.columns.hash1 = hash1 ;
         _hash.columns.hash2 = hash2 ;
         return ;
      }

      OSS_INLINE const CHAR *key() const
      {
         return ( const CHAR *)this + sizeof( _rtnSortTuple ) ;
      }

      std::string toString()const ;

      OSS_INLINE const CHAR *obj() const
      {
         return ( const CHAR *)this + sizeof( _rtnSortTuple ) +
                *((INT32 *)key()) ;
      }

      OSS_INLINE UINT32 objLen() const
      {
         return ( _len - sizeof( _rtnSortTuple ) - *( (UINT32 *)key() ) ) ;
      }

      OSS_INLINE const UINT64 &hash() const
      {
         return _hash.hash ;
      }

      OSS_INLINE ixmHashValue& hashValue()
      {
         return _hash ;
      }

      OSS_INLINE UINT32 len()const
      {
         return _len ;
      }

      OSS_INLINE INT32 compare( const _rtnSortTuple *tuple,
                            const Ordering &order ) const
      {
         BSONObj l( this->key(), FALSE ) ;
         BSONObj r( tuple->key(), FALSE ) ;
         INT32 comp = l.woCompare( r, order, FALSE) ;
         if ( 0 == comp )
         {
            if ( this->hash() == tuple->hash() )
            {
               return 0 ;
            }
            else if ( this->hash() < tuple->hash() )
            {
               return -1 ;
            }
            else
            {
               return 1 ;
            }
         }
         else
         {
            return comp ;
         }
      }

   private:
      UINT32 _len ;
      ixmHashValue _hash ;
   } ;

   class _rtnMergeTuple
   {
   public:
      _rtnMergeTuple( INT32 i,
                      const _rtnSortTuple *tuple)
      :_i( i ),
       _tuple( tuple )
      {

      }

      _rtnMergeTuple()
      :_i(-1),
       _tuple(NULL)
      {}

      ~_rtnMergeTuple(){}

   public:
      const CHAR *obj()const
      {
         return _tuple->obj() ;
      }

      UINT32 objLen() const
      {
         return _tuple->objLen() ;
      }

      const CHAR *key() const
      {
         return _tuple->key() ;
      }

      const _rtnSortTuple *tuple()const
      {
         return _tuple ;
      }

      INT32 getI() const
      {
         return _i ;
      }

   private:
      UINT32 _i ;
      const _rtnSortTuple *_tuple ;
   } ;

   class _rtnMergeTupleComp
   {
   public:
      _rtnMergeTupleComp( const Ordering &order)
      :_order( order )
      {}

      ~_rtnMergeTupleComp(){}

   public:
      BOOLEAN operator()( const _rtnMergeTuple &l,
                          const _rtnMergeTuple &r )const
      {
         return 0 > l.tuple()->compare( r.tuple(), _order ) ;
      }
   private:
      bson::Ordering _order ;
   } ;

   typedef class _utilMinHeap<_rtnMergeTuple, _rtnMergeTupleComp> RTN_MERGE_HEAP ;
}

#endif

