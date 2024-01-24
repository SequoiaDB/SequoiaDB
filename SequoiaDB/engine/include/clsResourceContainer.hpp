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

   Source File Name = clsResourceContainer.hpp

   Descriptive Name = Resource Container

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          3/10/2020   LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_RESOURCE_CONTAINER_HPP_
#define CLS_RESOURCE_CONTAINER_HPP_

#include "oss.hpp"
#include "ossTypes.hpp"

namespace engine
{
   class _coordResource ;

   class _clsResourceContainer : public SDBObject
   {
   public:
      _clsResourceContainer() : _pResource( NULL )
      {
      }

      ~_clsResourceContainer()
      {
         _pResource = NULL ;
      }

   public:
      void setResource( _coordResource *pResource )
      {
         _pResource = pResource ;
      }

      _coordResource* getResource()
      {
         return _pResource ;
      }

   private:
      _coordResource *_pResource ;
   };

   typedef _clsResourceContainer clsResourceContainer ;

   /*
      get global pointer
   */
   _clsResourceContainer* sdbGetResourceContainer() ;
}

#endif //CLS_RESOURCE_CONTAINER_HPP_


