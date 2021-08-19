/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = sptDBNode.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_NODE_HPP
#define SPT_DB_NODE_HPP
#include "client.hpp"
#include "sptApi.hpp"
using sdbclient::sdbNode ;
using sdbclient::_sdbNode ;
namespace engine
{
   #define SPT_NODE_NAME_FIELD      "_nodename"
   #define SPT_NODE_HOSTNAME_FIELD  "_hostname"
   #define SPT_NODE_SVCNAME_FIELD   "_servicename"
   #define SPT_NODE_NODEID_FIELD    "_nodeid"
   #define SPT_NODE_RG_FIELD        "_rg"
   #define SPT_NODE_RGNAME_FIELD    "_name"

   class _sptDBNode : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBNode )
   public:
      _sptDBNode( _sdbNode* pNode = NULL ) ;
      ~_sptDBNode() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 start( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 stop( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 connect( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
   private:
      sdbNode _node ;
   } ;
   typedef _sptDBNode sptDBNode ;

   #define SPT_SET_NODE_TO_RETURNVAL( pNode )   \
      do\
      {\
         sptDBNode *__sptNode__ = SDB_OSS_NEW sptDBNode( pNode ) ;\
         if( NULL == __sptNode__ )\
         {\
            rc = SDB_OOM ;\
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBNode" ) ;\
            goto error ;\
         }\
         rc = rval.setUsrObjectVal< sptDBNode >( __sptNode__ ) ;\
         if( SDB_OK != rc )\
         {\
            SAFE_OSS_DELETE( __sptNode__ ) ;\
            pNode = NULL ;\
            detail = BSON( SPT_ERR << "Failed to set return obj" ) ;\
            goto error ;\
         }\
      }while(0)

}
#endif
