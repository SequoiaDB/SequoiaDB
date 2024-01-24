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

   Source File Name = utilSectionMgr.cpp

   Descriptive Name = section management

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/07/2019  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilSectionMgr.hpp"
#include "msgDef.hpp"
#include "pd.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _utilSectionMgr::_utilSectionMgr()
   {
   }

   _utilSectionMgr::~_utilSectionMgr()
   {
      _sectionMap.clear() ;
   }

   void _utilSectionMgr::_addSection( INT64 offset, INT64 length )
   {
      SECTION_MAP_ITERATOR iter ;
      _utilSection newSection( offset, offset + length ) ;
      if ( _sectionMap.size() == 0 )
      {
         _sectionMap[ newSection.begin() ] = newSection ;
         return ;
      }

      iter = _sectionMap.lower_bound( newSection.begin() ) ;
      if ( iter != _sectionMap.begin() )
      {
         // previou section may overlap the newSection
         // _sectionMap: [1,5) [10, 15)
         // newSection: [4, 20)
         // lower_bound: [10, 15), but we should deal with [1, 5) too
         --iter ;
      }

      while ( iter != _sectionMap.end() )
      {
         SDB_ASSERT( iter->first == iter->second.begin() ,
                     "should be equal" ) ;

         if ( newSection.begin() >= iter->second.end() )
         {
            ++iter ;
            continue ;
         }

         if ( newSection.end() == iter->second.begin() )
         {
            // perfect fit
            // iter: [10, 20)
            // new:  [5, 10)
            // result: [5, 20)
            newSection.setEnd( iter->second.end() ) ;
            _sectionMap[ newSection.begin() ] = newSection ;
            _sectionMap.erase( iter ) ;
            return ;
         }

         if ( newSection.end() < iter->second.begin() )
         {
            // iter: [10, 20)
            // new:  [5, 9)
            // result: [5, 9) [10, 20)
            _sectionMap[ newSection.begin() ] = newSection ;
            return ;
         }

         if ( newSection.begin() < iter->second.begin() )
         {
            if ( newSection.end() <= iter->second.end() )
            {
               // iter: [10, 20)
               // new:  [5, 18)
               // result: [5, 20)
               newSection.setEnd( iter->second.end() ) ;
               _sectionMap[ newSection.begin() ] = newSection ;
               _sectionMap.erase( iter ) ;
               return ;
            }

            // iter:   [10, 20)
            // new:    [5, 30)
            // result: [5, 20)  [20, 30)
            _utilSection insertSection( newSection.begin(),
                                        iter->second.end() ) ;
            _sectionMap[ insertSection.begin() ] = insertSection ;
            _sectionMap.erase( iter++ ) ;

            newSection.setBegin( insertSection.end() ) ;
         }
         else
         {
            if ( newSection.end() <= iter->second.end() )
            {
               // iter: [10, 20)
               // new:  [12,18)
               return ;
            }

            // iter:   [10, 20)
            // new:    [12, 30)
            // result: [10, 20)  [20, 30)
            newSection.setBegin( iter->second.end() ) ;
            ++iter ;
         }
      }

      _sectionMap[ newSection.begin() ] = newSection ;
   }

   INT32 _utilSectionMgr::addSection( INT64 offset, INT64 length )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _addSection( offset, length ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to add section, occur unexpected"
                 "error:%s", e.what() ) ;
         rc = SDB_OP_INCOMPLETE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilSectionMgr::_saveContinuousEnd( INT64 *continuousEnd, INT64 end )
   {
      if ( NULL != continuousEnd )
      {
         *continuousEnd = end ;
      }
   }

   BOOLEAN _utilSectionMgr::isContain( INT64 offset, INT64 length,
                                       BOOLEAN mustFullContain,
                                       INT64 *continuousEnd )
   {
      BOOLEAN hasContained = FALSE ;
      SECTION_MAP_ITERATOR iter ;
      _utilSection newSection( offset, offset + length ) ;
      if ( _sectionMap.size() == 0 )
      {
         return FALSE ;
      }

      iter = _sectionMap.lower_bound( newSection.begin() ) ;
      if ( iter != _sectionMap.begin() )
      {
         --iter ;
      }

      for ( ; iter != _sectionMap.end(); ++iter )
      {
         if ( newSection.begin() >= iter->second.end() )
         {
            // iter [10, 20)
            // new  [20, 30)
            continue ;
         }

         if ( newSection.begin() < iter->second.begin() )
         {
            // iter [10, 20)
            // new  [0,30) or [0,5) or [0, 18)
            break ;
         }

         if ( newSection.end() <= iter->second.end() )
         {
            // iter [10, 20)
            // new  [12, 18)
            // full contained, return here
            _saveContinuousEnd( continuousEnd, iter->second.end() ) ;
            return TRUE ;
         }

         // iter [10, 20)
         // new  [12, 30)

         // partitial contained
         hasContained = TRUE ;
         _saveContinuousEnd( continuousEnd, iter->second.end() ) ;

         // continue to check rest part [20, 30)
         newSection.setBegin( iter->second.end() ) ;
      }

      if ( mustFullContain )
      {
         return FALSE ;
      }
      else
      {
         if ( hasContained )
         {
            // partitial contained
            return TRUE ;
         }

         // contain nothing
         return FALSE ;
      }
   }

   BOOLEAN _utilSectionMgr::isEmpty() const
   {
      return _sectionMap.empty() ? TRUE : FALSE ;
   }

   INT32 _utilSectionMgr::toBSONObjBuilder( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      if ( isEmpty() )
      {
         goto done ;
      }

      try
      {
         BSONArrayBuilder sectionArrayBuilder(
               builder.subarrayStart( FIELD_NAME_LOB_LOCK_SECTIONS ) ) ;

         for ( SECTION_MAP_ITERATOR iter = _sectionMap.begin();
               iter != _sectionMap.end(); ++iter )
         {
            BSONObjBuilder sectionBuilder(sectionArrayBuilder.subobjStart()) ;
            sectionBuilder.append( FIELD_NAME_LOB_OFFSET,
                                   iter->second.begin() ) ;
            sectionBuilder.append( FIELD_NAME_LOB_LENGTH,
                                   iter->second.length() ) ;
            sectionBuilder.done() ;
         }

         sectionArrayBuilder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to append to builder, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilSectionMgr::fromBSONObj( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      BSONElement ele = obj.getField( FIELD_NAME_LOB_LOCK_SECTIONS ) ;
      if ( ele.eoo() )
      {
         _sectionMap.clear() ;
         goto done ;
      }

      if ( Array != ele.type() )
      {
         PD_LOG( PDERROR, "invalid LockSections type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
         _sectionMap.clear() ;
         INT64 offset ;
         INT64 length ;
         BSONObjIterator iter( ele.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONElement oneSectionEle = iter.next() ;
            if ( oneSectionEle.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "SectionEle must be Object type:type=%d",
                       oneSectionEle.type() ) ;
               goto error ;
            }

            BSONObj oneSecObj = oneSectionEle.embeddedObject() ;
            BSONElement offEle = oneSecObj.getField( FIELD_NAME_LOB_OFFSET ) ;
            if ( NumberLong != offEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid field type:name=%s,type=%d",
                       FIELD_NAME_LOB_OFFSET, offEle.type() ) ;
               goto error ;
            }
            offset = offEle.numberLong() ;

            BSONElement lenEle = oneSecObj.getField( FIELD_NAME_LOB_LENGTH ) ;
            if ( NumberLong != lenEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid field type:name=%s,type=%d",
                       FIELD_NAME_LOB_LENGTH, lenEle.type() ) ;
               goto error ;
            }
            length = lenEle.numberLong() ;

            rc = addSection( offset, length ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add section:offset=%lld,"
                         "length=%lld,rc=%d", offset, length, rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

