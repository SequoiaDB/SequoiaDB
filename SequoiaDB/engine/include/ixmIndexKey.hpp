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

   Source File Name = ixmIndexKey.hpp

   Descriptive Name = Index Management Index Key Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index key generation from a given index definition and a data record.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMINDEXKEY_HPP_
#define IXMINDEXKEY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "pd.hpp"
#include <string>
#include <vector>

using namespace bson;

namespace engine
{
   /*
      IXM get undefine key object
   */
   BSONObj ixmGetUndefineKeyObj( INT32 fieldNum ) ;

   enum IndexSuitability { USELESS = 0 , HELPFUL = 1 , OPTIMAL = 2 };
   class _ixmIndexCB ;

   enum IXM_KEYGEN_TYPE
   {
      GEN_OBJ_NO_FIELD_NAME         = 1,
      GEN_OBJ_KEEP_FIELD_NAME       = 2,
      GEN_OBJ_ARRAY_FIELD_NAME      = 3
   } ;

   class _ixmIndexKeyGen : public SDBObject
   {
   protected:
      INT32 indexVersion() const ;
      IndexSuitability _suitability( const BSONObj& query ,
                                     const BSONObj& order ) const ;
      vector<const CHAR*> _fieldNames ; // vector contains all fields
      vector<BSONElement> _fixedElements ; // dummy element for KeyGenerator
      BSONObj _undefinedKey ;

      INT32                _nFields ; // number of fields
      BSONObj              _keyPattern ;
      BSONObj              _info ;
      UINT16               _type ;

      IXM_KEYGEN_TYPE      _keyGenType ;

      void _init() ;
      friend class _ixmKeyGenerator ;
   public:
      _ixmIndexKeyGen ( const _ixmIndexCB *indexCB,
                        IXM_KEYGEN_TYPE genType = GEN_OBJ_NO_FIELD_NAME ) ;
      _ixmIndexKeyGen ( const BSONObj &keyDef,
                        IXM_KEYGEN_TYPE genType = GEN_OBJ_NO_FIELD_NAME ) ;
      INT32 reset ( const BSONObj & info ) ;
      INT32 reset ( const _ixmIndexCB *indexCB ) ;
      INT32 getKeys ( const BSONObj &obj, BSONObjSet &keys,
                      BSONElement *pArrEle = NULL,
                      BOOLEAN isKeepKeyName = FALSE,
                      BOOLEAN transform = TRUE,
                      BOOLEAN ignoreUndefined = FALSE ) const ;
      BSONElement missingField() const ;
      IndexSuitability suitability( const BSONObj &query ,
                                    const BSONObj &order ) const ;
      static BOOLEAN validateKeyDef ( const BSONObj &keyDef ) ;
   } ;
   typedef class _ixmIndexKeyGen ixmIndexKeyGen ;
}

#endif //IXMINDEXKEY_HPP_

