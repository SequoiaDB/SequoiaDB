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

   Source File Name = fapMongoDecimal.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==============================================
          2020/06/18  fangjiabin Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossUtil.hpp"
#include "../../bson/bson.hpp"
#include "fapMongodef.hpp"

#ifndef _FAP_MONGO_DECIMAL_HPP_
#define _FAP_MONGO_DECIMAL_HPP_

using namespace bson ;

namespace fap
{
// decimalBID means decimal in binary integer encoding
// It's based on IEEE 754-2008 standards.

#define FAP_MONGO_BSON_DECIMALBID_TYPE   19
// decimal in BID encoding has 128 bits
#define FAP_MONGO_DECIAMLOID_SIZE        16

class _mongoBSONObjIterator
{
public:
   _mongoBSONObjIterator( const BSONObj& jso )
   {
      int sz = jso.objsize() ;
      if ( 0 == sz )
      {
         _pos = _theend = 0 ;
         return ;
      }
      _pos = jso.objdata() + 4 ;
      _theend = jso.objdata() + sz - 1 ;
   }

   _mongoBSONObjIterator()
   {
      _pos    = NULL ;
      _theend = NULL ;
   }

   /** @return true if more elements exist to be enumerated. */
   bool more() { return _pos < _theend ; }

   BSONElement next()
   {
      assert( _pos <= _theend ) ;
      BSONElement e( _pos ) ;
      if ( FAP_MONGO_BSON_DECIMALBID_TYPE == e.type() )
      {
         // e.size = type(1) + fieldName(strlen(fieldName)+1) + dataSize(16)
         _pos +=
         ( 1 + ossStrlen( e.fieldName() ) + 1 + FAP_MONGO_DECIAMLOID_SIZE ) ;
      }
      else
      {
         _pos += e.size() ;
      }
      return e;
   }

   BSONElement operator*()
   {
      assert( _pos <= _theend ) ;
      return BSONElement( _pos ) ;
   }

private:
   const CHAR* _pos ;
   const CHAR* _theend ;
};
typedef _mongoBSONObjIterator mongoBSONObjIterator ;

INT32   sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                 BSONObjBuilder &mongoRecordBob,
                                 BOOLEAN &hasDecimal ) ;

INT32   sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                 BSONArrayBuilder &mongoRecordBab,
                                 BufBuilder &mongoRecordBb ) ;

INT32   mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                 BSONObjBuilder &sdbMsgObjBob ) ;

INT32   mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                 BSONArrayBuilder &sdbMsgObjBab ) ;

BOOLEAN mongoIsSupportDecimal( const mongoClientInfo &clientInfo ) ;
}

#endif