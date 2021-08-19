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

   Source File Name = rtnSubContext.cpp

   Descriptive Name = RunTime Sub-Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/28/2017   David Li  draft

   Last Changed =

*******************************************************************************/
#include "rtnSubContext.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   _rtnOrderKey::_rtnOrderKey ( const _rtnOrderKey &orderKey )
   {
      _orderBy = orderKey._orderBy ;
      _hash = orderKey._hash ;
      _keyObj = orderKey._keyObj ;
      _arrEle = orderKey._arrEle ;
   }

   _rtnOrderKey::_rtnOrderKey ()
   {
      _hash.hash = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNORDERKEY_OPLT, "_rtnOrderKey::operator<" )
   BOOLEAN _rtnOrderKey::operator<( const _rtnOrderKey &rhs ) const
   {
      PD_TRACE_ENTRY ( SDB_RTNORDERKEY_OPLT ) ;
      BOOLEAN result = FALSE ;
      INT32 rsCmp = _keyObj.woCompare( rhs._keyObj, _orderBy, FALSE ) ;
      if ( rsCmp < 0
         || ( 0 == rsCmp && _hash.hash < rhs._hash.hash ))
      {
         result = TRUE ;
      }
      PD_TRACE1 ( SDB_RTNORDERKEY_OPLT, PD_PACK_INT(result) );
      PD_TRACE_EXIT ( SDB_RTNORDERKEY_OPLT ) ;
      return result;
   }

   void _rtnOrderKey::clear()
   {
      _arrEle = BSONElement() ;
      _hash.columns.hash1 = 0 ;
      _hash.columns.hash2 = 0 ;
   }

   void _rtnOrderKey::setOrderBy( const BSONObj &orderBy )
   {
      _orderBy = orderBy;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNORDERKEY_GENKEY, "_rtnOrderKey::generateKey" )
   INT32 _rtnOrderKey::generateKey( const BSONObj &record,
                                    _ixmIndexKeyGen *keyGen )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( keyGen != NULL, "keyGen can't be null!" ) ;
      PD_TRACE_ENTRY ( SDB_RTNORDERKEY_GENKEY ) ;
      clear();
      BSONObjSet keySet( _orderBy ) ;

      rc = keyGen->getKeys( record, keySet, &_arrEle ) ;
      PD_RC_CHECK( rc, PDERROR,
                  "failed to generate order-key(rc=%d)",
                  rc ) ;
      SDB_ASSERT( !keySet.empty(), "empty key-set!" ) ;
      _keyObj = *(keySet.begin()) ;
      if ( _arrEle.eoo() )
      {
         _hash.hash = 0 ;
      }
      else
      {
         ixmMakeHashValue( _arrEle, _hash ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNORDERKEY_GENKEY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnSubContext::_rtnSubContext( const BSONObj& orderBy,
                                   _ixmIndexKeyGen* keyGen,
                                   INT64 contextID )
   {
      _orderKey.setOrderBy( orderBy ) ;
      _isOrderKeyChange = TRUE ;
      _keyGen = keyGen ;
      _contextID = contextID ;
      _startFrom = 0 ;
   }

   _rtnSubContext::~_rtnSubContext()
   {
      _keyGen = NULL ;
      _contextID = -1 ;
   }
}
