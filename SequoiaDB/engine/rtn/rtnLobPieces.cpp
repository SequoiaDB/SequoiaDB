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

   Source File Name = rtnLobPieces.cpp

   Descriptive Name = LOB piece info

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/11/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobPieces.hpp"
#include "pd.hpp"
#include <sstream>
#include <algorithm>

using namespace bson ;
using namespace std ;

namespace engine
{
   #define LOB_PIECES_INVALID_INDEX -1

   _rtnLobPiecesInfo::_rtnLobPiecesInfo()
   {
   }

   _rtnLobPiecesInfo::~_rtnLobPiecesInfo()
   {
      _sections.clear() ;
   }

   std::string _rtnLobPiecesInfo::toString() const
   {
      std::stringstream ss ;
      INT32 size = _sections.size() ;

      ss << "{ " ;

      for ( INT32 i = 0; i < size ; i++ )
      {
         const _rtnLobPieces& p = _sections[ i ] ;
         ss << i << ": "
            << "[" << p.first << ", " << p.last << "]" ;
         if ( i < size - 1 )
         {
            ss << ", " ;
         }
      }

      ss << " }" ;

      return ss.str() ;
   }

   INT32 _rtnLobPiecesInfo::_findPiece( UINT32 piece ) const
   {
      INT32 index = LOB_PIECES_INVALID_INDEX ;
      INT32 lower = 0 ;
      INT32 upper = _sections.size() - 1 ;

#ifdef _DEBUG
      for ( INT32 i = 1; i < (INT32)_sections.size() ; i++ )
      {
         const _rtnLobPieces& prev = _sections[ i - 1 ] ;
         const _rtnLobPieces& curr = _sections[ i ] ;

         SDB_ASSERT( curr.first <= curr.last, "incorrect range" ) ;
         SDB_ASSERT( prev.last < curr.first, "incorrect range" ) ;
      }
#endif

      while ( lower <= upper )
      {
         INT32 middle = ( lower + upper ) / 2 ;

         const _rtnLobPieces& p = _sections[ middle ] ;
         SDB_ASSERT( p.first <= p.last, "incorrect pieces" ) ;

         if ( piece < p.first )
         {
            upper = middle - 1 ;
         }
         else if ( piece > p.last )
         {
            lower = middle + 1 ;
         }
         else
         {
            SDB_ASSERT( p.contains( piece ), "impossible" ) ;
            index = middle ;
            break ;
         }
      }

      return index ;
   }

   INT32 _rtnLobPiecesInfo::_findNearestLeast( UINT32 piece ) const
   {
      INT32 middle = LOB_PIECES_INVALID_INDEX ;
      INT32 lower = 0 ;
      INT32 upper = _sections.size() - 1 ;

      SDB_ASSERT( !_sections.empty(), "sections should not be empty" ) ;

#ifdef _DEBUG
      for ( INT32 i = 1; i < (INT32)_sections.size() ; i++ )
      {
         const _rtnLobPieces& prev = _sections[ i - 1 ] ;
         const _rtnLobPieces& curr = _sections[ i ] ;

         SDB_ASSERT( curr.first <= curr.last, "incorrect piece" ) ;
         SDB_ASSERT( prev.last < curr.first, "incorrect section" ) ;
      }
#endif

      while ( lower <= upper )
      {
         middle = ( lower + upper ) / 2 ;

         const _rtnLobPieces& p = _sections[ middle ] ;
         SDB_ASSERT( p.first <= p.last, "incorrect pieces" ) ;

         if ( piece < p.first )
         {
            upper = middle -1 ;
         }
         else if ( piece > p.last )
         {
            lower = middle + 1 ;
         }
         else
         {
            SDB_ASSERT( p.contains( piece ), "impossible" ) ;
            goto done ;
         }
      }

      SDB_ASSERT( middle >= 0 && middle <= (INT32)_sections.size() - 1, "invalid index" ) ;

      {
         const _rtnLobPieces& p = _sections[ middle ] ;
         if ( piece > p.first )
         {
            while ( middle < (INT32)_sections.size() - 1 )
            {
               if ( piece > _sections[ middle + 1 ].first )
               {
                  middle++ ;
               }
               else
               {
                  break ;
               }
            }
         }
         else if ( piece < p.first )
         {
            middle--;

            while ( middle >= 0 )
            {
               if ( piece < _sections[ middle ].first )
               {
                  middle-- ;
               }
               else
               {
                  break ;
               }
            }
         }
      }

   done:
      SDB_ASSERT( middle >= -1 && middle <= (INT32)_sections.size() - 1, "invalid index" ) ;
      return middle ;
   }

   INT32 _rtnLobPiecesInfo::addPiece( UINT32 piece )
   {
      INT32 rc = SDB_OK ;
      INT32 index = LOB_PIECES_INVALID_INDEX ;

      try
      {
         if ( _sections.empty() )
         {
            _sections.push_back( _rtnLobPieces( piece, piece ) ) ;
            goto done ;
         }

         index = _findNearestLeast( piece ) ;
         if ( index < 0 )
         {
            _rtnLobPieces& p = _sections[ 0 ] ;
            if ( piece + 1 == p.first )
            {
               p.first = piece ;
            }
            else
            {
               _sections.insert( _sections.begin(), _rtnLobPieces( piece, piece ) ) ;
            }
            goto done ;
         }
         else
         {
            _rtnLobPieces& p = _sections[ index ] ;
            SDB_ASSERT( piece >= p.first, "incorrect index" ) ;
            if ( p.contains( piece ) ) // inside
            {
            }
            else if ( p.last + 1 == piece ) // close to the right border
            {
               p.last = piece ;
               if ( index < (INT32)_sections.size() - 1 )
               {
                  _rtnLobPieces& next = _sections[ index + 1 ] ;
                  if ( piece + 1 == next.first )
                  {
                     p.last = next.last ;
                     _sections.erase( _sections.begin() + index + 1 ) ;
                  }
               }
            }
            else if ( index == (INT32)_sections.size() - 1 ) // beyond, at the last
            {
               _sections.push_back( _rtnLobPieces( piece, piece ) ) ;
            }
            else if ( piece + 1 == _sections[ index + 1 ].first ) // close to the left border
            {
               _sections[ index + 1 ].first = piece ;
            }
            else // in the gap between the current and next section
            {
               _sections.insert( _sections.begin() + index + 1, _rtnLobPieces( piece, piece ) ) ;
            }
         }
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

#ifdef _DEBUG
      for ( INT32 i = 1; i < (INT32)_sections.size() ; i++ )
      {
         const _rtnLobPieces& prev = _sections[ i - 1 ] ;
         const _rtnLobPieces& curr = _sections[ i ] ;

         SDB_ASSERT( curr.first <= curr.last, "incorrect piece" ) ;
         SDB_ASSERT( prev.last < curr.first, "incorrect section" ) ;
      }
#endif

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobPiecesInfo::addPieces( const _rtnLobPieces& pieces )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( pieces.first <= pieces.last, "incorrect piece" ) ;
      if ( pieces.first > pieces.last )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _sections.empty() )
      {
         _sections.push_back( pieces ) ;
         goto done ;
      }

      for ( UINT32 i = pieces.first ; i <= pieces.last ; i++ )
      {
         rc = addPiece( i ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobPiecesInfo::delPiece( UINT32 piece )
   {
      INT32 rc = SDB_OK ;
      INT32 index = LOB_PIECES_INVALID_INDEX ;

      if ( _sections.empty() )
      {
         goto done ;
      }

      try
      {
         index = _findPiece( piece ) ;
         if ( LOB_PIECES_INVALID_INDEX == index )
         {
            goto done ;
         }
         else
         {
            _rtnLobPieces& p = _sections[ index ] ;
            SDB_ASSERT( p.contains( piece ), "incorrect index" ) ;

            if ( p.first == p.last ) // only this piece
            {
               SDB_ASSERT( piece == p.first, "incorrect index" ) ;
               _sections.erase( _sections.begin() + index ) ;
            }
            else if ( piece == p.first ) // left border
            {
               p.first = piece + 1 ;
            }
            else if ( piece == p.last ) //right border
            {
               p.last = piece - 1 ;
            }
            else // inside
            {
               _sections.insert( _sections.begin() + index + 1, _rtnLobPieces( piece + 1, p.last ) ) ;
               p.last = piece - 1 ;
            }
         }
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

#ifdef _DEBUG
      for ( INT32 i = 1; i < (INT32)_sections.size() ; i++ )
      {
         const _rtnLobPieces& prev = _sections[ i - 1 ] ;
         const _rtnLobPieces& curr = _sections[ i ] ;

         SDB_ASSERT( curr.first <= curr.last, "incorrect piece" ) ;
         SDB_ASSERT( prev.last < curr.first, "incorrect section" ) ;
      }
#endif

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobPiecesInfo::delPieces( const _rtnLobPieces& pieces )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( pieces.first <= pieces.last, "incorrect piece" ) ;
      if ( pieces.first > pieces.last )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _sections.empty() )
      {
         goto done ;
      }

      for ( UINT32 i = pieces.last ; i >= pieces.first; i-- )
      {
         rc = delPiece( i ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnLobPiecesInfo::hasPiece( UINT32 piece ) const
   {
      INT32 index = _findPiece( piece ) ;
      if ( LOB_PIECES_INVALID_INDEX != index )
      {
         return TRUE ;
      }
      else
      {
         return FALSE ;
      }
   }

   _rtnLobPieces _rtnLobPiecesInfo::getSection( INT32 index ) const
   {
      SDB_ASSERT( index >= 0 && index < sectionNum(), "invalid index" ) ;
      return _sections.at( index ) ;
   }

   INT32 _rtnLobPiecesInfo::requiredMem() const
   {
      if ( empty() )
      {
         return 0 ;
      }

      return ( sectionNum() * (INT32)sizeof( _rtnLobPieces ) ) ;
   }

   INT32 _rtnLobPiecesInfo::saveTo( CHAR* buf, INT32 length ) const
   {
      INT32 rc = SDB_OK ;
      _rtnLobPieces* dest = (_rtnLobPieces*)buf ;

      SDB_ASSERT( NULL != buf, "buf is null" ) ;
      SDB_ASSERT( length >= requiredMem(), "length is not enough" ) ;

      if ( length < requiredMem() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      for ( INT32 i = 0 ; i < (INT32)_sections.size() ; i++ )
      {
         SDB_ASSERT( (CHAR*)&dest[i] - buf < length, "impossible" ) ;

         const _rtnLobPieces& pieces = _sections[ i ] ;
         dest[ i ] = pieces ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobPiecesInfo::readFrom( const CHAR* buf, INT32 length )
   {
      SDB_ASSERT( NULL != buf, "buf is null" ) ;
      SDB_ASSERT( length % sizeof(_rtnLobPieces) == 0, "incorrect length" ) ;

      _sections.clear() ;
      return mergeFrom( buf, length ) ;
   }

   INT32 _rtnLobPiecesInfo::mergeFrom( const CHAR* buf, INT32 length )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != buf, "buf is null" ) ;
      SDB_ASSERT( length % sizeof(_rtnLobPieces) == 0, "incorrect length" ) ;

      _rtnLobPieces* src = (_rtnLobPieces*)buf ;
      INT32 size = length / (INT32)sizeof( _rtnLobPieces ) ;

      for ( INT32 i = 0 ; i < size ; i++ )
      {
         rc = addPieces( src[ i ] ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read LOB pieces info from memory, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobPiecesInfo::saveTo( bson::BSONArray& array ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONArrayBuilder builder ;

         for ( LOB_PIECESINFO_TYPE::const_iterator iter = _sections.begin() ;
               iter != _sections.end() ; iter++ )
         {
            const _rtnLobPieces& pieces = *iter ;
            builder.append( (INT32)pieces.first ) ;
            builder.append( (INT32)pieces.last ) ;
         }

         array = builder.arr() ;

         SDB_ASSERT( array.nFields() == sectionNum() * 2, "invalid pieces info" ) ;
      }
      catch ( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 _rtnLobPiecesInfo::readFrom( const bson::BSONArray& array )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( array.nFields() % 2 == 0, "invalid array" ) ;

      try
      {
         BSONObjIterator iter( array );
         while ( iter.more() )
         {
            _rtnLobPieces pieces ;

            BSONElement ele = iter.next() ;
            SDB_ASSERT( NumberInt == ele.type(), "invalid type" ) ;
            pieces.first = (UINT32) ele.Int() ;

            if ( !iter.more() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "LOB pieces info in array is not paired" ) ;
               goto error ;
            }

            ele = iter.next() ;
            SDB_ASSERT( NumberInt == ele.type(), "invalid type" ) ;
            pieces.last = (UINT32) ele.Int() ;

            rc = addPieces( pieces ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to add pieces info from array, rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
