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

   /* static function define */
   JS_STATIC_FUNC_DEFINE(_sptBsonobjArray, help)

   JS_BEGIN_MAPPING( _sptBsonobjArray, "BSONArray" )
     JS_ADD_MEMBER_FUNC( "size", size )
     JS_ADD_MEMBER_FUNC( "more", more )
     JS_ADD_MEMBER_FUNC( "next", next )
     JS_ADD_MEMBER_FUNC( "pos", pos )
     JS_ADD_MEMBER_FUNC( "index", getIndex )
     JS_ADD_CONSTRUCT_FUNC( construct )
     JS_ADD_DESTRUCT_FUNC( destruct )
     JS_ADD_RESOLVE_FUNC(resolve)
     /* static function */
     JS_ADD_STATIC_FUNC("help", help)
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

   INT32 _sptBsonobjArray::help( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      stringstream ss ;
      ss << "BSONArray functions:" << endl
         << " BSONArray(obj).size()" << endl
         << " BSONArray(obj).more()" << endl
         << " BSONArray(obj).next()" << endl
         << " BSONArray(obj).pos()" << endl
         << " BSONArray(obj).toArray()" << endl
         << " BSONArray(obj).toString()" << endl
         << " BSONArray(obj).index()" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
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

}

