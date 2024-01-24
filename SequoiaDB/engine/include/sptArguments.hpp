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

   Source File Name = sptArguments.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_ARGUMENTS_HPP_
#define SPT_ARGUMENTS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include "sptPrivateData.hpp"
#include "sptObjDesc.hpp"
#include "sptObject.hpp"

namespace engine
{
   enum SPT_NATIVE_TYPE
   {
      SPT_NATIVE_CHAR      = 1,
      SPT_NATIVE_INT16,
      SPT_NATIVE_INT32,
      SPT_NATIVE_INT64,
      SPT_NATIVE_FLOAT32,
      SPT_NATIVE_FLOAT64
   } ;

   /*
      _sptArguments define
   */
   class _sptArguments : public SDBObject
   {
   public:
      _sptArguments() {}
      virtual ~_sptArguments(){}

      virtual const sptObject*   getObject() const = 0 ;

   public:
      /// start with zero.
      virtual INT32 getNative( UINT32 pos, void *value,
                               SPT_NATIVE_TYPE type ) const = 0 ;
      virtual INT32 getString( UINT32 pos, std::string &value,
                               BOOLEAN strict = TRUE ) const = 0 ;
      virtual INT32 getBsonobj( UINT32 pos, bson::BSONObj &value,
                                BOOLEAN strict = TRUE,
                                BOOLEAN allowNull = FALSE )
                                const = 0 ;
      virtual INT32 getArray( UINT32 pos, vector< bson::BSONObj >&value,
                              BOOLEAN strict = TRUE ) const = 0 ;
      virtual INT32 getArray( UINT32 pos, vector< string > &value,
                              BOOLEAN strict = TRUE ) const = 0 ;

      virtual INT32 getUserObj( UINT32 pos, const _sptObjDesc &objDesc,
                                const void** value ) const = 0 ;
      virtual INT32 getBoolean( UINT32 pos, BOOLEAN &value,
                                BOOLEAN strict = TRUE ) const = 0 ;
      virtual sptPrivateData* getPrivateData() const = 0 ;

      virtual UINT32 argc() const = 0 ;

      virtual BOOLEAN isString( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isInt( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isLong( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isBoolean( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isDouble( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isNumber( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isObject( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isNull( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isVoid( UINT32 pos ) const = 0 ;
      virtual BOOLEAN isUserObj( UINT32 pos,
                                 const _sptObjDesc &objDesc ) const = 0 ;
      virtual BOOLEAN isArray( UINT32 pos ) const = 0 ;
      virtual string  getUserObjClassName( UINT32 pos ) const = 0 ;

      virtual string  getErrMsg() const = 0 ;
      virtual BOOLEAN hasErrMsg() const = 0 ;
   } ;
   typedef class _sptArguments sptArguments ;
}

#endif

