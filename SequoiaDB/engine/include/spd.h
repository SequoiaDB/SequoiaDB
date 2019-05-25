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

   Source File Name = spd.h

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
/** \file spd.h
    \brief Js return type for stored procedures used in sdb shell
*/

#ifndef SPD_H_
#define SPD_H_

#include "core.h"

SDB_EXTERN_C_START

enum _SDB_SPD_RES_TYPE
{
   SDB_SPD_RES_TYPE_VOID = 0, /**< Js return void type */
   SDB_SPD_RES_TYPE_STR,      /**< Js return a string */
   SDB_SPD_RES_TYPE_NUMBER,   /**< Js return a number */
   SDB_SPD_RES_TYPE_OBJ,      /**< Js return an object */
   SDB_SPD_RES_TYPE_BOOL,     /**< Js return a bool */
   SDB_SPD_RES_TYPE_RECORDSET,/**< Js return a cursor handle */
   SDB_SPD_RES_TYPE_CS,       /**< Js return a collection space handle */
   SDB_SPD_RES_TYPE_CL,       /**< Js return a collection handle */
   SDB_SPD_RES_TYPE_RG,       /**< Js return a replica group handle */
   SDB_SPD_RES_TYPE_RN,       /**< Js return a data node handle */
} ;

/** Js return type */
typedef enum _SDB_SPD_RES_TYPE SDB_SPD_RES_TYPE ;

SDB_EXTERN_C_END

#endif

