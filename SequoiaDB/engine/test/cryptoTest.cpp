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

#include "openssl/md5.h"
#include <stdio.h>
#include <string.h>
int main ( int argc, char **argv )
{
   MD5_CTX c ;
   unsigned char md[MD5_DIGEST_LENGTH] ;
   if ( argc !=2 )
   {
      printf ( "Syntax: %s <string>\n", (char*)argv[0]) ;
      return 0 ;
   }
   char *pString = (char*)argv[1] ;
   MD5_Init(&c) ;
   MD5_Update(&c, pString, strlen(pString)) ;
   MD5_Final(&(md[0]), &c) ;
   for ( int i=0; i<MD5_DIGEST_LENGTH; i++)
   {
      printf("%02x", md[i]) ;
   }
   printf("\n") ;
   return 0 ;
}
