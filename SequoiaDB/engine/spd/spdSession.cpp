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

   Source File Name = spdSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "spdSession.hpp"
#include "spdFuncDownloader.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "spdFMPMgr.hpp"
#include "spdFMP.hpp"
#include "fmpDef.hpp"

using namespace bson ;

#define SPD_GET_RES( res )\
        do\
        {\
           rc = _fmp->read( (res), _cb ) ;\
           if ( SDB_OK != rc )\
           {\
              PD_LOG( PDERROR, "failed read msg from fmp:%d", rc ) ;\
              goto error ;\
           }\
           rc = _resIsOk( (res) ) ;\
           if ( SDB_OK != rc )\
           {\
             PD_LOG( PDERROR, "get res:%s", res.toString().c_str() ) ;\
             _errmsg = res ;\
             goto error ;\
           }\
        } while ( FALSE )

namespace engine
{
   _spdSession::_spdSession()
   :_resType(FMP_RES_TYPE_VOID),
    _fmpMgr( NULL ),
    _cb( NULL ),
    _fmp( NULL )
   {

   }

   _spdSession::~_spdSession()
   {
      if ( NULL != _fmp )
      {
         _fmpMgr->returnFMP( _fmp, _cb ) ;
      }
   }

   INT32 _spdSession::eval( const BSONObj &procedures,
                            _spdFuncDownloader *downloader,
                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != downloader, "impossible" ) ;

      if ( NULL == _fmp )
      {
         _fmpMgr = pmdGetKRCB()->getFMPCB() ;

         rc = _fmpMgr->getFMP( _fmp ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get fmp:%d", rc ) ;
            goto error ;
         }
         SDB_ASSERT( NULL != _fmp, "impossible" ) ;
      }

      _cb = cb ;

      rc = _eval( procedures, downloader ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdSession::next( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( FMP_RES_TYPE_VOID != _resType, "impossible" ) ;
      BOOLEAN fetchFromLocal = FMP_RES_TYPE_RECORDSET == _resType ?
                               FALSE : TRUE ;
      BSONObj tmp ;
      BSONElement value ;
      if ( fetchFromLocal && _resmsg.isEmpty() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else if ( fetchFromLocal )
      {
         obj = _resmsg ;
         _resmsg = BSONObj() ;
      }
      else if ( !fetchFromLocal )
      {
         /// use a static bsonobj.
         rc = _fmp->write( BSON( FMP_CONTROL_FIELD <<
                                 FMP_CONTROL_STEP_FETCH ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "failed to write fetch msg:%d", rc ) ;
            goto error ;
         }

         SPD_GET_RES( tmp ) ;
         value = tmp.getField( FMP_RES_VALUE ) ;
         if ( Object != value.type() )
         {
            PD_LOG_MSG( PDERROR, "invalid type of obj fetched:%s",
                        tmp.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         obj = value.embeddedObject() ;
      }
      else
      {
         SDB_ASSERT( FALSE, "impossible" ) ;
      }
   done:
      return rc ;
   error:
      if ( !_errmsg.isEmpty() )
      {
         BSONElement e = _errmsg.getField( FMP_ERR_MSG ) ;
         if ( String == e.type() )
         {
            PD_LOG_MSG( PDERROR, e.valuestr() ) ;
         }
      }
      goto done ;
   }

   INT32 _spdSession::_eval( const BSONObj &procedures,
                             _spdFuncDownloader *downloader )
    {
       INT32 rc = SDB_OK ;
       BSONObj cnMsg ;
       INT32 funcType = FMP_FUNC_TYPE_INVALID ;

       /// 1. begin: {FMP_CONTROL_FIELD : FMP_CONTROL_STEP_BEGIN,
       //             FMP_FUNC_TYPE : FMP_FUNC_TYPE_XX }
       {
       BSONObjBuilder builder ;
       BSONObj resMsg ;
       BSONElement type = procedures.getField( FMP_FUNC_TYPE ) ;
       if ( type.eoo() || NumberInt != type.type() )
       {
          PD_LOG( PDERROR, "failed to get func type: %s",
                  procedures.toString().c_str() ) ;
          rc = SDB_INVALIDARG ;
          _errmsg = BSON( FMP_ERR_MSG << "Failed to get func type" <<
                          FMP_RES_CODE << rc ) ;
          goto error ;
       }

       builder.append( FMP_CONTROL_FIELD, FMP_CONTROL_STEP_BEGIN ) ;
       builder.append( type ) ;
       builder.append( FMP_DIAG_PATH, pmdGetOptionCB()->getDiagLogPath() ) ;
       builder.append( FMP_SEQ_ID, _fmp->getSeqID() ) ;
       builder.append( FMP_LOCAL_SERVICE, pmdGetOptionCB()->getServiceAddr() ) ;
       builder.append( FMP_LOCAL_USERNAME, _cb->getUserName() ) ;
       builder.append( FMP_LOCAL_PASSWORD, _cb->getPassword() ) ;

       rc = _fmp->write( builder.obj() ) ;
       if ( SDB_OK != rc )
       {
          PD_LOG( PDERROR, "failed to write begin msg:%d", rc ) ;
          _errmsg = BSON( FMP_ERR_MSG << "Failed to write begin msg" <<
                          FMP_RES_CODE << rc ) ;
          goto error ;
       }

       SPD_GET_RES( resMsg ) ;
       PD_LOG( PDDEBUG, "begine res:%s", resMsg.toString().c_str() ) ;

       funcType = type.Int() ;
       }

       /// 2. eval exsit func :{ funcobj }
       /// do not add cn field coz we do not want to copy obj again.
       {
       BSONObj func ;
       BSONObj resMsg ;
       rc = downloader->download( BSON( FMP_FUNC_TYPE << funcType ) ) ;
       if ( SDB_OK != rc )
       {
          PD_LOG( PDERROR, "failed to download func:%d", rc ) ;
          _errmsg = BSON( FMP_ERR_MSG << "Failed to download funcs" <<
                          FMP_RES_CODE << rc ) ;
          goto error ;
       }

       while ( TRUE )
       {
          func = BSONObj() ;
          rc = downloader->next( func ) ;
          if ( SDB_OK == rc )
          {
             rc = _fmp->write( func ) ;
             if ( SDB_OK != rc )
             {
                PD_LOG( PDERROR, "failed to write func msg:%d", rc ) ;
                _errmsg = BSON( FMP_ERR_MSG << "Failed to write func msg" <<
                                FMP_RES_CODE << rc ) ;
                goto error ;
             }

             SPD_GET_RES( resMsg ) ;
             PD_LOG( PDDEBUG, "pre eval res:%s", resMsg.toString().c_str() ) ;
          }
          else if ( SDB_DMS_EOC == rc )
          {
             break ;
          }
          else
          {
             PD_LOG( PDERROR, "failed to download func:%d", rc ) ;
             _errmsg = BSON( FMP_ERR_MSG << "Failed to download funcs" <<
                             FMP_RES_CODE << rc ) ;
             goto error ;
          }
       }
       }

       /// 3.eval : { FMP_CONTROL_FIELD : FMP_CONTROL_STEP_EVAL,
       //             funcobj.elements }
       {
       BSONObjBuilder builder ;
       BSONObj resMsg ;
       builder.append( FMP_CONTROL_FIELD, FMP_CONTROL_STEP_EVAL ) ;
       BSONObjIterator ite( procedures ) ;
       while ( ite.more() )
       {
          builder.append( ite.next() ) ;
       }

       rc = _fmp->write( builder.obj() ) ;
       if ( SDB_OK != rc )
       {
          PD_LOG( PDERROR, "failed to write procedures msg:%d", rc ) ;
          _errmsg = BSON( FMP_ERR_MSG << "Failed to write procedures msg" <<
                          FMP_RES_CODE << rc ) ;
          goto error ;
       }

       SPD_GET_RES( resMsg ) ;
       PD_LOG( PDDEBUG, "eval res:%s", resMsg.toString().c_str() ) ;

       /// 4. prepare result
       _resmsg = resMsg ;
       {
       BSONObjBuilder builder ;
       BSONElement resType = _resmsg.getField( FMP_RES_TYPE ) ;
       SDB_ASSERT( NumberInt == resType.type(), "impossible" ) ;
       /// if it is not a record set, fetch from local.
       _resType = resType.Int() ;

       if ( FMP_RES_TYPE_VOID != _resType &&
            FMP_RES_TYPE_RECORDSET != _resType )
       {
          BSONElement value = _resmsg.getField( FMP_RES_VALUE ) ;
          SDB_ASSERT( !value.eoo(), "impossible" ) ;
          builder.append( value ) ;
          if( FMP_RES_TYPE_SPECIALOBJ == _resType )
          {
             BSONElement className = _resmsg.getField( FMP_RES_CLASSNAME ) ;
             SDB_ASSERT( !className.eoo(), "impossible" ) ;
             builder.append( className ) ;
          }
          _resmsg = builder.obj() ;
       }
       }
       }
    done:
       return rc ;
    error:
       if ( _errmsg.isEmpty() )
       {
          _errmsg = BSON( FMP_RES_CODE << rc ) ;
       }
       goto done ;
    }


    INT32 _spdSession::_resIsOk( const BSONObj &res )
    {
       BOOLEAN rc = SDB_OK ;
       BSONElement ele = res.getField( FMP_RES_CODE ) ;
       if ( res.isEmpty() )
       {
          SDB_ASSERT( FALSE, "impossible" ) ;
       }
       else if ( ele.eoo() )
       {
          /// default is ok
       }
       else if ( NumberInt != ele.type() )
       {
          SDB_ASSERT( FALSE, "impossible" ) ;
       }
       else
       {
          rc = ele.Int() ;
       }
       return rc ;
    }
}
