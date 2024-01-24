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

#include "utilPooledObject.hpp"
#include "../bson/bson.h"
#include "../bson/ordering.h"
#include "ixmIndexKey.hpp"
#include "ixm_common.hpp"

using namespace bson ;

namespace engine
{
   class _rtnOrderKey : public utilPooledObject
   {
      typedef std::vector< BSONElement >  OrderKeyList;
      typedef std::vector< BSONElement >  OrderKeyEleList;
      typedef std::vector< BSONObj >      OrderKeyObjList;
      public:
         _rtnOrderKey( const BSONObj &orderKey ) ;

      public:
         BOOLEAN operator<( const _rtnOrderKey &rhs ) const ;
         void clear() ;
         INT32 generateKey( const BSONObj &record,
                            _ixmIndexKeyGen *keyGen ) ;

      private:
         Ordering             _ordering ;
         ixmHashValue         _hash ;
         ixmKeyBuilder        _keyBuilder ;
         BSONObj              _keyObj ;
   } ;
   typedef _rtnOrderKey rtnOrderKey ;

   class _rtnSubContext: public utilPooledObject
   {
   public:
      _rtnSubContext( const BSONObj& orderBy,
                      _ixmIndexKeyGen* keyGen,
                      INT64 contextID ) ;
      virtual ~_rtnSubContext() ;

   public:
      OSS_INLINE INT64     contextID () const { return _contextID ; }
      OSS_INLINE void      setContextID( INT64 contextID )
      {
         _contextID = contextID ;
      }

   public:
      virtual const CHAR*  front() = 0 ;
      virtual INT32        pop() = 0 ;
      virtual INT32        popN( INT32 num ) = 0 ;
      virtual INT32        popAll() = 0 ;
      virtual INT32        pushFront( const BSONObj &obj ) = 0 ;
      virtual INT32        recordNum() = 0 ;
      virtual INT32        remainLength() = 0 ;
      virtual INT32        truncate ( INT32 num ) = 0 ;
      virtual INT32        genOrderKey() = 0 ;

      // For context data processor
      virtual INT64        getDataID () const = 0 ;
      INT64                getProcessType () const { return _startFrom ; }

      BOOLEAN operator<( const _rtnSubContext &rhs ) const
      {
         return _orderKey < rhs._orderKey ;
      }

   protected:
      _rtnOrderKey      _orderKey ;
      BOOLEAN           _isOrderKeyChange ;
      INT64             _contextID ;
      INT64             _startFrom ;
      _ixmIndexKeyGen*  _keyGen ;
   } ;
   typedef _rtnSubContext rtnSubContext ;

   class _rtnSubContextComperator
   {
   public:
      _rtnSubContextComperator() {}
      ~_rtnSubContextComperator() {}

      BOOLEAN operator() ( const rtnSubContext *l,
                           const rtnSubContext *r ) const
      {
         return l != r && *l < *r ;
      }
   } ;
   typedef class _rtnSubContextComperator rtnSubContextComperator ;

}

#endif /* RTN_SUB_CONTEXT_HPP_ */

