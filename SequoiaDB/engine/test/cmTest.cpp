/*******************************************************************************

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

*******************************************************************************/

#include "core.h"
#include "pd.hpp"
#include "ossSocket.hpp"
#include "omagentDef.hpp"
#include "rtnRemoteExec.hpp"
#include "omagentDef.hpp"
#include "../bson/bson.h"

using namespace bson ;
using namespace engine ;


INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 remoCode = 0 ;
   INT32 retCode = 1 ;
   CHAR hostname[OSS_MAX_HOSTNAME] ;
   BSONObj obj1, obj2 ;
   BSONObjBuilder ob1, ob2 ;
   if ( argc > 1 )
   {
      switch ( argv[1][0] )
      {
         case 'a' :
            remoCode = SDBSTART ;
            break ;
         case 'b' :
            remoCode = SDBSTOP ;
            break ;
         case 'c' :
            remoCode = SDBADD ;
            break ;
         case 'd' :
            remoCode = SDBMODIFY ;
            break ;
         default:
            remoCode = 0 ;
      }
   }
   try
   {
      if ( argc == 3 )
         ob1.append ( "svcname", argv[2] ) ;
      if ( argc == 4 )
      {
         ob1.append ( "svcname", argv[2] ) ;
         ob1.append ( "dbpath", argv[3] ) ;
      }
      if ( argc == 5 )
      {
         ob1.append ( "svcname", argv[2] ) ;
         ob2.append ( "svcname", argv[3] ) ;
         ob2.append ( "dbpath", argv[4] ) ;
      }
      obj1 = ob1.obj() ;
      obj2 = ob2.obj() ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR, "Failed to build BSONObj" ) ;
      exit ( 0 ) ;
   }
   if ( ossSocket::getHostName ( hostname, OSS_MAX_HOSTNAME ) )
   {
      PD_LOG ( PDERROR, "Failed to get hostname" ) ;
      goto error ;
   }
   engine::rtnRemoteExec ( remoCode, hostname, &retCode, &obj1, &obj2 ) ;
   PD_LOG ( PDERROR, "rc = %d", retCode ) ;
done:
   return 0 ;
error:
   goto done ;
}
