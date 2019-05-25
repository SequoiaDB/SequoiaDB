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

   Source File Name = rawbson2json.c

   Descriptive Name = Raw BSON to JSON ( or CSV )

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rawbson2json.h"
#include "../client/jstobs.h"

BOOLEAN rawbson2json ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen )
{
   bson obj ;
   bson_init ( &obj ) ;
   bson_init_finished_data ( &obj, (char*)bsonObj ) ;
   return bsonToJson ( pOutputBuffer, bufferLen, &obj,
                       FALSE, FALSE ) ;
}

BOOLEAN rawbson2csv ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen )
{
   bson obj ;
   bson_init ( &obj ) ;
   bson_init_finished_data ( &obj, (char*)bsonObj ) ;
   return bsonToJson ( pOutputBuffer, bufferLen, &obj,
                       TRUE, FALSE ) ;
}
