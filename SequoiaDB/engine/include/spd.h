/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

/*
   _SDB_SPD_RES_TYPE define
*/
enum _SDB_SPD_RES_TYPE
{
   SDB_SPD_RES_TYPE_VOID = 0, /**< Js return void type */
   SDB_SPD_RES_TYPE_STR,      /**< Js return a string */
   SDB_SPD_RES_TYPE_NUMBER,   /**< Js return a number */
   SDB_SPD_RES_TYPE_OBJ,      /**< Js return an object */
   SDB_SPD_RES_TYPE_BOOL,     /**< Js return a bool */
   SDB_SPD_RES_TYPE_RECORDSET,/**< Js return a cursor handle */
   SDB_SPD_RES_TYPE_SPECIALOBJ = 10, /**< Js return a special obj */
   SDB_SPD_RES_TYPE_NULL,     /**< Js return null type */

   SDB_SPD_RES_TYPE_MAX
} ;

/** Js return type */
typedef enum _SDB_SPD_RES_TYPE SDB_SPD_RES_TYPE ;

#endif //SPD_H_

