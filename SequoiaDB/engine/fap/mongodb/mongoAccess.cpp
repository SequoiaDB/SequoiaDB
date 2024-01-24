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

   Source File Name = mongoAccess.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoAccess.hpp"
#include "ossUtil.hpp"
#include "pmdOptions.hpp"
#include "mongoSession.hpp"

namespace engine {
   PMD_EXPORT_ACCESSPROTOCOL_DLL( mongoAccess )
}

INT32 _mongoAccess::init( engine::IResource *pResource )
{
   UINT16 basePort = 0 ;
   if ( NULL != pResource )
   {
      _resource = pResource ;
   }

   ossMemset( (void *)_serviceName, 0, OSS_MAX_SERVICENAME + 1 ) ;
   if ( NULL != _resource )
   {
      basePort = _resource->getLocalPort() ;
      ossItoa( basePort + PORT_OFFSET, _serviceName, OSS_MAX_SERVICENAME ) ;
   }

   return SDB_OK ;
}

INT32 _mongoAccess::active()
{
   return SDB_OK ;
}

INT32 _mongoAccess::deactive()
{
   return SDB_OK ;
}

INT32 _mongoAccess::fini()
{
   return SDB_OK ;
}

const CHAR * _mongoAccess::getServiceName() const
{
   SDB_ASSERT( '\0' != _serviceName[0], "service name should not be empty" ) ;
   return _serviceName ;
}

engine::pmdSession * _mongoAccess::getSession( SOCKET fd )
{
   mongoSession *session = NULL ;
   session = SDB_OSS_NEW mongoSession( fd, _resource ) ;
   return session ;
}

void _mongoAccess::releaseSession( engine::pmdSession *pSession )
{
   mongoSession *session = dynamic_cast< mongoSession *>( pSession ) ;
   if ( NULL != session )
   {
      SDB_OSS_DEL session ;
      session = NULL ;
   }
}

void _mongoAccess::_release()
{
   ossMemset( _serviceName, 0, OSS_MAX_SERVICENAME + 1 ) ;

   if ( NULL != _resource )
   {
      // should not delete here, just make it point to nullptr
      _resource = NULL ;
   }
}
