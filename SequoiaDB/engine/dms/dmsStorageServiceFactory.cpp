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

   Source File Name = dmsStorageServiceFactory.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageServiceFactory.hpp"
#include "dmsDef.hpp"
#include "ossErr.h"
#include "ossMem.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
       _dmsStorageServiceFactory implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGESERVICEFACTORY_CREATE, "_dmsStorageServiceFactory::create" )
   INT32 _dmsStorageServiceFactory::create( DMS_STORAGE_ENGINE_TYPE engineType,
                                            IStorageService *&service )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGESERVICEFACTORY_CREATE ) ;

      SDB_ASSERT( nullptr == service, "service should not be created" ) ;
      PD_CHECK( nullptr == service, SDB_INVALIDARG, error, PDERROR,
                "Failed to create service, already created" ) ;

      switch ( engineType )
      {
         case DMS_STORAGE_ENGINE_WIREDTIGER:
         {
            service = SDB_OSS_NEW wiredtiger::dmsWTStorageService() ;
            PD_CHECK( nullptr != service, SDB_OOM, error, PDERROR,
                      "Failed to create storage service for engine [%s], "
                      "out of memory",
                      dmsGetStorageEngineName(engineType) ) ;
            break ;
         }
         default:
         {
            PD_CHECK( FALSE, SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                      "Failed to create storage service for engine [%s], "
                      "it is not supported",
                      dmsGetStorageEngineName(engineType) ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGESERVICEFACTORY_CREATE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGESERVICEFACTORY_RELEASE, "_dmsStorageServiceFactory::release" )
   void _dmsStorageServiceFactory::release( IStorageService *engine )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGESERVICEFACTORY_RELEASE ) ;

      if ( nullptr != engine )
      {
         SDB_OSS_DEL engine ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGESERVICEFACTORY_RELEASE ) ;
   }

}
