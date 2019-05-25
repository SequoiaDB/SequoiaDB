/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = json2rawbson.c

   Descriptive Name = JSON To Raw BSON

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component. This file contains declare of json2rawbson. Note
   this function should NEVER be directly called other than fromjson.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "json2rawbson.h"
#if defined (SDB_ENGINE) || defined (SDB_CLIENT)
#include "../client/jstobs.h"
#else
#include "jstobs.h"
#endif
CHAR * json2rawbson ( const CHAR *str, BOOLEAN isBatch )
{
   bson obj ;
   CHAR *p = NULL ;
   bson_init ( &obj ) ;
   if( json2bson( str, NULL, CJSON_RIGOROUS_PARSE, !isBatch, &obj ) )
   {
      if ( obj.data )
      {
         INT32 bsonsize = *((INT32*)obj.data) ;
         p = (CHAR*)malloc ( bsonsize + sizeof(unsigned)) ;
         if ( p )
         {
            memset ( p, 0, bsonsize + sizeof(unsigned) ) ;
            memcpy ( &p[sizeof(unsigned)], obj.data, bsonsize ) ;
         }
      }
   }
   bson_destroy ( &obj ) ;
   return p ;
}
CHAR * json2rawcbson ( const CHAR *str )
{
   bson obj ;
   CHAR *p = NULL ;
   bson_init ( &obj ) ;
   if( json2bson( str, NULL, CJSON_RIGOROUS_PARSE, TRUE, &obj ) )
   {
      if ( obj.data )
      {
         INT32 bsonsize = *((INT32*)obj.data) ;
         if ( 0 < bsonsize )
         {
            p = (CHAR*)malloc ( bsonsize ) ;
            if ( p )
            {
               memset ( p, 0, bsonsize ) ;
               memcpy ( p, obj.data, bsonsize ) ;
            }
         }
      }
   }
   bson_destroy ( &obj ) ;
   return p ;
}
