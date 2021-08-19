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

   Source File Name = sptDBNode.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBNode.hpp"
#include "sptDBRG.hpp"
#include "sptDBSdb.hpp"
#include "sptDBSecureSdb.hpp"

using sdbclient::_sdbReplicaGroup ;
using namespace bson ;

namespace engine
{
   #define SPT_NODE_NAME   "SdbNode"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBNode, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBNode, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBNode, start )
   JS_MEMBER_FUNC_DEFINE( _sptDBNode, stop )
   JS_MEMBER_FUNC_DEFINE( _sptDBNode, connect )

   JS_BEGIN_MAPPING( _sptDBNode, SPT_NODE_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "start", start )
      JS_ADD_MEMBER_FUNC( "stop", stop )
      JS_ADD_MEMBER_FUNC( "connect", connect )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBNode::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBNode::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBNode::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBNode::_sptDBNode( _sdbNode* pNode )
   {
      _node.pNode = pNode ;
   }

   _sptDBNode::~_sptDBNode()
   {
   }

   INT32 _sptDBNode::construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "use of new SdbNode() is forbidden, you should use "
                     "other functions to produce a SdbNode object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBNode::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBNode::start( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _node.start() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to start node" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNode::stop( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _node.stop() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to stop node" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNode::connect( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 isSecure = 0 ;
      const sptObject *pObj = arg.getObject() ;
      sptObjectPtr rgPtr ;
      sptObjectPtr connPtr ;
      const _sptDBSdb *pSdb = NULL ;
      _sptDBSdb *pRetSdb = NULL ;

      if ( arg.argc() >= 1 )
      {
         rc = arg.getNative( 0, (void *)&isSecure, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            if ( arg.hasErrMsg() )
            {
               detail = BSON( SPT_ERR << arg.hasErrMsg() ) ;
            }
            else
            {
               detail = BSON( SPT_ERR << "Param[isSecure] must be bool" ) ;
            }
            goto error ;
         }
      }

      if ( !pObj )
      {
         detail = BSON( SPT_ERR << "Get object failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // get rg object
      rc = pObj->getObjectField( SPT_NODE_RG_FIELD, rgPtr ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get rg js obj" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // get conn object
      rc = rgPtr->getObjectField( SPT_RG_CONN_FIELD, connPtr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get conn js obj" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = connPtr->getUserObj( _sptDBSdb::__desc, ( const void** )&pSdb ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to get conn obj" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !isSecure )
      {
         pRetSdb = SDB_OSS_NEW _sptDBSdb() ;
         if ( !pRetSdb )
         {
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBSdb" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = pRetSdb->_sptSdb.connect( _node.getHostName(),
                                        _node.getServiceName(),
                                        pSdb->_user.c_str(),
                                        pSdb->_passwd.c_str() ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "Failed to connect sdb" ) ;
            goto error ;
         }

         rc = rval.setUsrObjectVal<_sptDBSdb>( pRetSdb ) ;
      }
      else
      {
         pRetSdb = SDB_OSS_NEW _sptDBSecureSdb() ;
         if ( !pRetSdb )
         {
            detail = BSON( SPT_ERR <<
                           "Failed to alloc memory for _sptDBSecureSdb" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = pRetSdb->_sptSdb.connect( _node.getHostName(),
                                        _node.getServiceName(),
                                        pSdb->_user.c_str(),
                                        pSdb->_passwd.c_str() ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "Failed to connect sdb" ) ;
            goto error ;
         }

         rc = rval.setUsrObjectVal<_sptDBSecureSdb>( pRetSdb ) ;
      }

      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }

      rval.addReturnValProperty( "_host" )->setValue( _node.getHostName() ) ;
      rval.addReturnValProperty( "_port" )->setValue( _node.getServiceName() ) ;

   done:
      return rc ;
   error:
      if ( pRetSdb )
      {
         SDB_OSS_DEL pRetSdb ;
         pRetSdb = NULL ;
      }
      goto done ;
   }

   INT32 _sptDBNode::cvtToBSON( const CHAR* key, const sptObject &value,
                                BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                string &errMsg )
   {
      errMsg = "SdbNode can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBNode::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                string &errMsg )
   {
      INT32 rc = SDB_OK ;      
      sptObjectPtr rgPtr ;
      string rgName ;
      string nodeName ;

      // get rg name
      rc = value.getObjectField( SPT_NODE_RG_FIELD, rgPtr ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get rg js obj" ;
         goto error ;
      }
      rgPtr->getStringField( SPT_NODE_RGNAME_FIELD, rgName ) ;
      // get node name
      rc = value.getStringField( SPT_NODE_NAME_FIELD, nodeName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get node _nodename field" ;
         goto error ;
      }
      // build result
      retObj = BSON( SPT_NODE_NAME_FIELD << rgName + ":" + nodeName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBNode::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                  _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string fullName ;
      string rgName ;
      string nodename ;
      string hostname ;
      string svcname ;
      size_t pos = 0, pos2 = 0 ;
      _sdbReplicaGroup *pRG = NULL ;
      sptDBRG *pSptRG = NULL ;
      _sdbNode *pNode = NULL ;
      sptDBNode *pSptNode = NULL ;
      INT32 nodeID = -1 ;
      sptProperty *pTmpProp = NULL ;

      fullName = data.getStringField( SPT_NODE_NAME_FIELD ) ;
      pos = fullName.find( ":" ) ;
      if( pos == std::string::npos ||
          pos >= fullName.size() - 1 )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Invalid node name" ) ;
         goto error ;
      }

      rgName = fullName.substr( 0, pos ) ;
      nodename = fullName.substr( pos + 1 ) ;
      rc = db.getReplicaGroup( rgName.c_str(), &pRG ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get ReplicaGroup" ) ;
         goto error ;
      }

      pos2 = nodename.find( ":" ) ;
      if( pos2 == std::string::npos ||
          pos2 >= nodename.size() - 1 )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Invalid nodename" ) ;
         goto error ;
      }
      hostname = nodename.substr( 0, pos2 ) ;
      svcname = nodename.substr( pos2 + 1 ) ;

      rc = pRG->getNode( hostname.c_str(), svcname.c_str(), &pNode ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node" ) ;
         goto error ;
      }

      pSptRG = SDB_OSS_NEW sptDBRG( pRG ) ;
      if( NULL == pSptRG )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBRG obj" ) ;
         goto error ;
      }
      pRG = NULL ;

      rc = pNode->getNodeID( nodeID ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node id" ) ;
         goto error ;
      }

      pSptNode = SDB_OSS_NEW sptDBNode( pNode ) ;
      if( !pSptNode )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBNode" ) ;
         goto error ;
      }
      nodename = pNode->getNodeName() ;
      hostname= pNode->getHostName() ;
      svcname = pNode->getServiceName() ;
      pNode = NULL ;

      rc = rval.setUsrObjectVal< sptDBNode >( pSptNode ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      pSptNode = NULL ;

      rval.addReturnValProperty( SPT_NODE_HOSTNAME_FIELD )->
         setValue( hostname ) ;
      rval.addReturnValProperty( SPT_NODE_SVCNAME_FIELD )->
         setValue( svcname) ;
      rval.addReturnValProperty( SPT_NODE_NAME_FIELD )->
         setValue( nodename ) ;
      rval.addReturnValProperty( SPT_NODE_NODEID_FIELD )->setValue( nodeID ) ;

      pTmpProp = rval.addReturnValProperty( SPT_NODE_RG_FIELD ) ;
      if ( !pTmpProp )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to alloc memory" ) ;
         goto error ;
      }
      rc = pTmpProp->assignUsrObject< sptDBRG >( pSptRG ) ;
      if( rc )
      {
         detail = BSON( SPT_ERR << "Failed to set node _rg property" ) ;
         goto error ;
      }
      pSptRG = NULL ;

      pTmpProp->addBackwardProp( SPT_RG_CONN_FIELD ) ;
      pTmpProp = pTmpProp->addSubProp( SPT_RG_NAME_FIELD ) ;
      if ( !pTmpProp )
      {
         detail = BSON( SPT_ERR << "Failed to set node _rg property" ) ;
         goto error ;
      }
      pTmpProp->setValue( rgName ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pRG ) ;
      SAFE_OSS_DELETE( pNode ) ;
      SAFE_OSS_DELETE( pSptRG ) ;
      SAFE_OSS_DELETE( pSptNode ) ;
      goto done ;
   }
}
