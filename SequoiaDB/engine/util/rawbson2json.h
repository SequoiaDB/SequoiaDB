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

   Source File Name = rawbson2json.h

   Descriptive Name = Raw BSON To JSON ( or csv ) Header

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/21/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RAWBSON2JSON_H__
#define RAWBSON2JSON_H__

#include "core.h"
SDB_EXTERN_C_START
BOOLEAN rawbson2json ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen ) ;
BOOLEAN rawbson2csv ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen ) ;
SDB_EXTERN_C_END
#endif
