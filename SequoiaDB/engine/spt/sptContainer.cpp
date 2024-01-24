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

   Source File Name = sptContainer.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptContainer.hpp"
#include "sptSPScope.hpp"
#include "spt.hpp"
#include "pd.hpp"
#include "sptUsrSsh.hpp"
#include "sptUsrCmd.hpp"
#include "sptUsrFile.hpp"
#include "sptUsrSystem.hpp"
#include "sptUsrOma.hpp"
#include "sptUsrHash.hpp"
#include "sptUsrSdbTool.hpp"
#include "sptUsrRemote.hpp"
#include "sptUsrFilter.hpp"
#include "../client/common.h"
#include "client.hpp"

namespace engine
{
   #define SPT_SCOPE_CACHE_SIZE                 0 //(30)

   _sptContainer::_sptContainer()
   {
   }

   _sptContainer::~_sptContainer()
   {
      fini() ;
   }

   INT32 _sptContainer::init()
   {
      sptGetObjFactory()->sortAndAssert() ;
      initCacheStrategy( FALSE, 0 ) ;
      sdbclient::sdbSetErrorOnReplyCallback( (sdbclient::ERROR_ON_REPLY_FUNC)engine::sdbErrorCallback ) ;
      return SDB_OK ;
   }

   INT32 _sptContainer::fini()
   {
      ossScopedLock lock( &_latch ) ;

      for ( UINT32 i = 0 ; i < _vecScopes.size() ; ++i )
      {
         _vecScopes[ i ]->shutdown() ;
         SDB_OSS_DEL _vecScopes[ i ] ;
      }
      _vecScopes.clear() ;
      return SDB_OK ;
   }

   _sptScope* _sptContainer::_getFromCache( SPT_SCOPE_TYPE type,
                                            UINT32 loadMask )
   {
      _sptScope *pScope = NULL ;
      VEC_SCOPE_IT it ;
      ossScopedLock lock( &_latch ) ;

      it = _vecScopes.begin() ;
      while ( it != _vecScopes.end() )
      {
         pScope = *it ;
         if ( type == pScope->getType() &&
              loadMask == pScope->getLoadMask() )
         {
            _vecScopes.erase( it ) ;
            break ;
         }
         ++it ;
      }
      return pScope ;
   }

   _sptScope* _sptContainer::_createScope( SPT_SCOPE_TYPE type,
                                           UINT32 loadMask )
   {
      INT32 rc = SDB_OK ;
      _sptScope *scope = NULL ;

      if ( SPT_SCOPE_TYPE_SP == type )
      {
         scope = SDB_OSS_NEW _sptSPScope() ;
      }

      if ( NULL == scope )
      {
         ossPrintf( "it is a unknown type of scope:%d" OSS_NEWLINE, type ) ;
         goto error ;
      }

      rc = scope->start( loadMask ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to init scope, rc: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return scope ;
   error:
      SAFE_OSS_DELETE( scope ) ;
      goto done ;
   }

   _sptScope *_sptContainer::newScope( SPT_SCOPE_TYPE type,
                                       UINT32 loadMask )
   {
      _sptScope *scope = _getFromCache( type, loadMask ) ;
      if ( scope )
      {
         goto done ;
      }

      scope = _createScope( type, loadMask ) ;
      if ( !scope )
      {
         ossPrintf( "Failed to create scope[%d]" OSS_NEWLINE, type ) ;
         goto error ;
      }
   done:
      return scope ;
   error:
      goto done ;
   }

   void _sptContainer::releaseScope( _sptScope * pScope )
   {
      if ( !pScope )
      {
         goto done ;
      }
      _latch.get() ;
      if ( _vecScopes.size() < SPT_SCOPE_CACHE_SIZE )
      {
         pScope->clearJSFileNameList() ;
         _vecScopes.push_back( pScope ) ;
         pScope = NULL ;
      }
      _latch.release() ;

      if ( pScope )
      {
         pScope->shutdown() ;
         SDB_OSS_DEL pScope ;
         pScope = NULL ;
      }

   done:
      return ;
   }

}

