/*******************************************************************************

   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobSections.hpp

   Descriptive Name = LOB section access management

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/27/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOB_SECTIONS_HPP_
#define RTN_LOB_SECTIONS_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include <map>
#include <vector>
#include <string>

namespace engine
{
   struct _rtnLobSection: public SDBObject
   {
      INT64 offset ;
      INT64 length ;
      INT64 accessId ;

      _rtnLobSection()
         : offset( -1 ),
           length( -1 ),
           accessId( -1 )
      {
      }

      _rtnLobSection( INT64 off, INT64 len, INT64 id )
         : offset( off ),
           length( len ),
           accessId( id )
      {
      }

      _rtnLobSection( const _rtnLobSection& other )
      {
         offset = other.offset ;
         length = other.length ;
         accessId = other.accessId ;
      }

      void operator=( const _rtnLobSection& other )
      {
         offset = other.offset ;
         length = other.length ;
         accessId = other.accessId ;
      }

      OSS_INLINE void reset()
      {
         offset = -1 ;
         length = -1 ;
      }

      OSS_INLINE INT64 start() const
      {
         return offset ;
      }

      OSS_INLINE INT64 end() const
      {
         return ( offset + length - 1 ) ;
      }

      OSS_INLINE BOOLEAN valid() const
      {
         return ( ( offset >= 0 && length > 0 ) ? TRUE : FALSE ) ;
      }

      OSS_INLINE BOOLEAN include( INT64 offset ) const
      {
         if ( offset >= start() && offset <= end() )
         {
            return TRUE ;
         }
         else
         {
            return FALSE ;
         }
      }

      OSS_INLINE BOOLEAN overlapped( const _rtnLobSection& other ) const
      {
         if ( other.include( start() ) ||
              other.include( end() ) )
         {
            return TRUE ;
         }
         else if ( include( other.start() ) ||
                   include( other.end() ) )
         {
            return TRUE ;
         }
         else
         {
            return FALSE ;
         }
      }

      INT32 toBSONObj( bson::BSONObj& obj, BOOLEAN withAccessId = FALSE ) const ;
      INT32 fromBSONObj( bson::BSONObj& obj, INT64 accessId = -1 ) ;
   } ;

   class _rtnLobSections: public SDBObject
   {
   private:
      typedef std::map<INT64, _rtnLobSection> LOB_SECTIONS_TYPE ;

   public:
      _rtnLobSections() ;
      ~_rtnLobSections() ;

   private:
      _rtnLobSections( const _rtnLobSections& ) ;
      void operator=( const _rtnLobSections& ) ;

   public:
      class iterator: public SDBObject
      {
         friend class _rtnLobSections ;

      public:
         iterator()
         {
         }

         iterator( const iterator& other )
         {
            iter = other.iter ;
         }

         bool operator== ( const iterator& other ) const
         {
            return ( iter == other.iter ) ;
         }

         bool operator!= ( const iterator& other ) const
         {
            return ( iter != other.iter ) ;
         }

         iterator& operator= ( const iterator& other )
         {
            iter = other.iter ;
            return *this ;
         }

         iterator& operator++ ()
         {
            iter++;
            return *this ;
         }

         iterator operator++ ( int )
         {
            iterator tmp( *this ) ;
            ++(*this) ;
            return tmp ;
         }

         iterator& operator-- ()
         {
            iter--;
            return *this ;
         }

         iterator operator-- ( int )
         {
            iterator tmp( *this ) ;
            --(*this) ;
            return tmp ;
         }

         const _rtnLobSection& operator-> ()
         {
            return iter->second ;
         }

         const _rtnLobSection& operator* ()
         {
            return iter->second ;
         }
      private:
         iterator( LOB_SECTIONS_TYPE::const_iterator it )
         {
            iter = it ;
         }

      private:
         LOB_SECTIONS_TYPE::const_iterator iter ;
      } ;

      iterator begin() const
      {
         return iterator( _sections.begin() );
      }

      iterator end() const
      {
         return iterator( _sections.end() );
      }

   public:
      OSS_INLINE BOOLEAN isEmpty() const
      {
         return _sections.empty() ? TRUE : FALSE ;
      }

      OSS_INLINE INT32 sectionNum() const
      {
         return (INT32)_sections.size() ;
      }

      std::string toString() const ;
      BOOLEAN overlapped( const _rtnLobSection& section ) const ;
      BOOLEAN completelyContains( const _rtnLobSection& section ) const ;
      BOOLEAN conflicted( INT64 accessId ) const ;
      _rtnLobSection find( INT64 offset ) const ;
      INT32   addSection( const _rtnLobSection& section,
                          std::vector<INT64>* offsets = NULL ) ;
      void    delSectionById( INT64 accessId ) ;
      void    delSectionByOffset( INT64 offset ) ;

      INT32 saveTo( bson::BSONArray& array, BOOLEAN withAccessId = FALSE ) const ;
      INT32 readFrom( bson::BSONArray& array, INT64 accessId = -1 ) ;

   private:
      INT32   _addSection( const _rtnLobSection& section,
                           std::vector<INT64>& offsets ) ;

   private:
      LOB_SECTIONS_TYPE       _sections ;
   } ;
}

#endif /* RTN_LOB_SECTIONS_HPP_ */

