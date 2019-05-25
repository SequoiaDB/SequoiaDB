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

   Source File Name = rtnLobSections.cpp

   Descriptive Name = LOB section access management

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/27/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobSections.hpp"
#include "pd.hpp"
#include "msgDef.h"
#include <sstream>

using namespace bson ;

namespace engine
{
   INT32 _rtnLobSection::toBSONObj( bson::BSONObj& obj, BOOLEAN withAccessId ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;

         builder.append( FIELD_NAME_LOB_OFFSET, offset ) ;
         builder.append( FIELD_NAME_LOB_LENGTH, length ) ;
         if ( withAccessId )
         {
            builder.append( FIELD_NAME_LOB_ACCESSID, accessId ) ;
         }

         obj = builder.obj() ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobSection::fromBSONObj( bson::BSONObj& obj, INT64 accessId )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement ele ;

         ele = obj.getField( FIELD_NAME_LOB_OFFSET ) ;
         if ( NumberLong != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid \"%s\" field type: %d",
                    FIELD_NAME_LOB_OFFSET, ele.type() ) ;
            goto error ;
         }
         offset = ele.numberLong() ;

         ele = obj.getField( FIELD_NAME_LOB_LENGTH ) ;
         if ( NumberLong != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid \"%s\" field type: %d",
                    FIELD_NAME_LOB_LENGTH, ele.type() ) ;
            goto error ;
         }
         length = ele.numberLong() ;

         if ( -1 != accessId )
         {
            this->accessId = accessId ;
         }
         else
         {
            ele = obj.getField( FIELD_NAME_LOB_ACCESSID ) ;
            if ( NumberLong == ele.type() )
            {
               this->accessId = ele.numberLong() ;
            }
            else if ( EOO != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid \"%s\" field type: %d",
                       FIELD_NAME_LOB_ACCESSID, ele.type() ) ;
               goto error ;
            }
         }
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnLobSections::_rtnLobSections()
   {
   }

   _rtnLobSections::~_rtnLobSections()
   {
   }

   std::string _rtnLobSections::toString() const
   {
      std::stringstream ss ;
      INT32 size = _sections.size() ;
      INT32 i = 0 ;

      ss << "{ " ;

      LOB_SECTIONS_TYPE::const_iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; iter++, i++ )
      {
         const _rtnLobSection& cur = iter->second ;
         ss << i << ": "
            << "[" << cur.offset << ", " << cur.length << ", " << cur.accessId << "]" ;
         if ( i < size - 1 )
         {
            ss << ", " ;
         }
      }

      ss << " }" ;

      return ss.str() ;
   }

   BOOLEAN _rtnLobSections::overlapped( const _rtnLobSection& section ) const
   {
      SDB_ASSERT( section.offset >= 0 &&
                  section.length > 0 &&
                  section.accessId >= 0, "invalid section" ) ;

      BOOLEAN overlapped = FALSE ;

      LOB_SECTIONS_TYPE::const_iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; iter++ )
      {
         const _rtnLobSection& cur = iter->second ;
         if ( cur.overlapped( section ) )
         {
            if ( cur.accessId != section.accessId )
            {
               overlapped = TRUE ;
               break ;
            }
         }
      }

      return overlapped ;
   }

   BOOLEAN _rtnLobSections::completelyContains( const _rtnLobSection& section ) const
   {
      _rtnLobSection sec = section ;
      BOOLEAN completely = FALSE ;

      SDB_ASSERT( section.offset >= 0 &&
                  section.length > 0 &&
                  section.accessId >= 0, "invalid section" ) ;

      LOB_SECTIONS_TYPE::const_iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; iter++ )
      {
         const _rtnLobSection& cur = iter->second ;
         if ( sec.end() < cur.start() )
         {
            break ;
         }
         else if ( sec.start() > cur.end() )
         {
            continue ;
         }
         else // overlapped
         {
            SDB_ASSERT( cur.overlapped( sec ), "incorrect" ) ;

            if ( sec.start() < cur.start() )
            {
               break ;
            }
            else if ( sec.end() > cur.end() )
            {
               sec.length = sec.end() - cur.end() ;
               sec.offset = cur.end() + 1 ;
               continue ;
            }
            else
            {
               completely = TRUE ;
               break ;
            }
         }
      }

      return completely ;
   }

   BOOLEAN _rtnLobSections::conflicted( INT64 accessId ) const
   {
      SDB_ASSERT( accessId >= 0, "invlaid accessId" ) ;

      BOOLEAN cf = FALSE ;

      LOB_SECTIONS_TYPE::const_iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; iter++ )
      {
         const _rtnLobSection& cur = iter->second ;
         if ( cur.accessId != accessId )
         {
            cf = TRUE ;
            break ;
         }
      }

      return cf ;
   }

   _rtnLobSection _rtnLobSections::find( INT64 offset ) const
   {
      _rtnLobSection section ;

      SDB_ASSERT( offset >= 0, "invlaid offset" ) ;

      LOB_SECTIONS_TYPE::const_iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; iter++ )
      {
         const _rtnLobSection& cur = iter->second ;
         if ( cur.include( offset ) )
         {
            section = cur ;
            break ;
         }
      }

      return section ;
   }

   INT32 _rtnLobSections::addSection( const _rtnLobSection& section,
                                      std::vector<INT64>* offsets )
   {
      INT32 rc = SDB_OK ;
      std::vector<INT64> tmp ;

      std::vector<INT64>* off = offsets ;
      if ( NULL == off )
      {
         off = &tmp ;
      }

      try
      {
         rc = _addSection( section, *off ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( !off->empty() )
      {
         std::vector<INT64>::const_iterator iter ;
         for ( iter = offsets->begin() ; iter != offsets->end() ; iter++ )
         {
            delSectionByOffset( *iter ) ;
         }
         off->clear() ;
      }
      goto done ;
   }

   INT32 _rtnLobSections::_addSection( const _rtnLobSection& section,
                                       std::vector<INT64>& offsets )
   {
      INT32 rc = SDB_OK ;
      LOB_SECTIONS_TYPE tmp ;
      _rtnLobSection sec = section ;
      BOOLEAN overlapped = FALSE ;

      SDB_ASSERT( section.offset >= 0 &&
                  section.length > 0 &&
                  section.accessId >= 0, "invalid section" ) ;

      offsets.clear() ;

      LOB_SECTIONS_TYPE::const_iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; iter++ )
      {
         const _rtnLobSection& cur = iter->second ;
         if ( sec.end() < cur.start() )
         {
            if ( overlapped )
            {
               tmp.insert( LOB_SECTIONS_TYPE::value_type( sec.offset, sec ) ) ;
               sec.reset() ;
            }
            break ;
         }
         else if ( sec.start() > cur.end() )
         {
            continue ;
         }
         else // overlapped
         {
            SDB_ASSERT( cur.overlapped( sec ), "incorrect" ) ;
            overlapped = TRUE ;

            if ( cur.accessId != sec.accessId )
            {
               rc = SDB_LOB_LOCK_CONFLICTED ;
               goto error ;
            }

            if ( sec.start() < cur.start() )
            {
               _rtnLobSection lower( sec ) ;
               lower.length = cur.start() - sec.start() ;
               tmp.insert( LOB_SECTIONS_TYPE::value_type( lower.offset, lower ) ) ;

               if ( sec.end() > cur.end() )
               {
                  sec.length = sec.end() - cur.end() ;
                  sec.offset = cur.end() + 1 ;
                  continue ;
               }
               else
               {
                  sec.reset() ;
                  break ;
               }
            }
            else if ( sec.end() > cur.end() )
            {
               sec.length = sec.end() - cur.end() ;
               sec.offset = cur.end() + 1 ;
               continue ;
            }
            else
            {
               sec.reset() ;
               break ;
            }
         }
      }

      if ( overlapped )
      {
         LOB_SECTIONS_TYPE::const_iterator iter ;
         for ( iter = tmp.begin() ; iter != tmp.end() ; iter++ )
         {
            const _rtnLobSection& cur = iter->second ;
            offsets.push_back( cur.offset ) ;
            _sections.insert( LOB_SECTIONS_TYPE::value_type( cur.offset, cur ) ) ;
         }
         if ( sec.valid() )
         {
            offsets.push_back( sec.offset ) ;
            _sections.insert( LOB_SECTIONS_TYPE::value_type( sec.offset, sec ) ) ;
         }
      }
      else
      {
         offsets.push_back( sec.offset ) ;
         _sections.insert( LOB_SECTIONS_TYPE::value_type( sec.offset, sec ) ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLobSections::delSectionById( INT64 accessId )
   {
      LOB_SECTIONS_TYPE::iterator iter ;
      for ( iter = _sections.begin() ; iter != _sections.end() ; )
      {
         const _rtnLobSection& cur = iter->second ;
         if ( accessId == cur.accessId )
         {
            _sections.erase( iter++ ) ;
         }
         else
         {
            iter++ ;
         }
      }
   }

   void _rtnLobSections::delSectionByOffset( INT64 offset )
   {
      _sections.erase( offset ) ;
   }

   INT32 _rtnLobSections::saveTo( bson::BSONArray& array, BOOLEAN withAccessId ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONArrayBuilder builder ;

         for ( LOB_SECTIONS_TYPE::const_iterator iter = _sections.begin() ;
               iter != _sections.end() ; iter++ )
         {
            const _rtnLobSection& sec = iter->second ;
            BSONObj obj ;
            rc = sec.toBSONObj( obj, withAccessId ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to convert LobSection to BSONObj, rc=%d", rc ) ;
               goto error ;
            }
            builder.append( obj ) ;
         }

         array = builder.arr() ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobSections::readFrom( bson::BSONArray& array, INT64 accessId )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator iter( array );
         while ( iter.more() )
         {
            _rtnLobSection sec ;
            BSONObj obj ;

            BSONElement ele = iter.next() ;
            if ( Object != ele.type() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid element type: %d", ele.type() ) ;
               goto error ;
            }

            obj = ele.embeddedObject() ;

            rc = sec.fromBSONObj( obj, accessId ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to read LobSection from array, rc=%d", rc ) ;
               goto error ;
            }

            rc = addSection( sec ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to add LobSection, rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

