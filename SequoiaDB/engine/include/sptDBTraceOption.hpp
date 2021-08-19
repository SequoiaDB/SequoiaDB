/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptDBTraceOption.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/03/2019  FJ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_TRACEOPTION_HPP
#define SPT_DB_TRACEOPTION_HPP
#include "sptApi.hpp"

using namespace bson ;

namespace engine
{
   #define SPT_TRACEOPTION_NAME                  "SdbTraceOption"
   #define SPT_TRACEOPTION_COMPONENTS_FIELD      "_components"
   #define SPT_TRACEOPTION_BREAKPOINTS_FIELD     "_breakPoints"
   #define SPT_TRACEOPTION_TIDS_FIELD            "_tids"
   #define SPT_TRACEOPTION_FUNCTIONNAMES_FIELD   "_functionNames"
   #define SPT_TRACEOPTION_THREADTYPES_FIELD     "_threadTypes"

   /*
      _sptDBTraceOption define
   */
   class _sptDBTraceOption : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBTraceOption )
   public:
      _sptDBTraceOption() ;
      virtual ~_sptDBTraceOption() ;
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
                         BSONObj &detail ) ;
   protected:
      static void _setReturnVal( const BSONObj &data,
                                 _sptReturnVal &rval ) ;
   } ;
   typedef _sptDBTraceOption sptDBTraceOption ;
}

#endif

