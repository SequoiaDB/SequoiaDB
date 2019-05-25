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

   Source File Name = mthSAttribute.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SATTRIBUTE_HPP_
#define MTH_SATTRIBUTE_HPP_

#include "mthDef.hpp"
#include "pd.hpp"

namespace engine
{
   class _mthSAttribute : public SDBObject
   {
   public:
      _mthSAttribute()
      :_attribute( MTH_S_ATTR_NONE ),
       _4check( MTH_S_ATTR_NONE )
      {

      }
      ~_mthSAttribute(){} 

   public:
      OSS_INLINE INT32 set( MTH_S_ATTRIBUTE attribute )
      {
         INT32 rc = SDB_OK ;
         if ( !MTH_ATTR_IS_VALID( attribute ) )
         {
            PD_LOG( PDERROR, "invalid attribute:%d", attribute ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( MTH_S_ATTR_EXCLUDE == attribute ||
                   MTH_S_ATTR_INCLUDE == attribute )
         {
            if ( MTH_ATTR_IS_VALID( _4check ) )
            {
               if ( attribute != _4check )
               {
                  PD_LOG( PDERROR, "can not have a mix of inclusion and exclusion" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }
            else
            {
               _4check = attribute ;
            }
         }
         else
         {
         }

         _attribute |= attribute ;
      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE BOOLEAN isInclude() const
      {
         return MTH_ATTR_IS_INCLUDE( _attribute ) ;
      }

      OSS_INLINE BOOLEAN isDefault() const
      {
         return MTH_ATTR_IS_DEFAULT( _attribute) ;
      }

      OSS_INLINE BOOLEAN contains( MTH_S_ATTRIBUTE attribute ) const
      {
         return ( _attribute & attribute ) == attribute ;
      }

      OSS_INLINE BOOLEAN isValid() const
      {
         return MTH_ATTR_IS_VALID( _attribute ) ;
      }

      OSS_INLINE void clear()
      {
         _attribute = MTH_S_ATTR_NONE ;
         _4check = MTH_S_ATTR_NONE ;
         return ;
      }
   private:
      MTH_S_ATTRIBUTE _attribute ;
      MTH_S_ATTRIBUTE _4check ;
   } ;
   typedef class _mthSAttribute mthSAttribute ;
}

#endif

