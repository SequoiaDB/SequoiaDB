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

#include "pd.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "../include/ossUtil.h"
#include "../bson/bson.h"
#include "../util/fromjson.hpp"


using namespace engine;

#define RECEIVE_BUFFER_SIZE 4095
#define COLLECTION_SPACE_MAX_SZ 127
#define COLLECTION_MAX_SZ 127
char receiveBuffer [ RECEIVE_BUFFER_SIZE+1 ] ;
char *pOutBuffer = NULL ;
int outBufferSize = 0 ;

INT32 readInput ( CHAR *pPrompt, INT32 numIndent );

int main(int argc,char *argv[])
{
   INT32 rc = SDB_OK ;
   if( argc > 2 || argc < 2 )
   {
      std::cout << "Input false !\n" ; 
      return 0;
   }
   BSONObj matcher ;
   CHAR BUFF  [ RECEIVE_BUFFER_SIZE ] ;
   rc = fromjson ( argv[1], matcher ) ;
   std::cout << matcher <<endl ; 

   ossHexDumpBuffer( matcher.objdata(), matcher.objsize(), BUFF, sizeof(BUFF), NULL, OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
   std::cout << BUFF <<endl ; 
   return rc ;
}

char *readLine ( char* p, int length )
{
   fgets ( p, length, stdin ) ;
   p[strlen(p)-1] = 0 ;
   return p ;
}

INT32 readInput ( CHAR *pPrompt, INT32 numIndent )
{
   memset ( receiveBuffer, 0, sizeof(receiveBuffer) ) ;
   for ( INT32 i = 0; i<numIndent; i++ )
   {
      printf("\t") ;
   }
   printf("%s> ", pPrompt ) ;
   readLine ( receiveBuffer, sizeof(receiveBuffer) ) ;
   while ( receiveBuffer[strlen(receiveBuffer)-1] == '\\' &&
           RECEIVE_BUFFER_SIZE - strlen(receiveBuffer) > 0 )
   {
      for ( INT32 i = 0; i<numIndent; i++ )
      {
         printf("\t") ;
      }
      printf ( "> " ) ;
      readLine ( &receiveBuffer[strlen(receiveBuffer)-1],
                 RECEIVE_BUFFER_SIZE - strlen(receiveBuffer) ) ;
   }
   if ( RECEIVE_BUFFER_SIZE == strlen(receiveBuffer) )
   {
      printf ( "Error: Max input length is %d bytes\n", RECEIVE_BUFFER_SIZE ) ;
      return SDB_INVALIDARG ;
   }
   return SDB_OK ;
}
