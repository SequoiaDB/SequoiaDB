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

#ifndef SDB_CONDITION__H
#define SDB_CONDITION__H

#include "sdb_item.h"

enum sdb_cond_status
{
   sdb_cond_supported = 1,
   sdb_cond_partsupported,
   sdb_cond_beforesupported,
   sdb_cond_unsupported,

   sdb_cond_unknown = 65535
} ;

class sdb_cond_ctx : public Sql_alloc
{
public:

   sdb_cond_ctx() ;

   ~sdb_cond_ctx() ;

   void push( Item *cond_item ) ;

   void pop() ;

   void pop_all() ;

   void clear() ;

   sdb_item *create_sdb_item( Item_func *cond_item ) ;

   int to_bson( bson::BSONObj &obj ) ;

   void update_stat( int rc ) ;

   bool keep_on() ;

   sdb_item                   *cur_item ;
   List<sdb_item>             item_list ;
   sdb_cond_status            status ;
} ;

void sdb_parse_condtion( const Item *cond_item, sdb_cond_ctx *sdb_cond ) ;


#endif