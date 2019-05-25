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

#include "core.hpp"
#include "ossNPipe.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#define INPUT_BUFFER_SIZE 1024
#define OUTPUT_BUFFER_SIZE 1024
#define EXIT_CODE "exit"
int main ( int argc, char **argv )
{
   INT32 rc = SDB_OK ;
   OSSNPIPE handle ;
   if ( argc != 2 )
   {
      printf ( "Syntax: %s <pipe name>"OSS_NEWLINE,
               argv[0] ) ;
      return 0 ;
   }
   CHAR *pPipeName = argv[1] ;
   ossPrintf ( "Create named pipe: %s"OSS_NEWLINE, pPipeName ) ;
   rc = ossCreateNamedPipe ( pPipeName, INPUT_BUFFER_SIZE, OUTPUT_BUFFER_SIZE,
                             OSS_NPIPE_DUPLEX | OSS_NPIPE_BLOCK,
                             OSS_NPIPE_UNLIMITED_INSTANCES,
                             OSS_NPIPE_INFINITE_TIMEOUT,
                             handle ) ;
   if ( rc && SDB_FE != rc )
   {
      PD_LOG ( PDERROR, "Failed to create named pipe: %s, rc %d"OSS_NEWLINE,
               pPipeName, rc ) ;
      goto create_error ;
   }
   ossPrintf ( "Connect to named pipe: %s"OSS_NEWLINE, pPipeName ) ;
   rc = ossConnectNamedPipe ( handle, OSS_NPIPE_DUPLEX | OSS_NPIPE_BLOCK,
                              OSS_NPIPE_INFINITE_TIMEOUT ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to connect to named pipe: %s"OSS_NEWLINE,
               pPipeName ) ;
      goto connect_error ;
   }

   ossPrintf ( "Read from named pipe: %s"OSS_NEWLINE, pPipeName ) ;
   while ( TRUE )
   {
      INT64 readSize = 0 ;
      CHAR buffer [ OUTPUT_BUFFER_SIZE + 1 ] ;
      rc = ossReadNamedPipe ( handle, buffer, OUTPUT_BUFFER_SIZE, &readSize,
                              OSS_NPIPE_INFINITE_TIMEOUT ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read packet size"OSS_NEWLINE ) ;
         goto error ;
      }
      if ( readSize > 0 )
      {
         ossPrintf ( "Received: %s"OSS_NEWLINE, buffer ) ;
         if ( ossStrncmp ( buffer, EXIT_CODE, sizeof(EXIT_CODE) ) == 0 )
         {
            break ;
         }
      }
   }
error :
   rc = ossDisconnectNamedPipe ( handle ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to disconnect named pipe"OSS_NEWLINE ) ;
      goto create_error ;
   }
connect_error :
   rc = ossDeleteNamedPipe ( handle ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to delete named pipe"OSS_NEWLINE ) ;
      goto create_error ;
   }
create_error :
   return rc ;
}
