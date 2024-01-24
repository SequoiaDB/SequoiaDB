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

   Source File Name = dmsStorageServiceFactory.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_STORAGE_SERVICE_FACTORY_HPP_
#define SDB_DMS_STORAGE_SERVICE_FACTORY_HPP_

#include "dmsDef.hpp"
#include "interface/IStorageService.hpp"

namespace engine
{

   /*
      _dmsStorageServiceFactory define
    */
   class _dmsStorageServiceFactory : public SDBObject
   {
   public:
      _dmsStorageServiceFactory() = default ;
      virtual ~_dmsStorageServiceFactory() = default ;
      _dmsStorageServiceFactory( const _dmsStorageServiceFactory &o ) = delete ;
      _dmsStorageServiceFactory &operator =( const _dmsStorageServiceFactory & ) = delete ;

   public:
      static INT32 create( DMS_STORAGE_ENGINE_TYPE engineType,
                           IStorageService *&service ) ;
      static void release( IStorageService *service ) ;
   } ;

   typedef class _dmsStorageServiceFactory dmsStorageServiceFactory ;

}

#endif // SDB_DMS_STORAGE_SERVICE_FACTORY_HPP_