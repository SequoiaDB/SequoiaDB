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

   Source File Name = sptBsonobj.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptBsonobj.hpp"

using namespace bson ;

namespace engine
{

   /*
      _sptBsonobj implement
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptBsonobj, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptBsonobj, destruct)
   JS_MEMBER_FUNC_DEFINE_NORESET( _sptBsonobj, toJson )

   JS_BEGIN_MAPPING( _sptBsonobj, "BSONObj" )
     JS_ADD_MEMBER_FUNC( "toJson", toJson )
     JS_ADD_CONSTRUCT_FUNC( construct )
     JS_ADD_DESTRUCT_FUNC( destruct )
   JS_MAPPING_END()

   _sptBsonobj::_sptBsonobj()
   {

   }

   _sptBsonobj::_sptBsonobj( const bson::BSONObj &obj )
   {
      _obj = obj.copy() ;
   }

   _sptBsonobj::~_sptBsonobj()
   {

   }

   INT32 _sptBsonobj::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail)
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = arg.getBsonobj( 0, obj ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "The 1st param must be Object" ) ;
         goto error ;
      }
      _obj = obj ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptBsonobj::toJson( const _sptArguments &arg,
                              _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      rval.getReturnVal().setValue( _obj.toString( false, true ) ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobj::destruct()
   {
      return SDB_OK ;
   }

}

