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

#include "ossSocket.hpp"
#include "msgReplicator.hpp"
#include "../bson/bson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <boost/thread.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace bson ;
using namespace std ;
int  MAXLOOP = 100000 ;
bool dumpHex = false ;
int main ( int argc, char** argv )
{
   char *pHost ;
   int port ;
   int sentLen = 0 ;
   if ( argc != 3 && argc != 4 )
   {
      printf ( "Syntax: host port [-dump]\n") ;
      return 0 ;
   }
   pHost = (char*)argv[1] ;
   port = atoi((char*)argv[2]);
   if ( argc == 4 && strcmp ( (char*)argv[3], "-dump" ) == 0)
      dumpHex = true ;
   {
      int rc = 0 ;
      char *pBuffer = NULL ;
      char *pOutBuffer = NULL ;
      vector<BSONObj> resultList ;
      vector<BSONObj>::iterator it ;
      BSONObj dummyObj ;
      boost::posix_time::ptime t1 ;
      boost::posix_time::ptime t2 ;
      boost::posix_time::time_duration diff ;
      printf ( "Connect to host %s port %d\n", pHost, port ) ;
      ossSocket sock ( pHost, port ) ;
      rc = sock.initSocket () ;
      if ( rc )
      {
         printf ( "Failed to init Socket\n" ) ;
         goto client_done ;
      }
      rc = sock.connect () ;
      if ( rc )
      {
         printf ( "Failed to connect Socket \n" ) ;
         goto client_done ;
      }
      sock.disableNagle();
      printf ("Successfully connected to server\n" ) ;
      pBuffer = (char*)malloc(sizeof(char)*100) ;
      t1 =
           boost::posix_time::microsec_clock::local_time() ;
      {
         MsgReplHeader msgHeader ;
         msgHeader.messageLength = sizeof(msgHeader) ;
         rc = sock.send ( (CHAR*)(&msgHeader), sizeof(msgHeader), sentLen ) ;
         if ( rc )
         {
            printf ("wrong\n" ) ;
         }
      }
      
      t2 =
           boost::posix_time::microsec_clock::local_time() ;
      printf ("Disconnected from server\n") ;
client_done :
      if (pBuffer)
         free (pBuffer);
      if (pOutBuffer)
         free (pOutBuffer);
      sock.close () ;
   }
   return 0 ;
}
