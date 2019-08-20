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

#ifndef SDB_CONN__H
#define SDB_CONN__H

#include <map>
#include <mysql/psi/mysql_thread.h>
#include <my_global.h>
#include <atomic_class.h>
#include "include/client.hpp"


class sdb_conn_auto_ptr ;
class sdb_cl_auto_ptr ;
class sdb_conn
{
public:

   sdb_conn( my_thread_id _tid ) ;

   ~sdb_conn() ;

   int connect( ) ;

   sdbclient::sdb & get_sdb() ;

   my_thread_id get_tid() ;

   int begin_transaction() ;

   int commit_transaction() ;

   int rollback_transaction() ;

   bool is_transaction() ;

   int get_cl( char *cs_name, char *cl_name,
               sdb_cl_auto_ptr &cl_ptr,
               bool create = FALSE,
               const bson::BSONObj &options = sdbclient::_sdbStaticObject ) ;

   int create_cl( char *cs_name, char *cl_name,
                  sdb_cl_auto_ptr &cl_ptr,
                  const bson::BSONObj &options = sdbclient::_sdbStaticObject ) ;

   void clear_cl( char *cs_name, char *cl_name ) ;

   void clear_all_cl() ;

   bool is_idle() ;

private:
   sdbclient::sdb                                  connection ;
   bool                                            transactionon ;
   my_thread_id                                    tid ;
   pthread_rwlock_t                                rw_mutex ;
   std::multimap<std::string, sdb_cl_auto_ptr>     cl_list ;
} ;

#endif