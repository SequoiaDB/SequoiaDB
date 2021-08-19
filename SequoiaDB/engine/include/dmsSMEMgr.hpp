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

   Source File Name = dmsSMPMgr.hpp

   Descriptive Name = Data Management Service Space Management Extent Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains declare for
   space management extent manager, which is reponsible for fast free extent
   lookup and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/17/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSMEMGR_HPP__
#define DMSSMEMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include <list>
#include <vector>

using namespace std ;

namespace engine
{

   typedef UINT32 _dmsSegmentNode ; // high: start, low: size

   #define DMS_SEGMENT_NODE_GETSTART(x)   ((UINT16)((x)>>16))
   #define DMS_SEGMENT_NODE_GETSIZE(x)    ((UINT16)(0xFFFF&(x)))
   #define DMS_SEGMENT_NODE_SETSTART(x,y) (x=(((UINT32)(y)<<16)|(0xFFFF&(x))))
   #define DMS_SEGMENT_NODE_SETSIZE(x,y)  (x=(((UINT16)(y))|(0xFFFF0000&(x))))
   #define DMS_SEGMENT_NODE_SET(x,y,z)    (x=(((UINT32)(y)<<16)|(0xFFFF&(z))))

   class _dmsSMEMgr ;
   /*
      _dmsSegmentSpace : defined
   */
   class _dmsSegmentSpace : public SDBObject
   {
   private :
      list<_dmsSegmentNode>   _freeSpaceList ;
      UINT16                  _maxNode ;
      dmsExtentID             _startExtent ;
      UINT16                  _totalSize ;
      UINT16                  _totalFree ;

      ossSpinXLatch           _mutex ;
      _dmsSMEMgr              *_pSMEMgr ;

      // caller must hold exclusive latch
      void  _resetMax () ;

   public :
      explicit _dmsSegmentSpace ( dmsExtentID startExtent, UINT16 maxNode,
                                  _dmsSMEMgr *pSMEMgr ) ;
      ~_dmsSegmentSpace () ;

      INT32 reservePages ( UINT16 numPages, dmsExtentID &foundPage,
                           UINT32 pos, ossAtomic32 *pFreePos ) ;
      INT32 releasePages ( dmsExtentID start, UINT16 numPages,
                           BOOLEAN bitSet = TRUE ) ;

      UINT16 totalFree() ;

   } ;
   typedef class _dmsSegmentSpace dmsSegmentSpace ;

   struct _dmsSpaceManagementExtent ;
   class  _dmsStorageBase ;
   /*
      _dmsSMEMgr : defined
   */
   class _dmsSMEMgr : public SDBObject
   {
      friend class _dmsSegmentSpace ;

   private :
      vector<dmsSegmentSpace*>   _segments ;
      ossRWMutex                 _mutex ;
      UINT32                     _pageSize ;
      _dmsStorageBase            *_pStorageBase ;
      _dmsSpaceManagementExtent  *_pSME ;
      ossAtomic32                _totalFree ;
      ossAtomic32                _freePos ;

   public :
      _dmsSMEMgr() ;
      ~_dmsSMEMgr() ;

      INT32 init ( _dmsStorageBase *pStorageBase,
                   _dmsSpaceManagementExtent *pSME ) ;

      // attempt to reserve numPages pages from smp, if no more pages can be
      // found in existing pages, foundPage is set to DMS_INVALID_EXTENT
      INT32 reservePages ( UINT16 numPages, dmsExtentID &foundPage,
                           UINT32 *pSegmentNum = NULL ) ;

      // release numPages pages from page dmsExtentID
      INT32 releasePages ( dmsExtentID start, UINT16 numPages ) ;

      // deposit free pages into manager, this is called only by _extendSegments
      INT32 depositASegment ( dmsExtentID start ) ;

      UINT32 segmentNum () ;
      UINT32 totalFree () const ;

   } ;
   typedef class _dmsSMEMgr dmsSMEMgr ;
}

#endif //DMSSMEMGR_HPP__
