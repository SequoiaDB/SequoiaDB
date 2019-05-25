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

   Source File Name = rtnSorting.hpp

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

#ifndef RTNSORTING_HPP_
#define RTNSORTING_HPP_

#include "rtnSortDef.hpp"
#include "rtnMergeSorting.hpp"
#include "dmsTmpBlkUnit.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _rtnInternalSorting ;


   class _rtnSorting : public SDBObject
   {
   public:
      _rtnSorting();
      virtual ~_rtnSorting();

   public:
      INT32 init( UINT64 bufSize,
                  const BSONObj &orderby,
                  SINT64 fino,
                  SINT64 limit,
                  _pmdEDUCB *cb );

      INT32 push( const BSONObj& key, const CHAR* obj, INT32 objLen,
                  BSONElement* arrEle, _pmdEDUCB *cb ) ;

      INT32 sort( _pmdEDUCB *cb ) ;

      INT32 fetch( BSONObj &key, const CHAR** obj,
                   INT32* objLen, _pmdEDUCB *cb ) ;

      OSS_INLINE BOOLEAN isInMemorySort () const
      {
         return _blks.empty() ;
      }

   private:

      INT32 _moveToExternalBlks( _rtnInternalSorting *inter,
                                 RTN_SORT_BLKS &blks,
                                 _pmdEDUCB *cb ) ;

      INT32 _fetchFromInter( BSONObj &key, const CHAR** obj, INT32* objLen ) ;

      INT32 _fetchFromExter( BSONObj &next, const CHAR** obj, INT32* objLen,
                             _pmdEDUCB *cb ) ;
   private:
      _dmsTmpBlkUnit _unit ;
      BSONObj _orderby ;
      CHAR *_sortBuf ;
      UINT64 _totalBufSize ;
      RTN_SORT_STEP _step ;
      _pmdEDUCB *_cb ;
      _rtnInternalSorting *_internalBlk ;
      _rtnMergeSorting *_mergeBlk ;
      RTN_SORT_BLKS _blks ;
      UINT64 _blkBegin ;
      SINT64 _fino ;
      SINT64 _limit ;
      CHAR *_cpBuf ;
      const UINT32 _cpBufSize ;
   };

   typedef class _rtnSorting rtnSorting;
}

#endif

