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

#define RECEIVE_BUFFER_SIZE 4095
CHAR receiveBuffer [ RECEIVE_BUFFER_SIZE + 1 ] ;

CHAR *readLine ( CHAR *p, INT32 length )
{
   CHAR *ret = NULL ;
   ret = fgets ( p, length, stdin ) ;
   if ( !ret )
      return NULL ;
   p [ ossStrlen ( p ) - 1 ] = 0 ;
   return p ;
}

INT32 readInput ( CHAR *pPrompt, INT32 numIndent )
{
   ossMemset ( receiveBuffer, 0, sizeof(receiveBuffer) ) ;
   for ( INT32 i = 0; i < numIndent; ++i )
   {
      ossPrintf ( "\t" ) ;
   }
   ossPrintf ( "%s> ", pPrompt ) ;
   readLine ( receiveBuffer, RECEIVE_BUFFER_SIZE ) ;
   while ( receiveBuffer [ ossStrlen ( receiveBuffer ) - 1 ] == '\\' &&
           RECEIVE_BUFFER_SIZE - ossStrlen ( receiveBuffer ) > 0 )
   {
      for ( INT32 i = 0; i < numIndent; ++i )
      {
         ossPrintf ( "\t" ) ;
      }
      ossPrintf ( "> " ) ;
      readLine ( &receiveBuffer [ strlen ( receiveBuffer ) - 1 ],
                 RECEIVE_BUFFER_SIZE - strlen ( receiveBuffer ) ) ;
   }
   if ( RECEIVE_BUFFER_SIZE == ossStrlen ( receiveBuffer ) )
   {
      ossPrintf ( "Error: Max input length is %d bytes\n", RECEIVE_BUFFER_SIZE);
      return SDB_INVALIDARG ;
   }
   return SDB_OK ;
}
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
   ossPrintf ( "Open named pipe: %s"OSS_NEWLINE, pPipeName ) ;
   rc = ossOpenNamedPipe ( pPipeName, OSS_NPIPE_DUPLEX | OSS_NPIPE_BLOCK,
                           OSS_NPIPE_INFINITE_TIMEOUT,
                           handle ) ;
   if ( rc && SDB_FE != rc )
   {
      PD_LOG ( PDERROR, "Failed to create named pipe: %s, rc %d"OSS_NEWLINE,
               pPipeName, rc ) ;
      goto open_error ;
   }

   ossPrintf ( "Write to named pipe: %s"OSS_NEWLINE, pPipeName ) ;
   while ( TRUE )
   {
      rc = readInput ( "Input", 1 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read from command line" ) ;
         goto error ;
      }
      rc = ossWriteNamedPipe ( handle, receiveBuffer,
                               ossStrlen ( receiveBuffer ) + 1,
                               NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read packet size"OSS_NEWLINE ) ;
         goto error ;
      }
      if ( ossStrncmp ( receiveBuffer, EXIT_CODE, sizeof(EXIT_CODE) ) == 0 )
         break ;
   }
error :
   rc = ossCloseNamedPipe ( handle ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to close named pipe"OSS_NEWLINE ) ;
      goto open_error ;
   }
open_error :
   return rc ;
}
