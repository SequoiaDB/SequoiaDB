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

   Source File Name = dmsPageMap.hpp

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
#ifndef DMS_PAGE_MAP_HPP__
#define DMS_PAGE_MAP_HPP__

#include "dms.hpp"
#include "ossAtomic.hpp"
#include <map>

namespace engine
{

   /*
      _dmsPageMap define
   */
   class _dmsPageMap : public SDBObject
   {
      friend class _dmsPageMapUnit ;

      public:
         typedef std::map< dmsExtentID, dmsExtentID >    MAP_PAGES ;
         typedef MAP_PAGES::iterator                     MAP_PAGES_IT ;
         typedef MAP_PAGES::const_iterator               MAP_PAGES_CIT ;

      public:
         _dmsPageMap() ;
         ~_dmsPageMap() ;

         BOOLEAN  isEmpty() ;
         UINT64   size() ;

         void  addItem( dmsExtentID src, dmsExtentID dst ) ;
         void  rmItem( dmsExtentID src ) ;
         void  clear() ;

         BOOLEAN findItem( dmsExtentID src, dmsExtentID *pDst ) const ;

         MAP_PAGES_IT begin() ;
         MAP_PAGES_IT end() ;
         void         erase( MAP_PAGES_IT pos ) ;

      private:
         void              _setInfo( ossAtomic64 *pTotalSize,
                                     ossAtomic32 *pNonEmptyNum ) ;

      private:
         MAP_PAGES         _mapPages ;
         ossAtomic64       _size ;
         ossAtomic64       *_pTotalSize ;
         ossAtomic32       *_pNonEmptyNum ;

   } ;
   typedef _dmsPageMap dmsPageMap ;

   /*
      _dmsPageMapUnit define
   */
   class _dmsPageMapUnit : public SDBObject
   {
      public:
         _dmsPageMapUnit() ;
         ~_dmsPageMapUnit() ;

         BOOLEAN        isEmpty() ;
         UINT64         totalSize() ;
         UINT32         nonEmptyNum() ;

         void           clear() ;

         dmsPageMap*    getMap( UINT16 slot ) ;

         dmsPageMap*    beginNonEmpty( UINT16 &slot ) ;
         dmsPageMap*    nextNonEmpty( UINT16 &slot ) ;

      private:
         dmsPageMap                 _mapSlot[ DMS_MME_SLOTS ] ;
         ossAtomic64                _totalSize ;
         ossAtomic32                _nonEmptyNum ;

   } ;
   typedef _dmsPageMapUnit dmsPageMapUnit ;

}

#endif /* DMS_PAGE_MAP_HPP__ */

