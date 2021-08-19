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

   Source File Name = ossAtomic.hpp

   Descriptive Name = Operating System Services Atomic Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for atomic
   operations.

   Dependencies: ossAtomicBase.hpp

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_ATOMIC_HPP
#define OSS_ATOMIC_HPP

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomicBase.hpp"

//
// OSS atomic class
//

template <class T>
class ossAtomicPredicateGreaterThan : public SDBObject
{
   public:
   BOOLEAN op( const T & leftOperand, const T & rightOperand ) const
   {
      return leftOperand > rightOperand ;
   }
} ;


template <class T>
class ossAtomicPredicateLesserThan : public SDBObject
{
   public:
   BOOLEAN op( const T & leftOperand, const T & rightOperand ) const
   {
      return leftOperand < rightOperand ;
   }
} ;


class _ossAtomic32 ;

// Implements atomic operation on a signed 32bit value.
// Note:
//    32bit alignment is required for this type.
class _ossAtomicSigned32 : public SDBObject
{
   public :
      typedef SINT32 valueType ;

   private :
      struct
      {
         valueType volatile value ;
      } m_value ;

      friend class _ossAtomic32 ;

      template < class T >
      valueType swapPredicated( valueType val, const T & predVal )
      {
         valueType prev = peek() ;
         for ( ; ; )
         {
            valueType newVal = prev ;
            if ( predVal.op( val, prev ) )
            {
               newVal = val ;
            }
            valueType tmp = ossCompareAndSwap32WithReturn( &m_value.value,
                                   prev,
                                   newVal ) ;
            if ( prev == tmp )
            {
               break ;
            }
            else
            {
               prev = tmp ;
            }
         }
         return prev ;
      }

   public :
      explicit _ossAtomicSigned32 ( valueType val )
      {
         init ( val ) ;
      }
      // object can be reinit to another value
      void init ( valueType val )
      {
         m_value.value = val ;
      }

      size_t size( void ) const
      {
         return sizeof( m_value.value ) ;
      }

      valueType peek() const
      {
         return ( m_value.value ) ;
      }

      valueType add( valueType val )
      {
         return ossFetchAndAdd32( &m_value.value, val ) ;
      }

      valueType sub( valueType val )
      {
         return  add( -val ) ;
      }

      valueType bitOR( valueType val )
      {
         return ossFetchAndOR32( &m_value.value, val ) ;
      }

      valueType bitAND( valueType val )
      {
         return ossFetchAndAND32( &m_value.value, val ) ;
      }

      valueType fetch( void )
      {
         return ossAtomicFetch32( &m_value.value ) ;
      }

      valueType inc( void )
      {
         return ossFetchAndIncrement32( &m_value.value ) ;
      }

      valueType dec( void )
      {
         return ossFetchAndDecrement32( &m_value.value ) ;
      }

      valueType swap( valueType val )
      {
         return ossAtomicExchange32( &m_value.value, val ) ;
      }

      BOOLEAN compareAndSwap( valueType compVal, valueType val )
      {
         return ossCompareAndSwap32( &m_value.value, compVal, val ) ;
      }

      valueType compareAndSwapWithReturn( valueType compVal, valueType val )
      {
         return ossCompareAndSwap32WithReturn( &m_value.value, compVal, val ) ;
      }

      valueType swapGreaterThan( valueType val )
      {
         return swapPredicated( val,
                ossAtomicPredicateGreaterThan<valueType>() ) ;
      }

      valueType swapLesserThan( valueType val )
      {
         return swapPredicated( val,
                ossAtomicPredicateLesserThan<valueType>() ) ;
      }

      BOOLEAN compare( valueType compVal )
      {
         return ossCompareAndSwap32( &m_value.value, compVal, compVal ) ;
      }

      void poke( valueType val )
      {
         m_value.value = val ;
      }
} ;
typedef class _ossAtomicSigned32 ossAtomicSigned32 ;


// Implements atomic operation on an unsigned 32bit value.
// Note:
//    32bit alignment is required for this type.
class _ossAtomic32 : public SDBObject
{
   private :
      ossAtomicSigned32 V ;
   public :
      typedef UINT32 valueType ;
      explicit _ossAtomic32 ( valueType val ):
      V((ossAtomicSigned32::valueType )val )
      {
      }

      void init( valueType val )
      {
         V.init( ( ossAtomicSigned32::valueType )val ) ;
      }

      size_t size( void )
      {
         return V.size() ;
      }

      valueType peek() const
      {
         return ( valueType )V.peek() ;
      }

      valueType add( valueType val )
      {
         return ( valueType )V.add( ( ossAtomicSigned32::valueType )val ) ;
      }

      valueType sub( valueType val )
      {
         return ( valueType )V.sub( ( ossAtomicSigned32::valueType )val ) ;
      }

      valueType bitOR( valueType val )
      {
         return ( valueType )V.bitOR( ( ossAtomicSigned32::valueType )val ) ;
      }

      valueType bitAND( valueType val )
      {
         return ( valueType )V.bitAND( ( ossAtomicSigned32::valueType )val ) ;
      }

      valueType fetch( void )
      {
         return ( valueType )V.fetch() ;
      }

      valueType inc( void )
      {
         return ( valueType )V.inc() ;
      }

      valueType dec( void )
      {
         return ( valueType )V.dec() ;
      }

      valueType swap( valueType val )
      {
         return (valueType)V.swap( ( ossAtomicSigned32::valueType )val ) ;
      }

      BOOLEAN compareAndSwap( valueType compVal, valueType val )
      {
         return V.compareAndSwap( ( ossAtomicSigned32::valueType )compVal,
                                   ( ossAtomicSigned32::valueType )val ) ;
      }

      valueType compareAndSwapWithReturn( valueType compVal, valueType val )
      {
         return V.compareAndSwapWithReturn(
                     ( ossAtomicSigned32::valueType )compVal,
                     ( ossAtomicSigned32::valueType )val ) ;
      }

      valueType swapGreaterThan( valueType val )
      {
         return V.swapPredicated( val,
                                  ossAtomicPredicateGreaterThan<valueType>() ) ;
      }

      valueType swapLesserThan( valueType val )
      {
         return V.swapPredicated( val,
                     ossAtomicPredicateLesserThan<valueType>() ) ;
      }

      BOOLEAN compare( valueType compVal )
      {
         return V.compare( ( ossAtomicSigned32::valueType )compVal ) ;
      }

      void poke( valueType val )
      {
         V.poke( ( ossAtomicSigned32::valueType )val ) ;
      }
} ;
typedef class _ossAtomic32 ossAtomic32 ;

class _ossAtomic64 ;

// Implements atomic operation on a signed 64bit value.
// Note:
//    64bit alignment is required for this type.
class _ossAtomicSigned64 : public SDBObject
{
   public :
      typedef SINT64 valueType ;

   private :
      struct
      {
         valueType volatile value ;
      } m_value ;

      friend class _ossAtomic64 ;

      template < class T >
      valueType swapPredicated( valueType val, const T & predVal )
      {
         valueType prev = peek() ;
         for ( ; ; )
         {
            valueType newVal = prev ;
            if ( predVal.op( val, prev ) )
            {
               newVal = val ;
            }
            valueType tmp = ossCompareAndSwap64WithReturn( &m_value.value,
                                                           prev,
                                                           newVal ) ;
            if ( prev == tmp )
            {
               break ;
            }
            else
            {
               prev = tmp ;
            }
         }
         return prev ;
      }

   public :
      explicit _ossAtomicSigned64 ( valueType val )
      {
         init ( val ) ;
      }
      void init ( valueType val )
      {
         m_value.value = val ;
      }

      size_t size( void ) const
      {
         return sizeof( m_value.value ) ;
      }

      valueType peek() const
      {
         return ossAtomicPeek64( &m_value.value ) ;
      }

      valueType add( valueType val )
      {
         return ossFetchAndAdd64( &m_value.value, val ) ;
      }

      valueType sub( valueType val )
      {
         return add( -val ) ;
      }

      valueType bitOR( valueType val )
      {
         return ossFetchAndOR64( &m_value.value, val ) ;
      }

      valueType bitAND( valueType val )
      {
         return ossFetchAndAND64( &m_value.value, val ) ;
      }

      valueType fetch( void )
      {
         return ossAtomicFetch64( &m_value.value ) ;
      }

      valueType inc( void )
      {
         return ossFetchAndIncrement64( &m_value.value ) ;
      }

      valueType dec( void )
      {
         return ossFetchAndDecrement64( &m_value.value ) ;
      }

      valueType swap( valueType val )
      {
         return ossAtomicExchange64( &m_value.value, val ) ;
      }

      BOOLEAN compareAndSwap( valueType compVal, valueType val )
      {
         return ossCompareAndSwap64( &m_value.value, compVal, val ) ;
      }

      valueType compareAndSwapWithReturn( valueType compVal, valueType val )
      {
         return ossCompareAndSwap64WithReturn( &m_value.value, compVal, val ) ;
      }

      valueType swapGreaterThan( valueType val )
      {
         return swapPredicated( val,
                ossAtomicPredicateGreaterThan<valueType>() ) ;
      }

      valueType swapLesserThan( valueType val )
      {
         return swapPredicated( val,
                ossAtomicPredicateLesserThan<valueType>() ) ;
      }

      BOOLEAN compare( valueType compVal )
      {
         return ossCompareAndSwap64( &m_value.value, compVal, compVal ) ;
      }

      void poke( valueType val )
      {
         m_value.value = val ;
      }
} ;
typedef class _ossAtomicSigned64 ossAtomicSigned64 ;

// Implements atomic operation on an unsigned 64bit value.
// Note:
//    64bit alignment is required for this type.
class _ossAtomic64 : public SDBObject
{
   private :
      ossAtomicSigned64 V ;
   public :
      typedef UINT64 valueType ;
      explicit _ossAtomic64 ( valueType val ):
      V(val)
      {
      }
      void init( valueType val )
      {
         V.init( ( ossAtomicSigned64::valueType )val ) ;
      }

      size_t size( void )
      {
         return  V.size() ;
      }

      valueType peek() const
      {
         return ( valueType )V.peek() ;
      }

      valueType add( valueType val )
      {
         return ( valueType )V.add( ( ossAtomicSigned64::valueType )val ) ;
      }

      valueType sub( valueType val )
      {
         return ( valueType )V.sub( ( ossAtomicSigned64::valueType )val ) ;
      }

      valueType bitOR( valueType val )
      {
         return ( valueType )V.bitOR( ( ossAtomicSigned64::valueType )val ) ;
      }

      valueType bitAND( valueType val )
      {
         return ( valueType )V.bitAND( ( ossAtomicSigned64::valueType )val ) ;
      }

      valueType fetch( void )
      {
         return ( valueType )V.fetch() ;
      }

      valueType inc( void )
      {
         return ( valueType )V.inc() ;
      }

      valueType dec( void )
      {
         return ( valueType )V.dec() ;
      }

      valueType swap( valueType val )
      {
         return (valueType)V.swap( ( ossAtomicSigned64::valueType )val ) ;
      }

      BOOLEAN compareAndSwap( valueType compVal, valueType val )
      {
         return V.compareAndSwap( ( ossAtomicSigned64::valueType )compVal,
                                  ( ossAtomicSigned64::valueType )val ) ;
      }

      valueType compareAndSwapWithReturn( valueType compVal, valueType val )
      {
         return V.compareAndSwapWithReturn(
                     ( ossAtomicSigned64::valueType )compVal,
                     ( ossAtomicSigned64::valueType )val ) ;
      }

      valueType swapGreaterThan( valueType val )
      {
         return V.swapPredicated( val,
                                  ossAtomicPredicateGreaterThan<valueType>() );
      }

      valueType swapLesserThan( valueType val )
      {
         return V.swapPredicated( val,
                                  ossAtomicPredicateLesserThan<valueType>() ) ;
      }

      BOOLEAN compare( valueType compVal )
      {
         return V.compare( ( ossAtomicSigned64::valueType )compVal ) ;
      }

      void poke( valueType val )
      {
         V.poke( ( ossAtomicSigned64::valueType )val ) ;
      }
} ;
typedef class _ossAtomic64 ossAtomic64 ;

#if defined OSS_ARCH_64
   typedef ossAtomic64 ossAtomicPtr ;
#elif defined OSS_ARCH_32
   typedef ossAtomic32 ossAtomicPtr ;
#endif


#endif
