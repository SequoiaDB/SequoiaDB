/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptDBOptionBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/06/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_OPTIONBASE_HPP
#define SPT_DB_OPTIONBASE_HPP
#include "sptApi.hpp"

using namespace bson ;

namespace engine
{
   #define SPT_OPTIONBASE_NAME                "SdbOptionBase"
   #define SPT_OPTIONBASE_COND_FIELD          "_cond"
   #define SPT_OPTIONBASE_SEL_FIELD           "_sel"
   #define SPT_OPTIONBASE_SORT_FIELD          "_sort"
   #define SPT_OPTIONBASE_HINT_FIELD          "_hint"
   #define SPT_OPTIONBASE_SKIP_FIELD          "_skip"
   #define SPT_OPTIONBASE_LIMIT_FIELD         "_limit"
   #define SPT_OPTIONBASE_FLAGS_FIELD         "_flags"

   /*
      _sptDBOptionBase define
   */
   class _sptDBOptionBase : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBOptionBase )
   public:
      _sptDBOptionBase() ;
      virtual ~_sptDBOptionBase() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      static INT32 cvtToBSON( const CHAR* key,
                              const sptObject &value,
                              BOOLEAN isSpecialObj,
                              BSONObjBuilder& builder,
                              string &errMsg ) ;

      static INT32 fmpToBSON( const sptObject &value,
                              BSONObj &retObj,
                              string &errMsg ) ;

      static INT32 bsonToJSObj( sdbclient::sdb &db,
                                const BSONObj &data,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   protected:
      static void _setReturnVal( const BSONObj &data,
                                 _sptReturnVal &rval ) ;

   } ;
   typedef _sptDBOptionBase sptDBOptionBase ;
}

#endif

