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

   Source File Name = rtnDataSet.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/06/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnDataSet.hpp"
#include "rtn.hpp"

using namespace bson ;

namespace engine
{

   _rtnDataSet::_rtnDataSet( const rtnQueryOptions &options,
                             _pmdEDUCB *cb )
   :_contextID( -1 ),
    _cb( cb ),
    _lastErr( SDB_OK ),
    _rtnCB( sdbGetRTNCB() ),
    _pBuff( NULL ),
    _ownned( FALSE )
   {
      initByQuery( options, cb ) ;
   }

   _rtnDataSet::_rtnDataSet( SINT64 contextID, _pmdEDUCB *cb )
   :_contextID( contextID ),
    _cb( cb ),
    _lastErr( SDB_OK ),
    _rtnCB( sdbGetRTNCB() ),
    _pBuff( NULL ),
    _ownned( FALSE )
   {
   }

   _rtnDataSet::_rtnDataSet( MsgOpReply *pReply, _pmdEDUCB *cb,
                             BOOLEAN ownned )
   :_contextID( -1 ),
    _cb( cb ),
    _lastErr( SDB_OK ),
    _rtnCB( sdbGetRTNCB() ),
    _pBuff( NULL ),
    _ownned( FALSE )
   {
      initByReply( pReply, cb, ownned ) ;
   }

   INT32 _rtnDataSet::initByQuery( const rtnQueryOptions &options,
                                   _pmdEDUCB *cb )
   {
      clear() ;

      _cb = cb ;
      rtnQueryOptions tempOptions( options ) ;
      INT32 rc = rtnQuery( tempOptions, _cb, sdbGetDMSCB(), _rtnCB,
                           _contextID, NULL, FALSE, FALSE ) ;
      _lastErr = rc ;

      return rc ;
   }

   INT32 _rtnDataSet::initByReply( MsgOpReply *pReply,
                                   _pmdEDUCB *cb,
                                   BOOLEAN ownned )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( -1 == pReply->contextID, "Context id must be -1" ) ;

      clear() ;

      _lastErr = pReply->flags ;
      _cb = cb ;

      rc = pReply->flags ;

      if ( SDB_OK == rc &&
           pReply->header.messageLength > (INT32)sizeof( MsgOpReply ) )
      {
         if ( !ownned )
         {
            _pBuff = ( CHAR* )pReply ;
         }
         else
         {
            _pBuff = ( CHAR* )SDB_OSS_MALLOC( pReply->header.messageLength ) ;
            if ( !_pBuff )
            {
               PD_LOG( PDERROR, "Alloc memory[%d] failed",
                       pReply->header.messageLength ) ;
               rc = SDB_OOM ;
               _lastErr = rc ;
               goto error ;
            }
            _ownned = TRUE ;
            ossMemcpy( _pBuff, pReply, pReply->header.messageLength ) ;
         }

         _contextBuf = rtnContextBuf( _pBuff + sizeof( MsgOpReply ),
                                      pReply->header.messageLength -
                                      sizeof( MsgOpReply ),
                                      pReply->numReturned ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnDataSet::~_rtnDataSet()
   {
      clear() ;
   }

   void _rtnDataSet::clear()
   {
      _contextBuf.release() ;

      if ( -1 != _contextID && _cb )
      {
         sdbGetRTNCB()->contextDelete( _contextID, _cb ) ;
         _contextID = -1 ;
      }
      if ( _pBuff && _ownned )
      {
         SDB_OSS_FREE( _pBuff ) ;
      }
      _pBuff = NULL ;
      _ownned = FALSE ;

      _lastErr = SDB_OK ;
   }

   INT32 _rtnDataSet::next( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      if ( SDB_OK != _lastErr )
      {
         rc = _lastErr ;
         goto error ;
      }

      if ( _contextBuf.eof() && -1 != _contextID )
      {
         rc = rtnGetMore( _contextID, -1, _contextBuf, _cb, _rtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Get more from context[%lld] failed, rc: %d",
                       _contextID, rc ) ;
            }
            _contextID = -1 ;
            _lastErr = rc ;
            goto error ;
         }
      }

      rc = _contextBuf.nextObj( obj ) ;
      if ( rc )
      {
         _lastErr = rc ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

