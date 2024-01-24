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

   Source File Name = IStorageService.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_STORAGE_SERVICE_HPP_
#define SDB_I_STORAGE_SERVICE_HPP_

#include "interface/IPersistUnit.hpp"
#include "sdbInterface.hpp"
#include "interface/IStorageEngine.hpp"
#include "interface/IStorageBackupLogger.hpp"
#include "interface/ICollection.hpp"
#include "dmsMetadata.hpp"
#include "dmsOprtOptions.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      IStorageService define
    */
   class IStorageService : public SDBObject
   {
   public:
      IStorageService() = default ;
      virtual ~IStorageService() = default ;
      IStorageService( const IStorageService &o ) = delete ;
      IStorageService &operator =( const IStorageService & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const = 0 ;

      virtual INT32 openEngine( const dmsOpenEngineOptions &options ) = 0 ;
      virtual INT32 closeEngine( const dmsCloseEngineOptions &options ) = 0 ;
      virtual INT32 changeConfig() = 0 ;

      virtual IStorageEngine *getEngine() = 0 ;

      virtual INT32 fsync( BOOLEAN isForce, BOOLEAN isSync, IExecutor *executor ) = 0 ;
      virtual INT32 backup( IStorageBackupLogger &backupLogger ) = 0 ;
      virtual INT32 getPersistUnit( IExecutor *executor, IPersistUnit *&persistUnit ) = 0 ;

      virtual INT32 createCS( const dmsCSMetadata &metadata,
                              const dmsCreateCSOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 dropCS( const dmsCSMetadata &metadata,
                            const dmsDropCSOptions &options,
                            IExecutor *executor ) = 0 ;

      virtual INT32 createCL( const dmsCLMetadata &metadata,
                              const dmsCreateCLOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 dropCL( const dmsCLMetadata &metadata,
                            const dmsDropCLOptions &options,
                            IContext *context,
                            IExecutor *executor ) = 0 ;

      virtual INT32 getCollection( const dmsCLMetadataKey &metadataKey,
                                   IExecutor *executor,
                                   std::shared_ptr< ICollection > &collPtr ) = 0 ;
      virtual INT32 loadCollection( const dmsCLMetadata &metadata,
                                    IExecutor *executor,
                                    std::shared_ptr< ICollection > &collPtr ) = 0 ;

      virtual BOOLEAN isAlterCompressorSupported() const = 0 ;
   } ;

}

#endif // SDB_I_STORAGE_SERVICE_HPP_