/* Copyright (c) 2003, 2011, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifndef SDB_ERR_CODE__H
#define SDB_ERR_CODE__H

#define IS_SDB_NET_ERR(rc) ( SDB_NETWORK_CLOSE == rc || SDB_NETWORK == rc || SDB_NOT_CONNECTED == rc || SDB_SYS == rc )

enum SDB_ERR_CODE
{
   SDB_ERR_OK = 0,
   SDB_ERR_COND_UNKOWN_ITEM            = 30000,
   SDB_ERR_COND_PART_UNSUPPORTED,
   SDB_ERR_COND_UNSUPPORTED,
   SDB_ERR_COND_INCOMPLETED,
   SDB_ERR_COND_UNEXPECTED_ITEM,
   SDB_ERR_OOM,
   SDB_ERR_OVF,
   SDB_ERR_SIZE_OVF,
   SDB_ERR_TYPE_UNSUPPORTED,
   SDB_ERR_INVALID_ARG,

   SDB_ERR_INNER_CODE_BEGIN = 40000,
   SDB_ERR_INNER_CODE_END = 50000
};

void convert_sdb_code( int &rc ) ;

int get_sdb_code( int rc ) ;

#endif