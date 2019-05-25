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

   Source File Name = rtnSubContext.hpp

   Descriptive Name = RunTime Sub-Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/28/2017   David Li  draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_SUB_CONTEXT_HPP_
#define RTN_SUB_CONTEXT_HPP_

#include "oss.hpp"
#include "../bson/bson.h"
#include "utilMap.hpp"
#include "ixmIndexKey.hpp"
#include "ixm_common.hpp"

using namespace bson ;

namespace engine
{
   class _rtnOrderKey : public SDBObject
   {
      typedef std::vector< BSONElement >  OrderKeyList;
      typedef std::vector< BSONElement >  OrderKeyEleList;
      typedef std::vector< BSONObj >      OrderKeyObjList;
      public:
         _rtnOrderKey( const _rtnOrderKey &orderKey ) ;
         _rtnOrderKey() ;

      public:
         BOOLEAN operator<( const _rtnOrderKey &rhs ) const ;
         void clear() ;
         void setOrderBy( const BSONObj &orderBy ) ;
         INT32 generateKey( const BSONObj &record,
                            _ixmIndexKeyGen *keyGen ) ;

      private:
         BSONObj              _orderBy ;
         ixmHashValue         _hash ;
         BSONObj              _keyObj ;
         BSONElement          _arrEle ;
   } ;
   typedef _rtnOrderKey rtnOrderKey ;

   class _rtnSubContext: public SDBObject
   {
   public:
      _rtnSubContext( const BSONObj& orderBy,
                      _ixmIndexKeyGen* keyGen,
                      INT64 contextID ) ;
      virtual ~_rtnSubContext() ;

   public:
      OSS_INLINE INT64     contextID () const { return _contextID ; }

   public:
      virtual const CHAR*  front() = 0 ;
      virtual INT32        pop() = 0 ;
      virtual INT32        popN( INT32 num ) = 0 ;
      virtual INT32        popAll() = 0 ;
      virtual INT32        recordNum() = 0 ;
      virtual INT32        remainLength() = 0 ;
      virtual INT32        truncate ( INT32 num ) = 0 ;
      virtual INT32        getOrderKey( _rtnOrderKey& orderKey ) = 0 ;

      virtual INT64        getDataID () const = 0 ;
      INT64                getProcessType () const { return _startFrom ; }

   protected:
      _rtnOrderKey      _orderKey ;
      BOOLEAN           _isOrderKeyChange ;
      INT64             _contextID ;
      INT64             _startFrom ;
      _ixmIndexKeyGen*  _keyGen ;
   } ;
   typedef _rtnSubContext rtnSubContext ;
}

#endif /* RTN_SUB_CONTEXT_HPP_ */

