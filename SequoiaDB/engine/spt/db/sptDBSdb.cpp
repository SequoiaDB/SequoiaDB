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

   Source File Name = sptDBSdb.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptDBSdb.hpp"
#include "sptDBCursor.hpp"
#include "sptDBRG.hpp"
#include "sptDBDC.hpp"
#include "sptDBCS.hpp"
#include "sptDBCL.hpp"
#include "sptDBNode.hpp"
#include "sptDBDomain.hpp"
#include "sptDBOptionBase.hpp"
#include "sptDBSnapshotOption.hpp"
#include "sptDBTraceOption.hpp"
#include "sptDBUser.hpp"
#include "sptBsonobj.hpp"
#include "ossSocket.hpp"
#include "msgDef.hpp"
#include "fmpDef.hpp"
#include "utilPasswdTool.hpp"
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
using namespace std ;
using namespace bson ;
using sdbclient::sdbCollectionSpace ;
using sdbclient::sdbReplicaGroup ;
using sdbclient::sdbCursor ;
using sdbclient::_sdbCollectionSpace ;
using sdbclient::_sdbReplicaGroup ;
using sdbclient::_sdbCursor ;
using sdbclient::_sdbDataCenter ;
using sdbclient::_sdbDomain ;

namespace engine
{
   #define SDB_DEF_COORD_NAME "localhost"
   #define SDB_DEF_COORD_PORT OSS_DFT_SVCPORT
   #define NODE_NAME_SPLIT ':'
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBSdb, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBSdb, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, close )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, getCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, getRG )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, getDC )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, createCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, createRG )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, removeRG )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, createCataRG )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, dropCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, snapshot )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, resetSnapshot )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, list )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, startRG )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, createUsr )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, dropUsr )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, exec )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, execUpdate )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, traceOn )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, traceResume )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, traceOff )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, traceStatus )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, transBegin )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, transCommit )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, transRollback )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, flushConfigure )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, createProcedure )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, removeProcedure )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, listProcedures )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, eval )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, backup )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, listBackup )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, removeBackup )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, listTasks )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, waitTasks )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, cancelTask )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, getSessionAttr )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, setSessionAttr )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, msg )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, createDomain )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, dropDomain )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, getDomain )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, listDomains )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, invalidateCache )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, forceSession )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, forceStepUp )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, sync )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, loadCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, unloadCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, setPDLevel )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, reloadConf )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, renameCS )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, analyze )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, updateConfig )
   JS_MEMBER_FUNC_DEFINE( _sptDBSdb, deleteConfig )
   JS_RESOLVE_FUNC_DEFINE( _sptDBSdb, resolve )

   JS_BEGIN_MAPPING( _sptDBSdb, "Sdb" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "close", close)
      JS_ADD_MEMBER_FUNC( "getCS", getCS )
      JS_ADD_MEMBER_FUNC( "getRG", getRG )
      JS_ADD_MEMBER_FUNC( "getDC", getDC )
      JS_ADD_MEMBER_FUNC( "createCS", createCS )
      JS_ADD_MEMBER_FUNC( "createRG", createRG )
      JS_ADD_MEMBER_FUNC( "removeRG", removeRG )
      JS_ADD_MEMBER_FUNC( "createCataRG", createCataRG )
      JS_ADD_MEMBER_FUNC( "dropCS", dropCS )
      JS_ADD_MEMBER_FUNC( "snapshot", snapshot )
      JS_ADD_MEMBER_FUNC( "resetSnapshot", resetSnapshot )
      JS_ADD_MEMBER_FUNC( "list", list )
      JS_ADD_MEMBER_FUNC( "startRG", startRG )
      JS_ADD_MEMBER_FUNC( "createUsr", createUsr )
      JS_ADD_MEMBER_FUNC( "dropUsr", dropUsr )
      JS_ADD_MEMBER_FUNC( "exec", exec )
      JS_ADD_MEMBER_FUNC( "execUpdate", execUpdate )
      JS_ADD_MEMBER_FUNC( "traceOn", traceOn )
      JS_ADD_MEMBER_FUNC( "traceResume", traceResume )
      JS_ADD_MEMBER_FUNC( "_traceOff", traceOff )
      JS_ADD_MEMBER_FUNC( "traceStatus", traceStatus )
      JS_ADD_MEMBER_FUNC( "transBegin", transBegin )
      JS_ADD_MEMBER_FUNC( "transCommit", transCommit )
      JS_ADD_MEMBER_FUNC( "transRollback", transRollback )
      JS_ADD_MEMBER_FUNC( "flushConfigure", flushConfigure )
      JS_ADD_MEMBER_FUNC( "createProcedure", createProcedure )
      JS_ADD_MEMBER_FUNC( "removeProcedure", removeProcedure )
      JS_ADD_MEMBER_FUNC( "listProcedures", listProcedures )
      JS_ADD_MEMBER_FUNC( "eval", eval )
      JS_ADD_MEMBER_FUNC( "backup", backup )
      JS_ADD_MEMBER_FUNC( "backupOffline", backup )
      JS_ADD_MEMBER_FUNC( "listBackup", listBackup )
      JS_ADD_MEMBER_FUNC( "removeBackup", removeBackup )
      JS_ADD_MEMBER_FUNC( "listTasks", listTasks )
      JS_ADD_MEMBER_FUNC( "waitTasks", waitTasks )
      JS_ADD_MEMBER_FUNC( "cancelTask", cancelTask )
      JS_ADD_MEMBER_FUNC( "getSessionAttr", getSessionAttr )
      JS_ADD_MEMBER_FUNC( "setSessionAttr", setSessionAttr )
      JS_ADD_MEMBER_FUNC( "msg", msg )
      JS_ADD_MEMBER_FUNC( "createDomain", createDomain )
      JS_ADD_MEMBER_FUNC( "dropDomain", dropDomain )
      JS_ADD_MEMBER_FUNC( "getDomain", getDomain )
      JS_ADD_MEMBER_FUNC( "listDomains", listDomains )
      JS_ADD_MEMBER_FUNC( "invalidateCache", invalidateCache )
      JS_ADD_MEMBER_FUNC( "forceSession", forceSession )
      JS_ADD_MEMBER_FUNC( "forceStepUp", forceStepUp )
      JS_ADD_MEMBER_FUNC( "sync", sync )
      JS_ADD_MEMBER_FUNC( "loadCS", loadCS )
      JS_ADD_MEMBER_FUNC( "unloadCS", unloadCS )
      JS_ADD_MEMBER_FUNC( "setPDLevel", setPDLevel )
      JS_ADD_MEMBER_FUNC( "reloadConf", reloadConf )
      JS_ADD_MEMBER_FUNC( "renameCS", renameCS )
      JS_ADD_MEMBER_FUNC( "analyze", analyze )
      JS_ADD_MEMBER_FUNC( "updateConf", updateConfig )
      JS_ADD_MEMBER_FUNC( "deleteConf", deleteConfig )
      JS_ADD_RESOLVE_FUNC( resolve )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBSdb::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBSdb::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBSdb::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBSdb::_sptDBSdb( BOOLEAN isSecure ): _sptSdb( isSecure )
   {
   }

   _sptDBSdb::~_sptDBSdb()
   {
   }

   INT32 _sptDBSdb::construct( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32   rc = SDB_OK ;
      string  hostname ;
      string  svcname ;
      string  username ;
      string  passwd ;
      string  objectName ;

      if ( arg.argc() > 3 )
      {
         // if password has been input, we don't save command to history file.
         sdbSetIsNeedSaveHistory( FALSE ) ;
      }

      // Get hostname
      rc = arg.getString( 0, hostname, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         stringstream ss ;
         ss << SDB_DEF_COORD_PORT ;
         hostname = SDB_DEF_COORD_NAME ;
         svcname = ss.str() ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Hostname must be string" ) ;
         goto error ;
      }
      else
      {
         // Get svcname
         rc = arg.getString( 1, svcname ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            size_t pos = hostname.find( NODE_NAME_SPLIT ) ;
            if( std::string::npos == pos )
            {
               stringstream ss ;
               ss << SDB_DEF_COORD_PORT ;
               svcname = ss.str() ;
            }
            else
            {
               svcname = hostname.substr( pos + 1 ) ;
               hostname = hostname.substr( 0, pos ) ;
            }
         }
         else if( SDB_OK != rc )
         {
            UINT16 port = 0 ;
            rc = arg.getNative( 1, (void*)&port, SPT_NATIVE_INT16 ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Svcname must be string or int" ) ;
               goto error ;
            }
            svcname = boost::lexical_cast< string >( port ) ;
         }
      }

      if( arg.argc() > 2 )
      {
         if( !arg.isNull( 2 ) )
         {
            objectName = arg.getUserObjClassName( 2 ) ;
         }

         if ( SPT_USER_NAME == objectName )
         {
            sptDBUser *pUser = NULL ;
            BSONObj userObj ;

            rc = arg.getUserObj( 2, sptDBUser::__desc, ( const void** )&pUser ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "The obj must be User" ) ;
               goto error ;
            }

            rc = arg.getBsonobj( 2, userObj ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to get User obj" ) ;
               goto error ;
            }

            username = userObj.getStringField( SDB_AUTH_USER ) ;
            passwd   = pUser->getPasswd() ;
         }
         else if ( SPT_CIPHERUSER_NAME == objectName )
         {
            sptDBCipherUser *pCipherUser = NULL ;
            BSONObj cipherUserObj ;

            rc = arg.getUserObj( 2, sptDBCipherUser::__desc,
                                 ( const void** )&pCipherUser ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "The obj must be CipherUser" ) ;
               goto error ;
            }

            rc = arg.getBsonobj( 2, cipherUserObj ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to get CipherUser obj" ) ;
               goto error ;
            }

            rc = _getUserInfoFromCipherUserObj( cipherUserObj, pCipherUser,
                                                username, passwd, detail ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else
         {
            // Get username
            if( !arg.isNull( 2 ) )
            {
               rc = arg.getString( 2, username ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "username must be string" ) ;
                  goto error ;
               }
            }

            // Get password
            if( !arg.isNull( 3 ) )
            {
               rc = arg.getString( 3, passwd ) ;
               if( SDB_OUT_OF_BOUND == rc )
               {
                  detail = BSON( SPT_ERR <<
                                "you should input your password to connect engine!" ) ;
                  goto error ;
               }
               else if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Password must be string" ) ;
                  goto error ;
               }
            }
         }
      }

      rc = _sptSdb.connect( hostname.c_str(), svcname.c_str(),
                            username.c_str(), passwd.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to connect sdb" ) ;
         goto error ;
      }

      _user = username ;
      _passwd = passwd ;
      rval.addSelfProperty( "_host" )->setValue( hostname ) ;
      rval.addSelfProperty( "_port" )->setValue( svcname ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::destruct()
   {
      _sptSdb.disconnect() ;
      return SDB_OK ;
   }

   INT32 _sptDBSdb::close( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      _sptSdb.disconnect() ;
      return SDB_OK ;
   }

   INT32 _sptDBSdb::analyze( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj option ;
      if( !arg.isNull( 0 ) )
      {
         rc = arg.getBsonobj( 0, option ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "option must be obj" ) ;
            goto error ;
         }
      }
      rc = _sptSdb.analyze( option ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to analyze sdb" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::getCS( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string csName ;

      rc = arg.getString( 0, csName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CollectionSpace name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CollectionSpace name must be string" ) ;
         goto error ;
      }

      rc = _getCSAndSetProperty( csName, rval, detail ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::getRG( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() < 1 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "RGName or RGID must be config" ) ;
         goto error ;
      }
      if( arg.isString( 0 ) )
      {
         string rgName ;
         rc = arg.getString( 0, rgName ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "RgName must be string" ) ;
            goto error ;
         }
         rc = _getRGAndSetProperty( rgName, rval, detail ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else if( arg.isInt( 0 ) )
      {
         INT32 rgID = 0 ;
         rc = arg.getNative( 0, &rgID, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "RgID must be number" ) ;
            goto error ;
         }
         rc = _getRGAndSetProperty( rgID, rval, detail ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Sdb.getRG(<name>|<id>): wrong arguments" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::getDC( const _sptArguments &arg,
                _sptReturnVal &rval,
                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbDataCenter *pDC = NULL ;
      sptDBDC *sptDC = NULL ;
      rc = _sptSdb.getDC( &pDC ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get datacenter" ) ;
         goto error ;
      }
      sptDC = SDB_OSS_NEW sptDBDC( pDC ) ;
      if( NULL == sptDC )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new DC obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBDC >( sptDC ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      rval.getReturnVal().setName( pDC->getName() ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_DC_NAME_FIELD )->setValue( pDC->getName() ) ;
   done:
      return rc ;
   error:
      if( NULL != sptDC )
      {
         SDB_OSS_DEL sptDC ;
         sptDC = NULL ;
         pDC = NULL ;
      }
      SAFE_OSS_DELETE( pDC ) ;
      goto done ;
   }

   INT32 _sptDBSdb::createCS( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCollectionSpace *pCS = NULL ;
      sptDBCS *sptCS = NULL ;
      string csName ;
      BSONObjBuilder optionsBuilder ;
      INT32 pageSize = SDB_PAGESIZE_DEFAULT ;
      if( 1 != arg.argc() && 2 != arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = arg.getString( 0, csName ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "csName must be string" ) ;
         goto error ;
      }

      if( 2 == arg.argc() )
      {
         if( arg.isInt( 1 ) )
         {
            rc = arg.getNative( 1, &pageSize, SPT_NATIVE_INT32 ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "PageSize should be number" ) ;
               goto error ;
            }
            optionsBuilder.append( FIELD_NAME_PAGE_SIZE, pageSize ) ;
         }
         else if( arg.isObject( 1 ) )
         {
            BSONObj options ;
            rc = arg.getBsonobj( 1, options ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Options should be obj" ) ;
               goto error ;
            }
            optionsBuilder.appendElements( options ) ;
            if( options.getField( FIELD_NAME_PAGE_SIZE ).eoo() )
            {
               optionsBuilder.append( FIELD_NAME_PAGE_SIZE, pageSize ) ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "The 1rd argument should be obj or number" ) ;
            goto error ;
         }
      }
      rc = _sptSdb.createCollectionSpace( csName.c_str(), optionsBuilder.obj(),
                                          &pCS ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create collection space" ) ;
         goto error ;
      }

      sptCS = SDB_OSS_NEW sptDBCS( pCS ) ;
      if( NULL == sptCS )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBCS obj" ) ;
      }
      rc = rval.setUsrObjectVal< sptDBCS >( sptCS ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      rval.getReturnVal().setName( pCS->getCSName() ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( "_name" )->setValue( pCS->getCSName() ) ;
      rval.addSelfToReturnValProperty( SPT_CS_CONN_FIELD ) ;
   done:
      return rc ;
   error:
      if( NULL != sptCS )
      {
         SDB_OSS_DEL sptCS ;
         sptCS = NULL ;
         pCS = NULL ;
      }
      SAFE_OSS_DELETE( pCS ) ;
      goto done ;
   }

   INT32 _sptDBSdb::createRG( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string rgName ;
      _sdbReplicaGroup *pRG = NULL ;
      sptDBRG *sptRG = NULL ;
      rc = arg.getString( 0, rgName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "RGName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "RGName must be string" ) ;
         goto error ;
      }

      rc = _sptSdb.createReplicaGroup( rgName.c_str(), &pRG ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      sptRG = SDB_OSS_NEW sptDBRG( pRG ) ;
      if( NULL == sptRG )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBRG obj" ) ;
      }
      rc = rval.setUsrObjectVal< sptDBRG >( sptRG ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set user obj" ) ;
         goto error ;
      }
      rval.getReturnVal().setName( pRG->getName() ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( "_name" )->setValue( pRG->getName() ) ;
      rval.addSelfToReturnValProperty( SPT_RG_CONN_FIELD ) ;
   done:
      return rc ;
   error:
      if( NULL != sptRG )
      {
         SDB_OSS_DEL sptRG ;
         sptRG = NULL ;
         pRG = NULL ;
      }
      SAFE_OSS_DELETE( pRG ) ;
      goto done ;
   }

   INT32 _sptDBSdb::createCataRG( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      string dbPath ;
      BSONObj config ;

      rc = arg.getString( 0, hostname ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Host must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Host must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, svcname, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Service must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Service must be string or int" ) ;
         goto error ;
      }

      rc = arg.getString( 2, dbPath ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Dbpath must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Dbpath must be string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 3, config ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Config must be obj" ) ;
         goto error ;
      }

      rc = _sptSdb.createReplicaCataGroup( hostname.c_str(), svcname.c_str(),
                                           dbPath.c_str(), config ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed tp create catalog replica group" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::removeRG( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string rgName ;

      rc = arg.getString( 0, rgName ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "RGName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "RGName must be string" ) ;
         goto error ;
      }

      rc = _sptSdb.removeReplicaGroup( rgName.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove RG" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::dropCS( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string csName ;

      rc = arg.getString( 0, csName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CSName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CSName must be string" ) ;
         goto error ;
      }

      if( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "DropCS() can only specify one parameter" ) ;
         goto error ;
      }

      rc = _sptSdb.dropCollectionSpace( csName.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop CS" ) ;
         goto error ;
      }
      rval.addSelfProperty( csName )->setDelete() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::snapshot( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      INT32 snapshotType = 0 ;
      BSONObj obj ;
      string objectName ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;
      INT64 numToSkip = 0 ;
      INT64 numToRet = -1 ;

      rc = arg.getNative( 0, &snapshotType, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Snapshot type must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Snapshot type must be number" ) ;
         goto error ;
      }
      if( !arg.isNull( 1 ) )
      {
         objectName = arg.getUserObjClassName( 1 ) ;
      }

      if ( SPT_OPTIONBASE_NAME == objectName ||
           SPT_SNAPSHOTOPTION_NAME == objectName )
      {
         rc = arg.getBsonobj( 1, obj ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << ( arg.hasErrMsg() ? arg.getErrMsg() :
                                        "Cond must be obj" ) ) ;
            goto error ;
         }

         if ( obj.hasField( SPT_OPTIONBASE_COND_FIELD ) )
         {
            cond = obj.getObjectField( SPT_OPTIONBASE_COND_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SEL_FIELD ) )
         {
            sel = obj.getObjectField( SPT_OPTIONBASE_SEL_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SORT_FIELD ) )
         {
            order = obj.getObjectField( SPT_OPTIONBASE_SORT_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_HINT_FIELD ) )
         {
            hint = obj.getObjectField( SPT_OPTIONBASE_HINT_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SKIP_FIELD ) )
         {
            numToSkip = obj.getIntField( SPT_OPTIONBASE_SKIP_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_LIMIT_FIELD ) )
         {
            numToRet = obj.getIntField( SPT_OPTIONBASE_LIMIT_FIELD ) ;
         }
      }
      else
      {
         if( !arg.isNull( 1 ) )
         {
            rc = arg.getBsonobj( 1, cond ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Cond must be obj" ) ;
               goto error ;
            }
         }

         if( !arg.isNull( 2 ) )
         {
            rc = arg.getBsonobj( 2, sel ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Sel must be obj" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 3 ) )
         {
            rc = arg.getBsonobj( 3, order ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Order must be obj" ) ;
               goto error ;
            }
         }
      }
      rc = _sptSdb.getSnapshot( &pCursor, snapshotType,
                                cond, sel, order, hint,
                                numToSkip, numToRet ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get snapshot" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::resetSnapshot( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj cond ;

      rc = arg.getBsonobj( 0, cond ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Cond must be obj" ) ;
         goto error ;
      }

      rc = _sptSdb.resetSnapshot( cond ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to reset snapshot" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::list( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string objectName ;
      _sdbCursor *pCursor = NULL ;
      INT32 listType = 0 ;
      BSONObj obj ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;
      INT64 numToRet = -1 ;
      INT64 numToSkip = 0 ;

      rc = arg.getNative( 0, &listType, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "List type must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "List type must be number" ) ;
         goto error ;
      }

      if( !arg.isNull( 1 ) )
      {
         objectName = arg.getUserObjClassName( 1 ) ;
      }

      if ( SPT_OPTIONBASE_NAME == objectName ||
           SPT_SNAPSHOTOPTION_NAME == objectName )
      {
         rc = arg.getBsonobj( 1, obj ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << ( arg.hasErrMsg() ? arg.getErrMsg() :
                                        "Cond must be obj" ) ) ;
            goto error ;
         }

         if ( obj.hasField( SPT_OPTIONBASE_COND_FIELD ) )
         {
            cond = obj.getObjectField( SPT_OPTIONBASE_COND_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SEL_FIELD ) )
         {
            sel = obj.getObjectField( SPT_OPTIONBASE_SEL_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SORT_FIELD ) )
         {
            order = obj.getObjectField( SPT_OPTIONBASE_SORT_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_HINT_FIELD ) )
         {
            hint = obj.getObjectField( SPT_OPTIONBASE_HINT_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SKIP_FIELD ) )
         {
            numToSkip = obj.getIntField( SPT_OPTIONBASE_SKIP_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_LIMIT_FIELD ) )
         {
            numToRet = obj.getIntField( SPT_OPTIONBASE_LIMIT_FIELD ) ;
         }
      }
      else if( arg.argc() > 1 )
      {
         rc = arg.getBsonobj( 1, cond ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Cond must be obj" ) ;
            goto error ;
         }
         if( arg.argc() > 2 )
         {
            rc = arg.getBsonobj( 2, sel ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Sel must be obj" ) ;
               goto error ;
            }
            if( arg.argc() > 3 )
            {
               rc = arg.getBsonobj( 3, order ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Order must be obj" ) ;
                  goto error ;
               }
            }
         }
      }

      rc = _sptSdb.getList( &pCursor, listType, cond, sel, order, hint,
                            numToSkip, numToRet ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get list" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::startRG( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      if( arg.argc() < 1 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "RGName must be config" ) ;
         goto error ;
      }

      for( UINT32 index = 0; index < arg.argc(); index++ )
      {
         string rgName ;
         sdbReplicaGroup rg ;
         rc = arg.getString( index, rgName ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "rgName must be string" ) ;
            goto error ;
         }

         rc = _sptSdb.getReplicaGroup( rgName.c_str(), rg ) ;
         if( SDB_OK !=  rc )
         {
            detail = BSON( SPT_ERR << "Failed to get RG" ) ;
            goto error ;
         }

         rc = rg.start() ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to start RG" ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::createUsr( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32   rc = SDB_OK ;
      string  username ;
      string  passwd ;
      string  objectName ;
      BSONObj options ;
      stringstream ss ;

      if ( 0 == arg.argc() )
      {
         detail = BSON( SPT_ERR << "You should input username and password, or"
                  " input User obj or CipherUser obj" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         // if password has been input, we don't save command to history file.
         sdbSetIsNeedSaveHistory( FALSE ) ;
      }

      if ( arg.argc() > 3 )
      {
         detail = BSON( SPT_ERR << "Arguments exceed 3" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( !arg.isNull( 0 ) )
      {
         objectName = arg.getUserObjClassName( 0 ) ;
      }

      if ( SPT_USER_NAME == objectName )
      {
         sptDBUser *pUser = NULL ;
         BSONObj userObj ;

         rc = arg.getUserObj( 0, sptDBUser::__desc, ( const void** )&pUser ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "The obj must be User" ) ;
            goto error ;
         }

         rc = arg.getBsonobj( 0, userObj ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get User obj" ) ;
            goto error ;
         }

         username = userObj.getStringField( SDB_AUTH_USER ) ;
         passwd   = pUser->getPasswd() ;
      }
      else if ( SPT_CIPHERUSER_NAME == objectName )
      {
         sptDBCipherUser *pCipherUser = NULL ;
         BSONObj cipherUserObj ;

         rc = arg.getUserObj( 0, sptDBCipherUser::__desc,
                              ( const void** )&pCipherUser ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "The obj must be CipherUser" ) ;
            goto error ;
         }

         rc = arg.getBsonobj( 0, cipherUserObj ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get cipher user obj" ) ;
            goto error ;
         }

         rc = _getUserInfoFromCipherUserObj( cipherUserObj, pCipherUser,
                                             username, passwd, detail ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = arg.getString( 0, username ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "UserName must be string" ) ;
            goto error ;
         }

         rc = arg.getString( 1, passwd ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Password must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Password must be string" ) ;
            goto error ;
         }
      }

      if ( SPT_USER_NAME == objectName || SPT_CIPHERUSER_NAME == objectName )
      {
         rc = arg.getBsonobj( 1, options ) ;
         if ( rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << ( arg.hasErrMsg() ?
                                        arg.getErrMsg() :
                                        "Options must be object" ) ) ;
            goto error ;
         }
      }
      else
      {
         rc = arg.getBsonobj( 2, options ) ;
         if ( rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << ( arg.hasErrMsg() ?
                                        arg.getErrMsg() :
                                        "Options must be object" ) ) ;
            goto error ;
         }
      }

      if ( username.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Username can't be empty" ) ;
         goto error ;
      }

      if ( SDB_MAX_USERNAME_LENGTH < username.size() )
      {
         rc = SDB_INVALIDARG ;
         ss << "Exceeds username maximum length. Its max maximum length is "
            << SDB_MAX_USERNAME_LENGTH ;
         detail = BSON( SPT_ERR << ss.str().c_str() ) ;
         goto error ;
      }

      if ( SDB_MAX_PASSWORD_LENGTH < passwd.size() )
      {
         rc = SDB_INVALIDARG ;
         ss << "Exceeds password maximum length. Its max maximum length is "
            << SDB_MAX_PASSWORD_LENGTH ;
         detail = BSON( SPT_ERR << ss.str().c_str() ) ;
         goto error ;
      }

      rc = _sptSdb.createUsr( username.c_str(), passwd.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create user" ) ;
         goto error ;
      }

      if ( _user.empty() )
      {
         _user = username ;
         _passwd = passwd ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::dropUsr( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32   rc = SDB_OK ;
      string  username ;
      string  passwd ;
      string  objectName ;

      if ( 0 == arg.argc() )
      {
         detail = BSON( SPT_ERR << "You should input username and password, or"
                  " input User obj or CipherUser obj" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         // if password has been input, we don't save command to history file.
         sdbSetIsNeedSaveHistory( FALSE ) ;
      }

      if ( arg.argc() > 2 )
      {
         detail = BSON( SPT_ERR << "Arguments exceed 2" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( !arg.isNull( 0 ) )
      {
         objectName = arg.getUserObjClassName( 0 ) ;
      }

      if ( SPT_USER_NAME == objectName )
      {
         sptDBUser *pUser = NULL ;
         BSONObj userObj ;

         rc = arg.getUserObj( 0, sptDBUser::__desc, ( const void** )&pUser ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "The obj must be User" ) ;
            goto error ;
         }

         rc = arg.getBsonobj( 0, userObj ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get User obj" ) ;
            goto error ;
         }

         username = userObj.getStringField( SDB_AUTH_USER ) ;
         passwd   = pUser->getPasswd() ;
      }
      else if ( SPT_CIPHERUSER_NAME == objectName )
      {
         sptDBCipherUser *pCipherUser = NULL ;
         BSONObj cipherUserObj ;

         rc = arg.getUserObj( 0, sptDBCipherUser::__desc,
                              ( const void** )&pCipherUser ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "The obj must be CipherUser" ) ;
            goto error ;
         }

         rc = arg.getBsonobj( 0, cipherUserObj ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get cipher user obj" ) ;
            goto error ;
         }

         rc = _getUserInfoFromCipherUserObj( cipherUserObj, pCipherUser,
                                             username, passwd, detail ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = arg.getString( 0, username ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "UserName must be string" ) ;
            goto error ;
         }

         rc = arg.getString( 1, passwd ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Password must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Password must be string" ) ;
            goto error ;
         }
      }

      rc = _sptSdb.removeUsr( username.c_str(), passwd.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop user" ) ;
         goto error ;
      }

      if ( _user == username )
      {
         _user.clear() ;
         _passwd.clear() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::exec( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string sql ;
      _sdbCursor *pCursor = NULL ;

      rc = arg.getString( 0, sql ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Sql must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Sql must be string" ) ;
         goto error ;
      }

      rc = _sptSdb.exec( sql.c_str(), &pCursor ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to exec sql" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::execUpdate( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string sql ;
      BSONObj result ;

      rc = arg.getString( 0, sql ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Sql must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Sql must be string" ) ;
         goto error ;
      }

      rc = _sptSdb.execUpdate( sql.c_str(), &result ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to exec update" ) ;
         goto error ;
      }

      if ( !result.isEmpty() )
      {
         rval.getReturnVal().setValue( result ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::traceOn( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 bufferSize = 0 ;
      string component ;
      string breakpoint ;
      vector< UINT32 > tidVec ;
      string objectName ;
      BSONObj optionObj ;

      rc = arg.getNative( 0, &bufferSize, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "BufferSize type must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "BufferSize type must be number" ) ;
         goto error ;
      }

      if( !arg.isNull( 1 ) )
      {
         objectName = arg.getUserObjClassName( 1 ) ;
      }

      if( SPT_TRACEOPTION_NAME == objectName )
      {
         rc = arg.getBsonobj( 1, optionObj ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "SdbTraceOption must be obj" ) ;
            goto error ;
         }
         rc = _sptSdb.traceStart( bufferSize, optionObj ) ;
      }
      else
      {
         rc = arg.getString( 1, component ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Component must be string" ) ;
            goto error ;
         }
         rc = arg.getString( 2, breakpoint ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Breakpoint must be string" ) ;
            goto error ;
         }

         if( arg.argc() >= 4 )
         {
            if( arg.isInt( 3 ) )
            {
               UINT32 tid = 0 ;
               rc = arg.getNative( 3, &tid, SPT_NATIVE_INT32 ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Failed to get invalid tid" ) ;
                  goto error ;
               }
               tidVec.push_back( tid ) ;
            }
            else if( arg.isObject( 3 ) )
            {
               BSONObj tidObj ;
               rc = arg.getBsonobj( 3, tidObj ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Failed to get invalid tid" ) ;
                  goto error ;
               }
               try
               {
                  INT32 tmpTid = 0 ;
                  BSONObj::iterator itr( tidObj ) ;
                  while( itr.more() )
                  {
                     itr.next().Val( tmpTid ) ;
                     tidVec.push_back( static_cast< UINT32 >( tmpTid ) ) ;
                  }
               }
               catch( std::exception e )
               {
                  rc = SDB_INVALIDARG ;
                  detail = BSON( SPT_ERR << "Failed to get tid array element" ) ;
                  goto error ;
               }
            }
            else
            {
               detail = BSON( SPT_ERR << "Tid must be int or array type" ) ;
               goto error ;
            }
         }

         rc = _sptSdb.traceStart( bufferSize, component.c_str(),
                                  breakpoint.c_str(), tidVec ) ;
      }

      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to start trace" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::traceResume( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      rc = _sptSdb.isValid( &result ) ;
      if( SDB_OK != rc || FALSE == result )
      {
         if( FALSE == result )
         {
            rc = SDB_NOT_CONNECTED ;
         }
         detail = BSON( SPT_ERR << "Sdb.traceResume(): no connection handle" ) ;
         goto error ;
      }
      rc = _sptSdb.traceResume() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to resume trace" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::traceOff( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string fileName ;

      rc = arg.getString( 0, fileName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "FileName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "FileName must be string" ) ;
         goto error ;
      }

      rc = _sptSdb.traceStop( fileName.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to stop trace" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::traceStatus( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      rc = _sptSdb.traceStatus( &pCursor ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get status of trace" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::transBegin( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _sptSdb.transactionBegin() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to begin transaction" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::transCommit( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _sptSdb.transactionCommit() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to commit transaction" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::transRollback( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _sptSdb.transactionRollback() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to rollback transaction" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::flushConfigure( const _sptArguments &arg,
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
      rc = _sptSdb.flushConfigure( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to flush configure" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::createProcedure( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string funcStr ;
      if( !arg.isObject( 0 ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Function must be function" ) ;
         goto error ;
      }
      rc = arg.getString( 0, funcStr, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Function must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Function must be function" ) ;
         goto error ;
      }

      rc = _sptSdb.crtJSProcedure( funcStr.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create procedure" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::removeProcedure( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string func ;
      rc = arg.getString( 0, func ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Func must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Func must be string" ) ;
         goto error ;
      }
      rc = _sptSdb.rmProcedure( func.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove procedure" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done;
   }

   INT32 _sptDBSdb::listProcedures( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj cond ;

      rc = arg.getBsonobj( 0, cond ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Cond must be obj" ) ;
         goto error ;
      }

      rc = _sptSdb.listProcedures( &pCursor, cond ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list procedures" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::eval( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      string code ;
      SDB_SPD_RES_TYPE valueType = SDB_SPD_RES_TYPE_MAX ;
      BSONObj errMsg ;
      BSONObj nextRecord ;

      rc = arg.getString( 0, code, FALSE ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Code must be string or function" ) ;
         goto error ;
      }
      rc = _sptSdb.evalJS( code.c_str(), valueType, cursor, errMsg ) ;
      if( SDB_OK != rc )
      {
         const CHAR *pEerrDetail ;
         BSONElement ele = errMsg.getField( FMP_ERR_MSG ) ;
         if( String == ele.type() )
         {
            pEerrDetail = ele.valuestr() ;
         }
         else
         {
            pEerrDetail = getErrDesp( rc ) ;
         }
         detail = BSON( SPT_ERR << pEerrDetail ) ;
         goto error ;
      }

      if ( FMP_RES_TYPE_RECORDSET == valueType )
      {
         SPT_SET_CURSOR_TO_RETURNVAL( cursor.pCursor ) ;
         cursor.pCursor = NULL ;
      }
      else if( FMP_RES_TYPE_SPECIALOBJ == valueType )
      {
         BSONObj data ;
         BSONObj val ;
         string className ;
         const sptObjDesc *desc = NULL ;
         rc = cursor.next( data ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get special obj data" ) ;
            goto error ;
         }
         className = data.getStringField( FMP_RES_CLASSNAME ) ;
         val = data.getObjectField( FMP_RES_VALUE ) ;
         desc = sptGetObjFactory()->findObj( className ) ;
         if( NULL == desc )
         {
            rc = SDB_SYS ;
            detail = BSON( SPT_ERR << "Failed to get special obj desc" ) ;
            goto error ;
         }
         rc = desc->bsonToJSObj( _sptSdb, val, rval, detail ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         BSONElement ele ;
         BSONType type ;
         rc = cursor.next( nextRecord ) ;
         if( rc && SDB_DMS_EOC != rc )
         {
            detail = BSON( SPT_ERR << "Failed to fetch return data" ) ;
            goto error ;
         }

         ele = nextRecord.getField( FMP_RES_VALUE ) ;
         type = ele.type() ;

         /// is void
         if( EOO == type )
         {
            rc = SDB_OK ;
         }
         else if ( jstNULL == type )
         {
            rval.getReturnVal().setNull() ;
         }
         else if ( String == type )
         {
            rval.getReturnVal().setValue( ele.String() ) ;
         }
         else if ( Bool == type )
         {
            rval.getReturnVal().setValue( ele.Bool() ) ;
         }
         else if ( NumberDouble == type )
         {
            rval.getReturnVal().setValue( ele.Double() ) ;
         }
         else if ( NumberInt == type )
         {
            rval.getReturnVal().setValue( ele.Int() ) ;
         }
         else if ( NumberLong == type )
         {
            rval.getReturnVal().setValue( ele.Long() ) ;
         }
         else if ( Code == type )
         {
            rval.getReturnVal().setJSCode( ele.code().c_str() ) ;
         }
         else if ( Object == type )
         {
            rval.getReturnVal().setJSCode(
               ele.Obj().toString( FALSE, TRUE ).c_str() ) ;
         }
         else if ( Array == type )
         {
            rval.getReturnVal().setJSCode(
               ele.Obj().toString( TRUE, TRUE ).c_str() ) ;
         }
         else
         {
            detail = BSON( SPT_ERR << "Invaliad bson obj was fetched" ) ;
            goto error ;
         }
      }

   done:
      cursor.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::backup( const _sptArguments &arg,
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
      rc = _sptSdb.backup( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to backup" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::listBackup( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj opt ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;

      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, opt ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }

         if( arg.argc() > 1 )
         {
            rc = arg.getBsonobj( 1, cond ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Condition must be obj" ) ;
               goto error ;
            }
            if( arg.argc() > 2 )
            {
               rc = arg.getBsonobj( 2, sel ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Select must be obj" ) ;
                  goto error ;
               }
               if( arg.argc() > 3 )
               {
                  rc = arg.getBsonobj( 3, order ) ;
                  if( SDB_OK != rc )
                  {
                     detail = BSON( SPT_ERR << "Order must be obj" ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      rc = _sptSdb.listBackup( &pCursor, opt, cond, sel, order ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list backup" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::removeBackup( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
      }
      rc = _sptSdb.removeBackup( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove backup" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::listTasks( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor= NULL ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;

      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, cond ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Condition must be obj" ) ;
            goto error ;
         }
         if( arg.argc() > 1 )
         {
            rc = arg.getBsonobj( 1, sel ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Select must be obj" ) ;
               goto error ;
            }
            if( arg.argc() > 2 )
            {
               rc = arg.getBsonobj( 2, order ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Order must be obj" ) ;
                  goto error ;
               }
               if( arg.argc() > 3 )
               {
                  rc = arg.getBsonobj( 3, hint ) ;
                  if( SDB_OK != rc )
                  {
                     detail = BSON( SPT_ERR << "Hint must be obj" ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      rc = _sptSdb.listTasks( &pCursor, cond, sel, order, hint ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list tasks" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::waitTasks( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT32 num = arg.argc() ;
      SINT64 *taskIDs = NULL ;
      if( num < 1 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR <<
                        "Sdb.waitTasks() need at lease one task id for argument." ) ;
         goto error ;
      }
      taskIDs = (SINT64*)SDB_OSS_MALLOC( num * sizeof( SINT64 ) ) ;
      for( INT32 index = 0; index < num; index++ )
      {
         rc = arg.getNative( 0, &taskIDs[index], SPT_NATIVE_INT64 ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "TaskID must be number" ) ;
            goto error ;
         }
      }
      rc = _sptSdb.waitTasks( taskIDs, num ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to wait tasks" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::cancelTask( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 isAsync = FALSE ;
      SINT64 taskID = 0 ;
      if( arg.argc() < 1 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR <<
                        "Sdb.cancelTask(): need at least one arguments" ) ;
         goto error ;
      }
      rc = arg.getNative( 0, &taskID, SPT_NATIVE_INT64 ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "TaskID must be number" ) ;
         goto error ;
      }
      rc = arg.getNative( 1, &isAsync, SPT_NATIVE_INT32 ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "IsAsync must be Bool" ) ;
         goto error ;
      }
      rc = _sptSdb.cancelTask( taskID, isAsync ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to cancel task" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::getSessionAttr ( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     bson::BSONObj & detail )
   {
      INT32 rc = SDB_OK ;

      bson::BSONObj result ;
      sptBsonobj * sptResult = NULL ;

      rc = _sptSdb.getSessionAttr( result, FALSE ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get session attributes" ) ;
         goto error ;
      }

      sptResult = SDB_OSS_NEW sptBsonobj( result ) ;
      if( NULL == sptResult )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptBsonobj obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< sptBsonobj >( sptResult ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }

   done :
      return rc ;

   error :
      SAFE_OSS_DELETE( sptResult ) ;
      goto done ;
   }

   INT32 _sptDBSdb::setSessionAttr( const _sptArguments &arg,
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
      rc = _sptSdb.setSessionAttr( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set session attr" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::msg( const _sptArguments &arg,
              _sptReturnVal &rval,
              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string msg ;
      rc = arg.getString( 0, msg ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Msg must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Msg must be string" ) ;
         goto error ;
      }
      rc = _sptSdb.msg( msg.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to send test msg" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::createDomain( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbDomain *pDM = NULL ;
      string domainName ;
      BSONArray bsArray ;
      BSONObj options ;
      BSONObjBuilder builder ;
      rc = arg.getString( 0, domainName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Domain name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Domain name must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, bsArray ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Groups must be array" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 2, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      builder.appendArray( FIELD_NAME_GROUPS, bsArray ) ;
      builder.appendElementsUnique( options ) ;
      rc = _sptSdb.createDomain( domainName.c_str(), builder.obj(), &pDM ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create domain" ) ;
         goto error ;
      }
      SPT_SET_DOMAIN_TO_RETURNVAL( pDM ) ;
      rval.addReturnValProperty( "_domainname" )->setValue( domainName ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pDM ) ;
      goto done ;
   }

   INT32 _sptDBSdb::dropDomain( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string domainName ;
      rc = arg.getString( 0, domainName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Domain name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Domain name must be string" ) ;
         goto error ;
      }
      rc = _sptSdb.dropDomain( domainName.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop domain" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::getDomain( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string name ;
      _sdbDomain *pDomain = NULL ;
      rc = arg.getString( 0, name ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Name must be string" ) ;
         goto error ;
      }
      rc = _sptSdb.getDomain( name.c_str(), &pDomain ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get domain" ) ;
         goto error ;
      }
      SPT_SET_DOMAIN_TO_RETURNVAL( pDomain ) ;
      rval.addReturnValProperty( SPT_DOMAIN_NAME_FIELD )->setValue( name ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pDomain ) ;
      goto done ;
   }

   INT32 _sptDBSdb::listDomains( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;

      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, cond ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Condition must be obj" ) ;
            goto error ;
         }
         if( arg.argc() > 1 )
         {
            rc = arg.getBsonobj( 1, sel ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Select must be obj" ) ;
               goto error ;
            }
            if( arg.argc() > 2 )
            {
               rc = arg.getBsonobj( 2, order ) ;
               if( SDB_OK != rc )
               {
                  detail = BSON( SPT_ERR << "Order must be obj" ) ;
                  goto error ;
               }
               if( arg.argc() > 3 )
               {
                  rc = arg.getBsonobj( 3, hint ) ;
                  if( SDB_OK != rc )
                  {
                     detail = BSON( SPT_ERR << "Hint must be obj" ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      rc = _sptSdb.listDomains( &pCursor, cond, sel, order, hint ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list domains" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBSdb::invalidateCache( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      rc = arg.getBsonobj( 0, condition ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Condition should be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.invalidateCache( condition ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to invalidate cache" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::forceSession( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 sessionID = -1 ;
      BSONObj condition ;
      BSONObj options ;
      rc = arg.getBsonobj( 0, condition ) ;

      if( 1 != arg.argc() && 2 != arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Sdb.forceSession():need one or two arguments" ) ;
         goto error ;
      }
      // Get sessionID
      if( arg.isInt( 0 ) )
      {
         rc = arg.getNative( 0, &sessionID, SPT_NATIVE_INT64 ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get sessioID" ) ;
            goto error ;
         }
      }
      else if( arg.isString( 0 ) )
      {
         string sessionStr ;
         rc = arg.getString( 0, sessionStr ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to get sessioID" ) ;
            goto error ;
         }
         {
            istringstream inStr( sessionStr ) ;
            inStr >> sessionID ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "SessionID must be string or int" ) ;
         goto error ;
      }
      if( -1 == sessionID )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "SessionID: invalid argument" ) ;
         goto error ;
      }
      // Get options
      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.forceSession( sessionID, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to force stop session" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::forceStepUp( const _sptArguments &arg,
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
      rc = _sptSdb.forceStepUp( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to force step up" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::sync( const _sptArguments &arg,
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
      rc = _sptSdb.syncDB( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to sync db" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::loadCS( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string csName ;
      BSONObj options ;
      rc = arg.getString( 0, csName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CSName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CSName must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.loadCS( csName.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to load cs" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::unloadCS( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string csName ;
      BSONObj options ;
      rc = arg.getString( 0, csName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CSName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CSName must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.unloadCS( csName.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to unload cs" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::setPDLevel( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 level = -1 ;
      BSONObj options ;
      rc = arg.getNative( 0, &level, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Level must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Level must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.setPDLevel( level, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set PDLevel" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::reloadConf( const _sptArguments &arg,
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
      rc = _sptSdb.reloadConfig( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to reload configs" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::renameCS( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oldName ;
      string newName ;
      BSONObj options ;
      rc = arg.getString( 0, oldName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Old name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Old name must be string" ) ;
         goto error ;
      }
      rc = arg.getString( 1, newName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "New name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "New name must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 2, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.renameCollectionSpace( oldName.c_str(), newName.c_str(),
                                          options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to rename collection space" ) ;
         goto error ;
      }
      rval.addSelfProperty( oldName )->setDelete() ;
   done:
      return rc ;
   error:
      goto done ;

   }

   INT32 _sptDBSdb::updateConfig( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj configs ;
      BSONObj options ;
      rc = arg.getBsonobj( 0, configs, TRUE, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Configs must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         if ( arg.hasErrMsg() )
         {
            detail = BSON( SPT_ERR << arg.getErrMsg() ) ;
         }
         else
         {
            detail = BSON( SPT_ERR << "Config must be object" ) ;
         }
         goto error ;
      }

      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.updateConfig( configs, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to update configs" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::deleteConfig( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj configs ;
      BSONObj options ;
      rc = arg.getBsonobj( 0, configs, TRUE, FALSE ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Configs must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         if ( arg.hasErrMsg() )
         {
            detail = BSON( SPT_ERR << arg.getErrMsg() ) ;
         }
         else
         {
            detail = BSON( SPT_ERR << "Config must be object" ) ;
         }
         goto error ;
      }

      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _sptSdb.deleteConfig( configs, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to delete configs" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::resolve( const _sptArguments &arg,
                             UINT32 opcode,
                             BOOLEAN &processed,
                             string &callFunc,
                             BOOLEAN &setIDProp,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      if( SPT_JSOP_GETPROP == opcode )
      {
         callFunc = "_resolveCS" ;
         processed = TRUE ;
      }
      return SDB_OK ;
   }

   INT32 _sptDBSdb::_getCSAndSetProperty( const string &csName,
                                          _sptReturnVal &rval,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCollectionSpace *ptrCs = NULL ;
      sptDBCS *sptCS = NULL ;
      rc = _sptSdb.getCollectionSpace( csName.c_str(), &ptrCs ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get collectionSpace" ) ;
         goto error ;
      }
      // build return cs obj
      sptCS = SDB_OSS_NEW sptDBCS( ptrCs ) ;
      if( NULL == sptCS )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new spt cs obj" ) ;
         goto error ;
      }
      ptrCs = NULL ;

      rc = rval.setUsrObjectVal< sptDBCS >( sptCS ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set user obj" ) ;
         goto error ;
      }
      sptCS = NULL ;

      rval.getReturnVal().setName( csName ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_CS_NAME_FIELD )->setValue( csName ) ;
      rval.addSelfToReturnValProperty( SPT_CS_CONN_FIELD ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( ptrCs ) ;
      SAFE_OSS_DELETE( sptCS ) ;
      goto done ;
   }

   INT32 _sptDBSdb::_getRGAndSetProperty( const string &rgName,
                                          _sptReturnVal &rval,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbReplicaGroup *pRG = NULL ;
      rc = _sptSdb.getReplicaGroup( rgName.c_str(), &pRG ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get rg" ) ;
         goto error ;
      }
      SPT_SET_RG_TO_RETURNVAL( pRG ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pRG ) ;
      goto done ;
   }

   INT32 _sptDBSdb::_getRGAndSetProperty( INT32 id, _sptReturnVal &rval,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbReplicaGroup *pRG = NULL ;
      rc = _sptSdb.getReplicaGroup( id, &pRG ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get rg" ) ;
         goto error ;
      }
      SPT_SET_RG_TO_RETURNVAL( pRG ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pRG ) ;
      goto done ;
   }

   INT32 _sptDBSdb::_getUserInfoFromCipherUserObj( const BSONObj &cipherUserObj,
                                                   sptDBCipherUser *pCipherUser,
                                                   string &username,
                                                   string &passwd,
                                                   bson::BSONObj &detail )
   {
      INT32  rc = SDB_OK ;
      string token ;
      string clusterName ;
      // userFullName = userShortName + '@' + clusterName
      string userFullName ;
      string cipherFile ;
      stringstream ss ;
      passwd::utilPasswordTool passwdTool ;

      BSONObjIterator itr( cipherUserObj ) ;
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_USER ) &&
              String == ele.type() )
         {
            // userShortName == username
            username = ele.valuestr() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                   SPT_CIPHERUSER_FIELD_NAME_CIPHER_FILE ) &&
                   String == ele.type() )
         {
            cipherFile = ele.valuestr() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                   SPT_CIPHERUSER_FIELD_NAME_CLUSTER_NAME )
                   && String == ele.type() )
         {
            clusterName = ele.valuestr() ;
         }
      }

      if ( !clusterName.empty() )
      {
         userFullName = username ;
         userFullName += "@" ;
         userFullName += clusterName ;
      }
      else
      {
         userFullName = username ;
      }

      token = pCipherUser->getToken() ;

      rc = passwdTool.getPasswdByCipherFile( userFullName, token,
                                             cipherFile, passwd ) ;
      if ( rc )
      {
         ss << "Failed to get user[" << userFullName.c_str()
            << "]'s passwd from cipher file["
            << cipherFile.c_str() << "]" ;
         detail = BSON( SPT_ERR << ss.str().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSdb::cvtToBSON( const CHAR* key, const sptObject &value,
                               BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                               string &errMsg )
   {
      errMsg = "Sdb can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBSdb::fmpToBSON( const sptObject &value, BSONObj &retObj,
                               string &errMsg )
   {
      errMsg = "Sdb obj can not be return" ;
      return SDB_SYS ;
   }

   INT32 _sptDBSdb::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                 _sptReturnVal &rval, bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "Sdb obj can not be return" ) ;
      return SDB_SYS ;
   }
}
