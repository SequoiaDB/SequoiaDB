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
#include "utilCommBuff.hpp"

namespace engine
{
   class _pmdEDUCB ;

   /**
    * Internal sorting implementation.
    * It will use several blocks of memory, allocated dynamically by sort area.
    * This is to avoid allocating large fixed size memory from the beginning,
    * which will results in memory waste.
    */
   class _rtnInternalSorting : public utilPooledObject
   {
   public:
      _rtnInternalSorting( const BSONObj &orderby,
                           utilCommBuff *tupleDirectory,
                           utilCommBuff *tupleBuff,
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

      UINT32 maxRecordSize() const
      {
         return _maxRecordSize ;
      }

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
      utilCommBuff *_tupleDirectory ;
      utilCommBuff *_tupleBuff ;

      UINT64 _objNum ;
      UINT64 _fetched ;
      UINT64 _recursion ;
      INT64  _limit ;
      UINT32 _maxRecordSize ;
   } ;
}

#endif

