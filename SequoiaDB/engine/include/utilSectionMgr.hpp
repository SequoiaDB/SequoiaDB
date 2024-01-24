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

   Source File Name = utilSectionMgr.hpp

   Descriptive Name = section management

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/07/2019  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SECTION_MGR_HPP_
#define UTIL_SECTION_MGR_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"
#include "../bson/bson.h"
#include <vector>
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{
   class _utilSection : public SDBObject
   {
   public:
      _utilSection( INT64 begin, INT64 end ):_begin(begin), _end(end)
      {}

      _utilSection()
      {
         _begin = 0 ;
         _end = OSS_SINT64_MAX ;
      }

      ~_utilSection() {}

      _utilSection( const _utilSection &right )
      {
         _begin = right._begin ;
         _end = right._end ;
      }

      _utilSection& operator = ( const _utilSection &right )
      {
         _begin = right._begin ;
         _end = right._end ;

         return *this ;
      }

   public:
      OSS_INLINE INT64 begin() const { return _begin ; }
      OSS_INLINE INT64 end() const { return _end ; }
      OSS_INLINE void setBegin( INT64 begin ) { _begin = begin ; }
      OSS_INLINE void setEnd( INT64 end ) { _end = end ; }

      OSS_INLINE INT64 length() const { return _end - _begin ; }

   private:
      INT64 _begin ;
      INT64 _end ;
   } ;

   typedef _utilSection utilSection ;

   class _utilSectionMgr : public SDBObject
   {
   public:
      typedef ossPoolMap< INT64, _utilSection > SECTION_MAP_TYPE ;
      typedef SECTION_MAP_TYPE::iterator SECTION_MAP_ITERATOR ;

      _utilSectionMgr() ;
      ~_utilSectionMgr() ;

   public:
      INT32 addSection( INT64 offset, INT64 length ) ;

      BOOLEAN isContain( INT64 offset, INT64 length,
                         BOOLEAN mustFullContain = TRUE,
                         INT64 *continuousEnd = NULL ) ;

      BOOLEAN isEmpty() const ;

      INT32 toBSONObjBuilder( BSONObjBuilder &builder ) ;

      INT32 fromBSONObj( const BSONObj &obj ) ;

   public:
      class const_iterator: public SDBObject
      {
         friend class _utilSectionMgr ;

      public:
         const_iterator()
         {
         }

         const_iterator( const const_iterator& other )
         {
            _sectionListIter = other._sectionListIter ;
         }

         bool operator== ( const const_iterator& other ) const
         {
            return ( _sectionListIter == other._sectionListIter ) ;
         }

         bool operator!= ( const const_iterator& other ) const
         {
            return ( _sectionListIter != other._sectionListIter ) ;
         }

         const_iterator& operator= ( const const_iterator& other )
         {
            _sectionListIter = other._sectionListIter ;
            return *this ;
         }

         const_iterator& operator++ ()
         {
            _sectionListIter++;
            return *this ;
         }

         const_iterator operator++ ( int )
         {
            const_iterator tmp( *this ) ;
            ++(*this) ;
            return tmp ;
         }

         const_iterator& operator-- ()
         {
            _sectionListIter--;
            return *this ;
         }

         const_iterator operator-- ( int )
         {
            const_iterator tmp( *this ) ;
            --(*this) ;
            return tmp ;
         }

         const _utilSection* operator-> ()
         {
            return &(_sectionListIter->second) ;
         }

         const _utilSection& operator* ()
         {
            return _sectionListIter->second ;
         }
      private:
         const_iterator( SECTION_MAP_TYPE::const_iterator it )
         {
            _sectionListIter = it ;
         }

      private:
         SECTION_MAP_TYPE::const_iterator _sectionListIter ;
      } ;

      const_iterator begin() const
      {
         return const_iterator( _sectionMap.begin() );
      }

      const_iterator end() const
      {
         return const_iterator( _sectionMap.end() );
      }

   private:
      void _addSection( INT64 offset, INT64 length ) ;
      void _saveContinuousEnd( INT64 *continuousEnd, INT64 end ) ;

   private:
      SECTION_MAP_TYPE _sectionMap ;
   } ;

   typedef _utilSectionMgr utilSectionMgr ;
}

#endif /* UTIL_SECTION_MGR_HPP_ */

