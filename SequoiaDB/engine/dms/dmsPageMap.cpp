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

   Source File Name = dmsPageMap.cpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/26/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsPageMap.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _dmsPageMap implement
   */
   _dmsPageMap::_dmsPageMap()
   :_size( 0 )
   {
      _pTotalSize    = NULL ;
      _pNonEmptyNum  = NULL ;
   }

   _dmsPageMap::~_dmsPageMap()
   {
      SDB_ASSERT( 0 == _mapPages.size(), "Map must be empty" ) ;
   }

   void  _dmsPageMap::addItem( dmsExtentID src, dmsExtentID dst )
   {
      MAP_PAGES_IT it = _mapPages.find( src ) ;
      if ( it == _mapPages.end() )
      {
         /// insert
         _mapPages[ src ] = dst ;

         if ( 0 == _size.inc() )
         {
            _pNonEmptyNum->inc() ;
         }
         _pTotalSize->inc() ;
      }
      else
      {
         /// update
         it->second = dst ;
      }
   }

   void  _dmsPageMap::rmItem( dmsExtentID src )
   {
      MAP_PAGES_IT it = _mapPages.find( src ) ;
      if ( it != _mapPages.end() )
      {
         /// find
         _mapPages.erase( it ) ;

         if ( 1 == _size.dec() )
         {
            _pNonEmptyNum->dec() ;
         }
         _pTotalSize->dec() ;
      }
   }

   void _dmsPageMap::clear()
   {
      _mapPages.clear() ;

      _pTotalSize->sub( _size.fetch() ) ;

      if ( _size.swapLesserThan( 0 ) > 0 )
      {
         _pNonEmptyNum->dec() ;
      }
   }

   BOOLEAN _dmsPageMap::findItem( dmsExtentID src, dmsExtentID *pDst ) const
   {
      MAP_PAGES_CIT cit = _mapPages.find( src ) ;
      if ( cit != _mapPages.end() )
      {
         if ( pDst )
         {
            *pDst = cit->second ;
         }
         return TRUE ;
      }
      return FALSE ;
   }

   UINT64 _dmsPageMap::size()
   {
      return (UINT64)_size.fetch() ;
   }

   BOOLEAN _dmsPageMap::isEmpty()
   {
      return _size.compare( 0 ) ? TRUE : FALSE ;
   }

   void _dmsPageMap::_setInfo( ossAtomic64 *pTotalSize,
                               ossAtomic32 *pNonEmptyNum )
   {
      _pTotalSize = pTotalSize ;
      _pNonEmptyNum = pNonEmptyNum ;
   }

   _dmsPageMap::MAP_PAGES_IT _dmsPageMap::begin()
   {
      return _mapPages.begin() ;
   }

   _dmsPageMap::MAP_PAGES_IT _dmsPageMap::end()
   {
      return _mapPages.end() ;
   }

   void _dmsPageMap::erase( _dmsPageMap::MAP_PAGES_IT pos )
   {
      _mapPages.erase( pos ) ;

      if ( 1 == _size.dec() )
      {
         _pNonEmptyNum->dec() ;
      }
      _pTotalSize->dec() ;
   }

   /*
      _dmsPageMapUnit implement
   */
   _dmsPageMapUnit::_dmsPageMapUnit()
   :_totalSize( 0 ), _nonEmptyNum( 0 )
   {
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _mapSlot[ i ]._setInfo( &_totalSize, &_nonEmptyNum ) ;
      }
   }

   _dmsPageMapUnit::~_dmsPageMapUnit()
   {
   }

   BOOLEAN _dmsPageMapUnit::isEmpty()
   {
      return _totalSize.compare( 0 ) ? TRUE : FALSE ;
   }

   UINT64 _dmsPageMapUnit::totalSize()
   {
      return (UINT64)_totalSize.fetch() ;
   }

   UINT32 _dmsPageMapUnit::nonEmptyNum()
   {
      return (UINT32)_nonEmptyNum.fetch() ;
   }

   void _dmsPageMapUnit::clear()
   {
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _mapSlot[ i ].clear() ;
      }
   }

   dmsPageMap* _dmsPageMapUnit::getMap( UINT16 slot )
   {
      if ( slot < DMS_MME_SLOTS )
      {
         return &( _mapSlot[ slot ] ) ;
      }
      return NULL ;
   }

   dmsPageMap* _dmsPageMapUnit::beginNonEmpty( UINT16 &slot )
   {
      UINT16 i = 0 ;
      while( i < DMS_MME_SLOTS )
      {
         if ( !_mapSlot[ i ].isEmpty() )
         {
            slot = i ;
            return &( _mapSlot[ i ] ) ;
         }
         ++i ;
      }
      return NULL ;
   }

   dmsPageMap* _dmsPageMapUnit::nextNonEmpty( UINT16 &slot )
   {
      while( ++slot < DMS_MME_SLOTS )
      {
         if ( !_mapSlot[ slot ].isEmpty() )
         {
            return &( _mapSlot[ slot ] ) ;
         }
      }
      return NULL ;
   }

}

