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

   Source File Name = sptBsonobjArray.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptBsonobjArray.hpp"

using namespace bson ;

namespace engine
{

   /*
      _sptBsonobjArray implement
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptBsonobjArray, construct)
   JS_DESTRUCT_FUNC_DEFINE( _sptBsonobjArray, destruct)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, size)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, more)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, next)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, pos)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, getIndex)
   JS_RESOLVE_FUNC_DEFINE(_sptBsonobjArray, resolve)

   JS_BEGIN_MAPPING( _sptBsonobjArray, "BSONArray" )
     JS_ADD_MEMBER_FUNC( "size", size )
     JS_ADD_MEMBER_FUNC( "more", more )
     JS_ADD_MEMBER_FUNC( "next", next )
     JS_ADD_MEMBER_FUNC( "pos", pos )
     JS_ADD_MEMBER_FUNC( "index", getIndex )
     JS_ADD_CONSTRUCT_FUNC( construct )
     JS_ADD_DESTRUCT_FUNC( destruct )
     JS_ADD_RESOLVE_FUNC(resolve)
     JS_SET_CVT_TO_BSON_FUNC( _sptBsonobjArray::cvtToBSON )
     JS_SET_JSOBJ_TO_BSON_FUNC( _sptBsonobjArray::fmpToBSON )
     JS_SET_BSON_TO_JSOBJ_FUNC( _sptBsonobjArray::bsonToJSObj )

   JS_MAPPING_END()

   _sptBsonobjArray::_sptBsonobjArray()
   {
      _curPos = 0 ;
   }

   _sptBsonobjArray::_sptBsonobjArray( const vector< BSONObj > &vecObjs )
   {
      for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
      {
         _vecObj.push_back( vecObjs[ i ].getOwned() ) ;
      }
      _curPos = 0 ;
   }

   _sptBsonobjArray::~_sptBsonobjArray()
   {
   }

   INT32 _sptBsonobjArray::construct( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "new BSONObjArray is forbidden." ) ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptBsonobjArray::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::size( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      INT32 size = _vecObj.size() ;
      rval.getReturnVal().setValue( size ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::more( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      BOOLEAN hasMore = FALSE ;
      if ( _curPos < _vecObj.size() )
      {
         hasMore = TRUE ;
      }
      rval.getReturnVal().setValue( hasMore ? true : false ) ;

      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::next( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      if ( _curPos < _vecObj.size() )
      {
         rval.getReturnVal().setValue( _vecObj[_curPos] ) ;
         ++_curPos ;
      }

      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::pos( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      if ( _curPos < _vecObj.size() )
      {
         rval.getReturnVal().setValue( _vecObj[_curPos] ) ;
      }
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::getIndex( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      rval.getReturnVal().setValue( _curPos ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::resolve( const _sptArguments &arg,
                                    UINT32 opcode,
                                    BOOLEAN &processed,
                                    string &callFunc,
                                    BOOLEAN &setIDProp,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      if ( arg.isInt( 0 ) )
      {
         INT32 idValue = 0 ;
         rc = arg.getNative( 0, (void*)&idValue, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "The 1st param must be INT32 value" ) ;
            goto error ;
         }
         if ( idValue < (INT32)_vecObj.size() )
         {
            /// set the return value
            rval.getReturnVal().setValue( _vecObj[idValue] ) ;
         }
         processed = TRUE ;
         setIDProp = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobjArray::fmpToBSON( const sptObject &value,
                                      BSONObj &retObj,
                                      string &errMsg )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder builder ;
      _sptBsonobjArray *pBsonArray = NULL ;
      rc = value.getUserObj( _sptBsonobjArray::__desc,
                             (const void **)&pBsonArray ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get BSONObjArray field" ;
         goto error ;
      }

      {
         const vector< BSONObj >& vecObj = pBsonArray->getBsonArray() ;
         for ( UINT32 i = 0 ; i < vecObj.size() ; ++i )
         {
            builder.append( vecObj[i] ) ;
         }
         retObj = builder.arr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobjArray::cvtToBSON( const CHAR *key,
                                      const sptObject &value,
                                      BOOLEAN isSpecialObj,
                                      BSONObjBuilder &builder,
                                      string &errMsg )
   {
      INT32 rc = SDB_OK ;
      _sptBsonobjArray *pBsonArray = NULL ;
      rc = value.getUserObj( _sptBsonobjArray::__desc,
                             (const void **)&pBsonArray ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get BSONObjArray field" ;
         goto error ;
      }

      {
         const vector< BSONObj >& vecObj = pBsonArray->getBsonArray() ;
         BSONArrayBuilder subBuild( builder.subarrayStart( key ) ) ;
         for ( UINT32 i = 0 ; i < vecObj.size() ; ++i )
         {
            subBuild.append( vecObj[i] ) ;
         }
         subBuild.done() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobjArray::bsonToJSObj( sdbclient::sdb &db,
                                        const BSONObj &data,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      vector< BSONObj > vecObj ;
      _sptBsonobjArray *pBsonArray = NULL ;

      BSONObjIterator itr( data ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( Object != e.type() )
         {
            detail = BSON( SPT_ERR << "Data is not Object Array" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         vecObj.push_back( e.embeddedObject() ) ;
      }

      pBsonArray = SDB_OSS_NEW _sptBsonobjArray( vecObj ) ;
      if ( !pBsonArray )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new _sptBsonobjArray" ) ;
         goto error ;
      }

      rval.setUsrObjectVal<_sptBsonobjArray>( pBsonArray ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

