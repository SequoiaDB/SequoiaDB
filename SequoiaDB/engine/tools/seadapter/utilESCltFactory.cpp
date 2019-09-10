/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilESCltFactory.cpp

   Descriptive Name = Elasticsearch client manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pd.hpp"
#include "utilESClt.hpp"
#include "utilESCltFactory.hpp"

namespace seadapter
{
   _utilESCltFactory::_utilESCltFactory()
   {
      _timeout = 0 ;
   }

   _utilESCltFactory::~_utilESCltFactory()
   {
   }

   INT32 _utilESCltFactory::init( const std::string &url, INT32 timeout )
   {
      INT32 rc = SDB_OK ;

      if ( url.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Url to init search engine client manager is empty" ) ;
         goto error ;
      }
      _url = url ;
      _timeout = timeout ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESCltFactory::create( utilESClt **seClt )
   {
      INT32 rc = SDB_OK ;
      utilESClt* client = NULL ;

      SDB_ASSERT( seClt, "Parameter should not be null" ) ;

      client = SDB_OSS_NEW _utilESClt() ;
      if ( !client )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate memory for search engine client, "
                 "rc: %d", rc ) ;
         goto error ;
      }
      rc = client->init( _url, FALSE, _timeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init search engine client, rc: %d",
                 rc ) ;
         goto error ;
      }

      *seClt = client ;

   done:
      return rc ;
   error:
      if ( client )
      {
         SDB_OSS_DEL client ;
      }
      goto done ;
   }
}

