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

   Source File Name = dmsWTStorageEngine.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_STORAGE_ENGINE_HPP_
#define DMS_WT_STORAGE_ENGINE_HPP_

#include "interface/IStorageEngine.hpp"
#include "ossTypes.h"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTItem.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTStore.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTHandler.hpp"

#include <boost/filesystem/path.hpp>
#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   // forward declaration
   class _dmsWTStorageService ;

   /*
      _dmsWTStorageEngine define
    */
   class _dmsWTStorageEngine : public IStorageEngine
   {
   public:
      _dmsWTStorageEngine( _dmsWTStorageService &service,
                           dmsWTEngineOptions &options ) ;
      virtual ~_dmsWTStorageEngine() ;
      _dmsWTStorageEngine( const _dmsWTStorageEngine &o ) = delete ;
      _dmsWTStorageEngine &operator =( const _dmsWTStorageEngine & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const
      {
         return DMS_STORAGE_ENGINE_WIREDTIGER ;
      }

      _dmsWTStorageService &getService()
      {
         return _service ;
      }

      const dmsWTEngineOptions &getOptions() const
      {
         return _options ;
      }

      BOOLEAN isClosed() const
      {
         return nullptr == _conn ;
      }

      BOOLEAN isOpened() const
      {
         return nullptr != _conn ;
      }

      INT32 open( const boost::filesystem::path &dbPath,
                  const CHAR *config ) ;
      INT32 close( const CHAR *config ) ;
      INT32 reconfig( const CHAR *config ) ;

      INT32 openSession( dmsWTSession &session,
                         dmsWTSessIsolation isolation = dmsWTSessIsolation::SNAPSHOT ) ;

      INT32 createStore( const CHAR *uri,
                         const CHAR *config,
                         dmsWTStore &store ) ;

      INT32 dropStore( const CHAR *uri,
                       const CHAR *config,
                       IContext *context = nullptr ) ;
      INT32 dropStores( const ossPoolList<ossPoolString> &uris,
                        const CHAR *config ) ;

      INT32 truncateStore( dmsWTSession &session,
                           const CHAR *uri,
                           const CHAR *config ) ;

      INT32 compactStore( dmsWTSession &session,
                          const CHAR *uri,
                          const CHAR *config ) ;

      INT32 loadStore( const CHAR *uri,
                       dmsWTStore &store ) ;
      INT32 openStoreCursor( const CHAR *uri,
                             const CHAR *config,
                             dmsWTCursor &cursor ) ;

      INT32 dumpURIListByPrefix( const CHAR *prefix,
                                 ossPoolList< ossPoolString > &uriList ) ;
      INT32 dumpURIList( ossPoolList< ossPoolString > &uriList ) ;

      INT32 getPersistSession( IExecutor *executor,
                               dmsWTSessionHolder &sessionHolder ) ;

      INT32 getReadSession( IExecutor *executor,
                            dmsWTSessionHolder &sessionHolder ) ;

      INT32 checkPoint( IExecutor *executor ) ;

   protected:
      INT32 _checkDBPath( const boost::filesystem::path &dbPath,
                          boost::filesystem::path &enginePath ) ;

   protected:
      _dmsWTStorageService &_service ;
      dmsWTEngineOptions &_options ;
      dmsWTHandler _handler ;
      WT_CONNECTION *_conn = nullptr ;
   } ;

   typedef class _dmsWTStorageEngine dmsWTStorageEngine ;

}
}

#endif // DMS_WT_STORAGE_ENGINE_HPP_
