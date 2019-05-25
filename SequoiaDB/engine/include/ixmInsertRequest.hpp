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

   Source File Name = ixmInsertRequest.hpp

   Descriptive Name = Index Management Insert Request Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   for two phase index insertion request, which breaks index insertion into
   lookup phase and performAction phase. However this logic is not really used
   in the program in SequoiaDB Version 1. This logic will be further
   investigated and implemented in the future release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMINSERTREQUEST_HPP_
#define IXMINSERTREQUEST_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "../bson/ordering.h"
#include "ixm.hpp"
#include "ixmExtent.hpp"
#include "pd.hpp"
using namespace bson ;
namespace engine
{
   class ixmIndexInsertRequest : private boost::noncopyable, public SDBObject
   {
   public:
      enum Op { Nothing, SetUsed, InsertHere } ;
      virtual ~ixmIndexInsertRequest(){} ;
      virtual INT32 performAction() const = 0 ;
   } ;

   class ixmIndexInsertRequestImpl : public ixmIndexInsertRequest
   {
   public :
      dmsRecordID _recordRID ;
      ixmKeyOwned _key ;
      const Ordering _order ;
      ixmIndexCB *_indexCB ;
      Op _op ;
      UINT16 _pos ;
      ixmExtentHead *_extent ;
      dmsExtentID _extentID ;
      ixmIndexInsertRequestImpl ( dmsRecordID recordRID,
                                  const BSONObj &key, Ordering order,
                                  ixmIndexCB *indexCB ) :
      _key(key),
      _order(order)
      {
         _recordRID = recordRID ;
         _indexCB = indexCB ;
         _op = Nothing ;
      }

      INT32 performAction() const
      {
         if ( _op == Nothing ) return SDB_OK ;
         else if ( _op == InsertHere )
         {
            return ixmExtent((CHAR*)_extent, _extentID,
                              _indexCB->getSU()).insertHere
                              (  _pos, _recordRID, _key,
                              _order, DMS_INVALID_EXTENT, DMS_INVALID_EXTENT,
                              _indexCB ) ;
         }
         else if ( _op == SetUsed )
         {
            const ixmKeyNode *kn = ixmExtent((CHAR*)_extent, _extentID,
                   _indexCB->getSU()).getKeyNode(_pos) ;
            if ( !kn )
            {
               pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                       "failed to get key node for %d", _pos ) ;
               return SDB_SYS ;
            }
            ixmKeyNode *key = const_cast<ixmKeyNode*>(kn) ;
            key->setUsed() ;
            key->_rid = _recordRID ;
         }
         return SDB_INVALIDARG ;
      }
   } ;
}

#endif
