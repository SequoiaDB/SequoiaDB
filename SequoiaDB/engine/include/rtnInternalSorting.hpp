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

   Source File Name = rtnInternalSorting.hpp

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

#ifndef RTNINTERNALSORTING_HPP_
#define RTNINTERNALSORTING_HPP_

#include "rtnSortDef.hpp"
#include "rtnSortTuple.hpp"
#include "ixmIndexKey.hpp"
#include "../bson/ordering.h"


namespace engine
{
   class _pmdEDUCB ;

   class _rtnInternalSorting : public SDBObject
   {
   public:
      _rtnInternalSorting( const BSONObj &orderby,
                           CHAR *buf, UINT64 size,
                           INT64 limit ) ;
      virtual ~_rtnInternalSorting() ;

   public:
      INT32 push( const BSONObj& keyObj, const CHAR* obj,
                  INT32 objLen, BSONElement* arrElement ) ;

      void clearBuf() ;

      INT32 sort( _pmdEDUCB *cb ) ;

      BOOLEAN more() const
      {
         if ( _limit > 0 && _fetched >= (UINT64)_limit )
         {
            return FALSE ;
         }
         return  _fetched < _objNum ;
      }

      INT32 next( _rtnSortTuple **tuple ) ;

      UINT64 getObjNum () { return _objNum ; }


   private:
      INT32 _quickSort( _rtnSortTuple **left,
                        _rtnSortTuple **right,
                        _pmdEDUCB *cb ) ;

      INT32 _partition( _rtnSortTuple **left,
                        _rtnSortTuple **right,
                        _rtnSortTuple **&leftAxis,
                        _rtnSortTuple **&rightAxis ) ;

      INT32 _insertSort( _rtnSortTuple **left,
                         _rtnSortTuple **right ) ;


      INT32 _swapLeftSameKey( _rtnSortTuple **left,
                              _rtnSortTuple **right,
                              _rtnSortTuple **&axis ) ;

      INT32 _swapRightSameKey( _rtnSortTuple **left,
                               _rtnSortTuple **right,
                               _rtnSortTuple **&axis ) ;


   private:
      bson::Ordering _order ;
      CHAR *_begin ;
      UINT64 _totalSize ;
      UINT64 _headOffset ;
      UINT64 _tailOffset ;
      UINT64 _objNum ;
      UINT64 _fetched ;
      UINT64 _recursion ;
      INT64  _limit ;
   } ;
}

#endif

