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

   Source File Name = sptDBDC.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBDC.hpp"
#include <string>
using namespace std ;

namespace engine
{
   #define SPT_DC_NAME  "SdbDC"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBDC, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBDC, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, createImage )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, removeImage )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, attachGroups )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, detachGroups )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, enableImage )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, disableImage )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, activate )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, deactivate )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, enableReadOnly )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, disableReadOnly )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, getDetail )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, setActiveLocation )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, setLocation )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, startMaintenanceMode )
   JS_MEMBER_FUNC_DEFINE( _sptDBDC, stopMaintenanceMode )

   JS_BEGIN_MAPPING( _sptDBDC, SPT_DC_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "createImage", createImage )
      JS_ADD_MEMBER_FUNC( "removeImage", removeImage )
      JS_ADD_MEMBER_FUNC( "attachGroups", attachGroups )
      JS_ADD_MEMBER_FUNC( "detachGroups", detachGroups )
      JS_ADD_MEMBER_FUNC( "enableImage", enableImage )
      JS_ADD_MEMBER_FUNC( "disableImage", disableImage )
      JS_ADD_MEMBER_FUNC( "activate", activate )
      JS_ADD_MEMBER_FUNC( "deactivate", deactivate )
      JS_ADD_MEMBER_FUNC( "enableReadonly", enableReadOnly )
      JS_ADD_MEMBER_FUNC( "disableReadonly", disableReadOnly )
      JS_ADD_MEMBER_FUNC( "getDetail", getDetail )
      JS_ADD_MEMBER_FUNC( "setActiveLocation", setActiveLocation )
      JS_ADD_MEMBER_FUNC( "setLocation", setLocation )
      JS_ADD_MEMBER_FUNC( "startMaintenanceMode", startMaintenanceMode )
      JS_ADD_MEMBER_FUNC( "stopMaintenanceMode", stopMaintenanceMode )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBDC::cvtToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBDC::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBDC::_sptDBDC( _sdbDataCenter *pDC )
   {
      _dc.pDC = pDC ;
   }

   _sptDBDC::~_sptDBDC()
   {
   }

   INT32 _sptDBDC::construct( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR <<
                     "use of new SdbDC() is forbidden, you should use "
                     "other functions to produce a SdbDC object"  );
      return SDB_SYS ;
   }

   INT32 _sptDBDC::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBDC::createImage( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string cataAddrList ;
      rc = arg.getString( 0, cataAddrList ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "CataAddrList must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "CataAddrList must be string" ) ;
         goto error ;
      }

      rc = _dc.createImage( cataAddrList.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create image" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::removeImage( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _dc.removeImage() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove image" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::attachGroups( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _dc.attachGroups( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to attach groups" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::detachGroups( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _dc.detachGroups( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to detach groups" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::enableImage( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _dc.enableImage() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to enable image" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::disableImage( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _dc.disableImage() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to disable image" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::activate( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _dc.activateDC() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to activate image" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::deactivate( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _dc.deactivateDC() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to deactivate image" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::enableReadOnly( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _dc.enableReadOnly( TRUE ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to enable readonly" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::disableReadOnly( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _dc.enableReadOnly( FALSE ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to disable readonly" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::getDetail( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      rc = _dc.getDetail( retObj ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get detail" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::setActiveLocation( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string locationName ;

      rc = arg.getString( 0, locationName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Location name can't be null" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Location name must be string" ) ;
         goto error ;
      }

      rc = _dc.setActiveLocation( locationName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set active location" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::setLocation( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostName ;
      string locationName ;

      rc = arg.getString( 0, hostName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Host name can't be null" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Host name must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, locationName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Location name can't be null" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Location name must be string" ) ;
         goto error ;
      }

      rc = _dc.setLocation( hostName.c_str() ,locationName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set location" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::startMaintenanceMode( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      if ( arg.argc() == 0 )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "Options can't be null" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, options ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }

      rc = _dc.startMaintenanceMode( options ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to start maintenance mode" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::stopMaintenanceMode( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      if ( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, options ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }
      }

      rc = _dc.stopMaintenanceMode( options ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to stop maintenance mode" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDC::cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg )
   {
      errMsg = "SdbDC can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBDC::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string dcName ;
      _sdbDataCenter *pDC = NULL ;
      sptDBDC *pSptDC = NULL ;
      rc = db.getDC( &pDC  ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get sdbDC" ) ;
         goto error ;
      }
      pSptDC = SDB_OSS_NEW sptDBDC( pDC ) ;
      if( NULL == pSptDC )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBDC obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBDC >( pSptDC ) ;
      if( SDB_OK != rc )
      {
         SAFE_OSS_DELETE( pSptDC ) ;
         pDC = NULL ;
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }
      rval.getReturnVal().setName( pDC->getName() ) ;
      rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_DC_NAME_FIELD )->setValue( dcName ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pDC ) ;
      goto done ;
   }
}
