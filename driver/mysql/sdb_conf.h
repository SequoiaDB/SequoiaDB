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

#ifndef SDB_CONF__H
#define SDB_CONF__H

#include <mysql/psi/mysql_file.h>
#include "sdb_def.h"

extern PSI_memory_key sdb_key_memory_conf_coord_addrs ;

class sdb_conf
{
public:

   ~sdb_conf() ;

   static sdb_conf *get_instance() ;

   int parse_conn_addrs( const char *conn_addrs ) ;

   char **get_coord_addrs() ;

   int get_coord_num() ;

private:

   sdb_conf() ;

   sdb_conf(const sdb_conf & rh){}

   sdb_conf & operator = (const sdb_conf & rh) { return *this ;}

   void clear_coord_addrs( int num ) ;

private:
   char                          *pAddrs[SDB_COORD_NUM_MAX] ;
   int                           coord_num ;
   pthread_rwlock_t              addrs_mutex ;
} ;

#define SDB_CONF_INST            sdb_conf::get_instance()

#endif