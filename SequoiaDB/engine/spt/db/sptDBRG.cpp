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

   Source File Name = sptDBRG.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBRG.hpp"
#include "utilStr.hpp"
#include "sptDBNode.hpp"
using namespace sdbclient ;
using namespace bson ;
namespace engine
{
   #define NODE_NAME_SPLIT ':'
   #define NODE_NAME_SPLIT_VEC_SIZE 2
   #define SPT_RG_NAME  "SdbReplicaGroup"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBRG, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBRG, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, getMaster )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, getSlave )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, start )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, stop )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, createNode )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, removeNode )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, getNode )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, reelect )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, detachNode )
   JS_MEMBER_FUNC_DEFINE( _sptDBRG, attachNode )

   JS_BEGIN_MAPPING( _sptDBRG, SPT_RG_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "getMaster", getMaster )
      JS_ADD_MEMBER_FUNC( "getSlave", getSlave )
      JS_ADD_MEMBER_FUNC( "start", start )
      JS_ADD_MEMBER_FUNC( "stop", stop )
      JS_ADD_MEMBER_FUNC( "createNode", createNode )
      JS_ADD_MEMBER_FUNC( "removeNode", removeNode )
      JS_ADD_MEMBER_FUNC( "getNode", getNode )
      JS_ADD_MEMBER_FUNC( "reelect", reelect )
      JS_ADD_MEMBER_FUNC( "detachNode", detachNode )
      JS_ADD_MEMBER_FUNC( "attachNode", attachNode )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBRG::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBRG::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBRG::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBRG::_sptDBRG( _sdbReplicaGroup *pRG )
   {
      _rg.pReplicaGroup = pRG ;
   }

   _sptDBRG::~_sptDBRG()
   {
   }

   INT32 _sptDBRG::construct( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      BSON( SPT_ERR << "use of new SdbReplicaGroup() is forbidden, you should use "
           "other functions to produce a SdbReplicaGroup object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBRG::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBRG::getMaster( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbNode *pNode = NULL ;
      INT32 nodeID = -1 ;
      rc = _rg.getMaster( &pNode ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get master" ) ;
         goto error ;
      }
      rc = pNode->getNodeID( nodeID ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node id" ) ;
         goto error ;
      }
      SPT_SET_NODE_TO_RETURNVAL( pNode ) ;
      rval.addReturnValProperty( SPT_NODE_HOSTNAME_FIELD )->
         setValue( pNode->getHostName() ) ;
      rval.addReturnValProperty( SPT_NODE_SVCNAME_FIELD )->
         setValue( pNode->getServiceName() ) ;
      rval.addReturnValProperty( SPT_NODE_NAME_FIELD )->
         setValue( pNode->getNodeName() ) ;
      rval.addReturnValProperty( SPT_NODE_NODEID_FIELD )->setValue( nodeID ) ;
      rval.addSelfToReturnValProperty( SPT_NODE_RG_FIELD, SPT_PROP_READONLY ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNode ) ;
      goto done ;
   }

   INT32 _sptDBRG::getSlave( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbNode *pNode = NULL ;
      vector< INT32 > posVec ;
      INT32 nodeID = -1 ;

      if( 0 < arg.argc() && 7 >= arg.argc() )
      {
         for( UINT32 index = 0; index < arg.argc(); index++ )
         {
            INT32 pos = -1 ;
            if (arg.isInt(index))
            {
               rc = arg.getNative( index, &pos, SPT_NATIVE_INT32 ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
            }
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "pos must be int" ) ;
               goto error ;
            }
            posVec.push_back( pos ) ;
         }
         rc = _rg.getSlave( &pNode, posVec ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get slave" ) ;
            goto error ;
         }
      }
      else if( arg.argc() > 7 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Too many argements" ) ;
         goto error ;
      }
      else
      {
         rc = _rg.getSlave( &pNode ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get Slave" ) ;
            goto error ;
         }
      }
      rc = pNode->getNodeID( nodeID ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node id" ) ;
         goto error ;
      }
      SPT_SET_NODE_TO_RETURNVAL( pNode ) ;
      rval.addReturnValProperty( SPT_NODE_HOSTNAME_FIELD )->
         setValue( pNode->getHostName() ) ;
      rval.addReturnValProperty( SPT_NODE_SVCNAME_FIELD )->
         setValue( pNode->getServiceName() ) ;
      rval.addReturnValProperty( SPT_NODE_NAME_FIELD )->
         setValue( pNode->getNodeName() ) ;
      rval.addReturnValProperty( SPT_NODE_NODEID_FIELD )->setValue( nodeID ) ;
      rval.addSelfToReturnValProperty( SPT_NODE_RG_FIELD, SPT_PROP_READONLY ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNode ) ;
      goto done ;
   }

   INT32 _sptDBRG::start( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _rg.start() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to start RG" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::stop( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _rg.stop() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to stop RG" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::createNode( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      string dbPath ;
      BSONObj configs ;
      _sdbNode *pNode = NULL ;
      INT32 nodeID = -1 ;

      rc = arg.getString( 0, hostname ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Hostname must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Hostname must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, svcname, false ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be string or int" ) ;
         goto error ;
      }

      rc = arg.getString( 2, dbPath ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "DbPath must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "DbPath must be string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 3, configs ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Configs must be obj" ) ;
         goto error ;
      }
      rc = _rg.createNode( hostname.c_str(), svcname.c_str(), dbPath.c_str(),
                           configs ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create node" ) ;
         goto error ;
      }

      rc = _rg.getNode( hostname.c_str(), svcname.c_str(), &pNode ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node" ) ;
         goto error ;
      }
      rc = pNode->getNodeID( nodeID ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node id" ) ;
         goto error ;
      }
      SPT_SET_NODE_TO_RETURNVAL( pNode ) ;
      rval.addReturnValProperty( SPT_NODE_HOSTNAME_FIELD )->
         setValue( pNode->getHostName() ) ;
      rval.addReturnValProperty( SPT_NODE_SVCNAME_FIELD )->
         setValue( pNode->getServiceName() ) ;
      rval.addReturnValProperty( SPT_NODE_NAME_FIELD )->
         setValue( pNode->getNodeName() ) ;
      rval.addReturnValProperty( SPT_NODE_NODEID_FIELD )->setValue( nodeID ) ;
      rval.addSelfToReturnValProperty( SPT_NODE_RG_FIELD, SPT_PROP_READONLY ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNode ) ;
      goto done ;
   }

   INT32 _sptDBRG::removeNode( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      BSONObj options ;

      rc = arg.getString( 0, hostname ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Hostname must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Hostname must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, svcname, false ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be string or int" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 2, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _rg.removeNode( hostname.c_str(), svcname.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove node" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::getNode( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      string dbPath ;
      BSONObj configs ;

      if( arg.argc() == 1 )
      {
         string nodename ;
         vector< string > splitStrVec ;
         string seperator ;

         seperator.push_back( NODE_NAME_SPLIT ) ;
         rc = arg.getString( 0, nodename ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "NodeName must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "NodeName must be string" ) ;
            goto error ;
         }
         rc = utilSplitStr( nodename, splitStrVec, seperator ) ;
         if( SDB_OK != rc || splitStrVec.size() != NODE_NAME_SPLIT_VEC_SIZE )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Failed to split node name" ) ;
            goto error ;
         }
         hostname = splitStrVec[0];
         svcname = splitStrVec[1];
      }
      else
      {
         rc = arg.getString( 0, hostname ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Hostname must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Hostname must be string" ) ;
            goto error ;
         }

         rc = arg.getString( 1, svcname, false ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "ServiceName must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "ServiceName must be string or int" ) ;
            goto error ;
         }
      }
      rc = getNodeAndSetProperty( hostname, svcname, rval, detail ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::reelect( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }

      rc = _rg.reelect( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to reelect master" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::detachNode( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      BSONObj options ;

      rc = arg.getString( 0, hostname ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "HostName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "HostName must be string" ) ;
         goto error ;
      }
      rc = arg.getString( 1, svcname, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be string or int" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 2, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be an object" ) ;
         goto error ;
      }

      rc = _rg.detachNode( hostname.c_str(), svcname.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to detach node" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::attachNode( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      BSONObj options ;

      rc = arg.getString( 0, hostname ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "HostName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "HostName must be string" ) ;
         goto error ;
      }
      rc = arg.getString( 1, svcname, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "ServiceName must be string or int" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 2, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be an object" ) ;
         goto error ;
      }
      rc = _rg.attachNode( hostname.c_str(), svcname.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to attach node" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::getNodeAndSetProperty( const string &hostname,
                                          const string &svcname,
                                          _sptReturnVal &rval,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbNode *pNode = NULL ;
      INT32 nodeID = -1 ;
      rc = _rg.getNode( hostname.c_str(), svcname.c_str(), &pNode ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node" ) ;
         goto error ;
      }
      rc = pNode->getNodeID( nodeID ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get node id" ) ;
         goto error ;
      }
      SPT_SET_NODE_TO_RETURNVAL( pNode ) ;
      rval.addReturnValProperty( SPT_NODE_HOSTNAME_FIELD )->
         setValue( pNode->getHostName() ) ;
      rval.addReturnValProperty( SPT_NODE_SVCNAME_FIELD )->
         setValue( pNode->getServiceName() ) ;
      rval.addReturnValProperty( SPT_NODE_NAME_FIELD )->
         setValue( pNode->getNodeName() ) ;
      rval.addReturnValProperty( SPT_NODE_NODEID_FIELD )->setValue( nodeID ) ;
      rval.addSelfToReturnValProperty( SPT_NODE_RG_FIELD, SPT_PROP_READONLY ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNode ) ;
      goto done ;
   }

   INT32 _sptDBRG::cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg )
   {
      errMsg = "SdbRG can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBRG::fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string rgName ;
      rc = value.getStringField( SPT_RG_NAME_FIELD, rgName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get rg _name field" ;
         goto error ;
      }
      retObj = BSON( SPT_RG_NAME_FIELD << rgName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBRG::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string rgName ;
      _sdbReplicaGroup *pRG = NULL ;
      sptDBRG *pSptRG = NULL ;
      rgName = data.getStringField( SPT_RG_NAME_FIELD ) ;
      rc = db.getReplicaGroup( rgName.c_str(), &pRG ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get ReplicaGroup" ) ;
         goto error ;
      }
      pSptRG = SDB_OSS_NEW sptDBRG( pRG ) ;
      if( NULL == pSptRG )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBRG obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBRG >( pSptRG ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      rval.getReturnVal().setName( rgName ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_RG_NAME_FIELD )->setValue( rgName ) ;
      rval.addSelfToReturnValProperty( SPT_RG_CONN_FIELD ) ;
   done:
      return rc ;
   error:
      if( NULL != pSptRG )
      {
         SDB_OSS_DEL pSptRG ;
         pSptRG = NULL ;
         pRG = NULL ;
      }
      SAFE_OSS_DELETE( pRG ) ;
      goto done ;
   }
}
