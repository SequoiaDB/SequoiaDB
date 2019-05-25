/******************************************************************************


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

   Source File Name = sptSPArguments.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_SPARGUMENTS_HPP_
#define SPT_SPARGUMENTS_HPP_

#include "sptArguments.hpp"
#include "jsapi.h"

namespace engine
{
   class _sptSPArguments : public _sptArguments
   {
   public:
      _sptSPArguments( JSContext *context, uintN argc, jsval *vp ) ;
      virtual ~_sptSPArguments() ;

   public:
      virtual INT32 getNative( UINT32 pos, void *value,
                               SPT_NATIVE_TYPE type ) const ;
      virtual INT32 getString( UINT32 pos, std::string &value,
                               BOOLEAN strict = TRUE ) const ;
      virtual INT32 getBsonobj( UINT32 pos, bson::BSONObj &value ) const ;

      virtual INT32 getUserObj( UINT32 pos, const _sptObjDesc &objDesc,
                                const void** value ) const ;

      virtual sptPrivateData* getPrivateData() const ;

      virtual UINT32 argc() const
      {
         return _argc ;
      }

      virtual BOOLEAN isString( UINT32 pos ) const ;
      virtual BOOLEAN isInt( UINT32 pos ) const ;
      virtual BOOLEAN isBoolean( UINT32 pos ) const ;
      virtual BOOLEAN isDouble( UINT32 pos ) const ;
      virtual BOOLEAN isNumber( UINT32 pos ) const ;
      virtual BOOLEAN isObject( UINT32 pos ) const ;
      virtual BOOLEAN isNull( UINT32 pos ) const ;
      virtual BOOLEAN isVoid( UINT32 pos ) const ;
      virtual BOOLEAN isUserObj( UINT32 pos,
                                 const _sptObjDesc &objDesc ) const ;
      virtual string getUserObjClassName( UINT32 pos ) const ;
   private:
      jsval *_getValAtPos( UINT32 pos ) const ;

   private:
      JSContext *_context ;
      uintN _argc ;
      jsval *_vp ;
   } ;
}

#endif

