/* Copyright (c) 2004, 2016, Oracle and/or its affiliates. All rights reserved.

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

#ifndef MYSQL_SERVER
   #define MYSQL_SERVER
#endif

#include "sql_class.h"  
#include "ha_sdb.h"
#include <mysql/plugin.h>
#include <include/client.hpp>
#include <mysql/psi/mysql_file.h>
#include <log.h>
#include <time.h>
#include "sdb_conf.h"
#include "sdb_cl.h"
#include "sdb_conn.h"
#include "sdb_util.h"
#include "sdb_condition.h"
#include "sdb_err_code.h"
#include "sdb_idx.h"

using namespace sdbclient ;

#ifndef SDB_VER
#define SDB_VER                  "UNKNOWN"
#endif
#define SDB_VER_INFO_NAME        "SequoiadbPluginVersion: "
#define SDB_VER_INFO_F1()        SDB_VER_INFO_NAME
#ifdef DEBUG
#define SDB_VER_INFO_F2()        SDB_VER" Debug"
#else
#define SDB_VER_INFO_F2()        SDB_VER" Release"
#endif
#define SDB_VER_INFO             (SDB_VER_INFO_F1()SDB_VER_INFO_F2())

#define SDB_DATA_EXT             ".data"
#define SDB_IDX_EXT              ".idx"
#define SDB_LOB_DATA_EXT         ".lobd"
#define SDB_LOB_META_EXT         ".lobm"
#define SDB_ID_STR_LEN           24
#define SDB_FIELD_MAX_LEN        (16*1024*1024)
#define SDB_STR_BUF_STEP_SIZE    1024
#define SDB_STR_BUF_SIZE         SDB_STR_BUF_STEP_SIZE
const static char sdb_ver_info[] = SDB_VER_INFO ;
static char *sdb_addr = NULL ;
static const char* SDB_ADDR_DFT = "localhost:11810" ;
mysql_mutex_t sdb_mutex;
static PSI_mutex_key key_mutex_sdb, key_mutex_SDB_SHARE_mutex ;
bson::BSONObj empty_obj ;
static HASH sdb_open_tables;
static PSI_memory_key key_memory_sdb_share;

static uchar* sdb_get_key( SDB_SHARE *share, size_t *length,
                           my_bool not_used MY_ATTRIBUTE((unused)))
{
  *length=share->table_name_length;
  return (uchar*) share->table_name;
}

static SDB_SHARE *get_sdb_share(const char *table_name, TABLE *table)
{
   SDB_SHARE *share = NULL ;
   char *tmp_name = NULL ;
   uint length;

   mysql_mutex_lock(&sdb_mutex);
   length=(uint) strlen(table_name);

   /*
    If share is not present in the hash, create a new share and
    initialize its members.
   */


   if ( !(share=(SDB_SHARE*) my_hash_search( &sdb_open_tables,
                                             (uchar*) table_name,
                                             length )))
   {
      if ( !my_multi_malloc( key_memory_sdb_share,
                            MYF(MY_WME | MY_ZEROFILL),
                            &share, sizeof(*share),
                            &tmp_name, length+1,
                            NullS ) )
      {
         goto error ;
      }

      share->use_count = 0 ;
      share->table_name_length = length ;
      share->table_name= tmp_name ;
      strncpy( share->table_name, table_name, length ) ;

      if ( my_hash_insert( &sdb_open_tables, (uchar*) share ))
      {
         goto error ;
      }
      thr_lock_init( &share->lock ) ;
      mysql_mutex_init( key_mutex_SDB_SHARE_mutex,
                        &share->mutex, MY_MUTEX_INIT_FAST ) ;
   }

   share->use_count++ ;

done:
   mysql_mutex_unlock( &sdb_mutex ) ;
  return share ;
error:
   if ( share )
   {
      my_free(share) ;
      share = NULL ;
   }
  goto done ;
}

static int free_sdb_share(SDB_SHARE *share)
{
   mysql_mutex_lock(&sdb_mutex) ;
   if (!--share->use_count)
   {
      my_hash_delete( &sdb_open_tables, (uchar*) share ) ;
      thr_lock_delete( &share->lock ) ;
      mysql_mutex_destroy( &share->mutex ) ;
      my_free(share) ;
  }
  mysql_mutex_unlock(&sdb_mutex) ;

  return 0 ;
}

ha_sdb::ha_sdb(handlerton * hton, TABLE_SHARE * table_arg)
   :handler(hton, table_arg)
{
   keynr = -1 ;
   str_field_buf_size = SDB_STR_BUF_SIZE ;
   str_field_buf = (char *)malloc( str_field_buf_size ) ;
   share = NULL ;
   first_read = TRUE ;
   memset( db_name, 0, CS_NAME_MAX_SIZE+1 ) ;
   memset( table_name, 0, CL_NAME_MAX_SIZE+1 ) ;
}

ha_sdb::~ha_sdb()
{
   if ( str_field_buf != NULL )
   {
      free( str_field_buf ) ;
      str_field_buf_size = 0 ;
   }
}

static const char *ha_sdb_exts[] = {
   SDB_DATA_EXT,
   SDB_IDX_EXT,
   SDB_LOB_DATA_EXT,
   SDB_LOB_META_EXT,
   NullS
};
const char **ha_sdb::bas_ext() const
{
   return ha_sdb_exts ;
}

ulonglong ha_sdb::table_flags() const
{
   return ( HA_REC_NOT_IN_SEQ | HA_NO_AUTO_INCREMENT | HA_NO_READ_LOCAL_LOCK
            | HA_BINLOG_ROW_CAPABLE | HA_BINLOG_STMT_CAPABLE
            | HA_TABLE_SCAN_ON_INDEX | HA_NULL_IN_KEY | HA_CAN_INDEX_BLOBS ) ;
}

ulong ha_sdb::index_flags( uint inx, uint part, bool all_parts ) const
{
   // TODO: SUPPORT SORT
   //HA_READ_NEXT | HA_KEYREAD_ONLY ;
   return ( HA_READ_RANGE | HA_DO_INDEX_COND_PUSHDOWN | HA_READ_NEXT
            | HA_READ_ORDER ) ;
}

uint ha_sdb::max_supported_record_length() const
{
   return HA_MAX_REC_LENGTH ;
}

uint ha_sdb::max_supported_keys() const
{
   return MAX_KEY ;
}

uint ha_sdb::max_supported_key_part_length() const
{
   return 1024 ;
}

uint ha_sdb::max_supported_key_length() const
{
   return 255 ;
}

int ha_sdb::open( const char *name, int mode, uint test_if_locked )
{
   int rc = 0 ;
   ref_length = SDB_ID_STR_LEN + 1 ; //length of _id

   if ( !(share = get_sdb_share(name, table )))
   {
      rc = SDB_ERR_OOM ;
      goto error ;
   }

   rc = sdb_parse_table_name( name, db_name, CS_NAME_MAX_SIZE+1,
                              table_name, CL_NAME_MAX_SIZE+1 ) ;
   if ( rc != 0 )
   {
      goto error ;
   }

   rc = SDB_CONN_MGR_INST->get_sdb_conn( ha_thd()->thread_id(),
                                         connection ) ;
   //rc = SDB_CONN_MGR_INST->get_sdb_conn( (my_thread_id)(ha_thd()->active_vio->mysql_socket.fd),
   //                                      connection ) ;
   if ( 0 != rc )
   {
      goto error ;
   }
   
   rc = connection->get_cl( db_name, table_name, cl ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   thr_lock_data_init( &share->lock, &lock_data, (void*)this ) ;
   //fd = ha_thd()->active_vio->mysql_socket.fd ;
   fd = ha_thd()->thread_id() ;
done:
   return rc ;
error:
   if ( share )
   {
      free_sdb_share( share ) ;
      share = NULL ;
   }
   goto done ;
}

int ha_sdb::close(void)
{
   cl.clear() ;
   connection.clear() ;
   if ( share )
   {
      free_sdb_share( share ) ;
      share = NULL ;
   }
   return 0;
}

int ha_sdb::row_to_obj( uchar *buf,  bson::BSONObj & obj )
{
   int rc = 0 ;
   bson::BSONObjBuilder obj_builder ;

   my_bitmap_map *org_bitmap= dbug_tmp_use_all_columns(table, table->read_set) ;
   if ( buf != table->record[0] )
   {
      repoint_field_to_record( table, table->record[0], buf ) ;
   }

   for( Field **field = table->field ; *field ; field++ )
   {
      //skip the null field
      if( (*field)->is_null() )
      {
         continue ;
      }

      //TODO: process the quotes
      switch( (*field)->type())
      {
         case MYSQL_TYPE_SHORT:
         case MYSQL_TYPE_LONG:
         case MYSQL_TYPE_TINY:
         case MYSQL_TYPE_YEAR:
         case MYSQL_TYPE_INT24:
            {
               if ( ((Field_num *)(*field))->unsigned_flag )
               {
                  obj_builder.append( (*field)->field_name,
                                      (long long)((*field)->val_int()) ) ;
               }
               else
               {
                  obj_builder.append( (*field)->field_name,
                                      (int)(*field)->val_int() ) ;
               }
               break ;
            }
         case MYSQL_TYPE_LONGLONG:
            {
               if( ((Field_num *)(*field))->unsigned_flag )
               {
                  my_decimal tmp_val ;
                  char buff[MAX_FIELD_WIDTH];
                  String str(buff, sizeof(buff), (*field)->charset() );
                  ((Field_num *)(*field))->val_decimal( &tmp_val ) ;
                  my_decimal2string(E_DEC_FATAL_ERROR, &tmp_val, 0, 0, 0, &str ) ;
                  obj_builder.appendDecimal( (*field)->field_name,
                                             str.c_ptr() ) ;
               }
               else
               {
                  obj_builder.append( (*field)->field_name,
                                      (*field)->val_int() ) ;
               }
               break ;
            }
         case MYSQL_TYPE_FLOAT:
         case MYSQL_TYPE_DOUBLE:
         case MYSQL_TYPE_TIME:
            {
               obj_builder.append( (*field)->field_name,
                                   (*field)->val_real() ) ;
               break ;
            }
         case MYSQL_TYPE_TINY_BLOB:
         case MYSQL_TYPE_MEDIUM_BLOB:
         case MYSQL_TYPE_LONG_BLOB:
         case MYSQL_TYPE_BLOB:
            {
               Field_str *f = (Field_str *)(*field) ;
               if ( str_field_buf_size < (*field)->data_length() )
               {
                  uint32 str_buf_size_new = 0 ;
                  if ( (*field)->data_length() >= SDB_FIELD_MAX_LEN )
                  {
                     my_printf_error( ER_TOO_BIG_FIELDLENGTH,
                                      ER(ER_TOO_BIG_FIELDLENGTH),
                                      MYF(0), (*field)->field_name,
                                      static_cast<ulong>(SDB_FIELD_MAX_LEN-1));
                     rc = -1 ;
                     goto error ;
                  }
                  str_buf_size_new
                     = ( (*field)->data_length() / SDB_STR_BUF_STEP_SIZE + 1 )
                       * SDB_STR_BUF_STEP_SIZE ;
                  str_field_buf = (char *)realloc( str_field_buf,
                                                   str_buf_size_new ) ;
                  str_field_buf_size = str_buf_size_new ;
               }
               String val_tmp( str_field_buf, str_field_buf_size, (*field)->charset() ) ;
               (*field)->val_str( &val_tmp, &val_tmp ) ;
               if ( f->binary() )
               {
                  obj_builder.appendBinData( (*field)->field_name,
                                             val_tmp.length(), bson::BinDataGeneral,
                                             val_tmp.ptr() ) ;
               }
               else
               {
                  obj_builder.appendStrWithNoTerminating( (*field)->field_name,
                                                        val_tmp.ptr(),
                                                        val_tmp.length() ) ;
               }
               break ;
            }
         case MYSQL_TYPE_VARCHAR:
         case MYSQL_TYPE_STRING:
         case MYSQL_TYPE_VAR_STRING:
            {
               if ( str_field_buf_size < (*field)->data_length() )
               {
                  uint32 str_buf_size_new = 0 ;
                  if ( (*field)->data_length() >= SDB_FIELD_MAX_LEN )
                  {
                     my_printf_error( ER_TOO_BIG_FIELDLENGTH,
                                      ER(ER_TOO_BIG_FIELDLENGTH),
                                      MYF(0), (*field)->field_name,
                                      static_cast<ulong>(SDB_FIELD_MAX_LEN-1));
                     rc = -1 ;
                     goto error ;
                  }
                  str_buf_size_new
                     = ( (*field)->data_length() / SDB_STR_BUF_STEP_SIZE + 1 )
                       * SDB_STR_BUF_STEP_SIZE ;
                  str_field_buf = (char *)realloc( str_field_buf,
                                                   str_buf_size_new ) ;
                  str_field_buf_size = str_buf_size_new ;
               }
               String val_tmp( str_field_buf, str_field_buf_size, (*field)->charset() ) ;
               (*field)->val_str( &val_tmp, &val_tmp ) ;
               obj_builder.appendStrWithNoTerminating( (*field)->field_name,
                                                        val_tmp.ptr(),
                                                        val_tmp.length() ) ;
               break ;
            }
         case MYSQL_TYPE_NEWDECIMAL:
         case MYSQL_TYPE_DECIMAL:
            {
               Field_decimal *f = (Field_decimal *)(*field) ;
               int precision = (int)(f->pack_length()) ;
               int scale = (int)(f->decimals()) ;
               if ( precision < 0 || scale < 0 )
               {
                  rc = -1 ;
                  goto error ;
               }
               char buff[MAX_FIELD_WIDTH];
               String str(buff, sizeof(buff), (*field)->charset() );
               String unused;
               f->val_str( &str, &unused ) ;
               obj_builder.appendDecimal( (*field)->field_name,
                                         str.c_ptr() ) ;
               break ;
            }

         case MYSQL_TYPE_DATE:
            {
               longlong date_val = 0 ;
               date_val = ((Field_newdate*)(*field))->val_int() ;
               struct tm tm_val ;
               tm_val.tm_sec = 0 ;
               tm_val.tm_min = 0 ;
               tm_val.tm_hour = 0 ;
               tm_val.tm_mday = date_val % 100 ;
               date_val = date_val / 100 ;
               tm_val.tm_mon = date_val % 100 - 1 ;
               date_val = date_val / 100 ;
               tm_val.tm_year = date_val - 1900 ;
               tm_val.tm_wday = 0 ;
               tm_val.tm_yday = 0 ;
               tm_val.tm_isdst = 0 ;
               time_t time_tmp = mktime( &tm_val ) ;
               bson::Date_t dt( (longlong)(time_tmp * 1000) ) ;
               obj_builder.appendDate( (*field)->field_name, dt ) ;
               break ;
            }
         case MYSQL_TYPE_TIMESTAMP2:
         case MYSQL_TYPE_TIMESTAMP:
            {
               struct timeval tm ;
               int warnings= 0;
               (*field)->get_timestamp( &tm, &warnings ) ;
               obj_builder.appendTimestamp( (*field)->field_name,
                                            tm.tv_sec*1000,
                                            tm.tv_usec ) ;
               break ;
            }

         /*case MYSQL_TYPE_TIMESTAMP2:
            {
               Field_timestampf *f = (Field_timestampf *)(*field) ;
               struct timeval tm ;
               f->get_timestamp( &tm, NULL ) ;
               obj_builder.appendTimestamp( (*field)->field_name,
                                            tm.tv_sec*1000,
                                            tm.tv_usec ) ;
               break ;
            }*/

         case MYSQL_TYPE_NULL:
            //skip the null value
            break ;

         case MYSQL_TYPE_DATETIME:
            {
               MYSQL_TIME ltime ;
               if ( !(*field)->get_time( &ltime ) )
               {
                  struct tm tm_val ;
                  tm_val.tm_sec = ltime.second ;
                  tm_val.tm_min = ltime.minute ;
                  tm_val.tm_hour = ltime.hour ;
                  tm_val.tm_mday = ltime.day ;
                  tm_val.tm_mon = ltime.month - 1 ;
                  tm_val.tm_year = ltime.year - 1900 ;
                  tm_val.tm_wday = 0 ;
                  tm_val.tm_yday = 0 ;
                  tm_val.tm_isdst = 0 ;
                  time_t time_tmp = mktime( &tm_val ) ;
                  unsigned long long time_val_tmp ;
                  memcpy( (char *)&time_val_tmp, &(ltime.second_part), 4 ) ;
                  memcpy( (char *)&time_val_tmp+4, &time_tmp, 4 ) ;
                  obj_builder.appendTimestamp( (*field)->field_name,
                                               time_val_tmp ) ;
                  break ;
               }
               // go to default and return error ;
            }

         default:
            {
               my_error( ER_BAD_FIELD_ERROR, MYF(0),
                         (*field)->field_name, table_name ) ;
               rc = -1 ;
               goto error ;
            }
            
      }
      
   }
   obj = obj_builder.obj() ;

done:
   if ( buf != table->record[0] )
   {
      repoint_field_to_record( table, buf, table->record[0] ) ;
   }
   dbug_tmp_restore_column_map( table->read_set, org_bitmap ) ;
   return rc ;
error:
   goto done ;
}

int ha_sdb::write_row(uchar *buf)
{
   int rc = 0 ;
   bson::BSONObj obj ;
   ha_statistic_increment( &SSV::ha_write_count ) ;
   check_thread() ;

   rc = row_to_obj( buf, obj ) ;
   if ( rc != 0 )
   {
      goto error ;
   }

   rc = cl->insert( obj ) ;
   if ( rc != 0 )
   {
      goto error ;
   }

   stats.records++ ;
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::update_row( const uchar *old_data, uchar *new_data )
{
   int rc = 0 ;
   bson::BSONObj old_obj, new_obj, rule_obj ;
   check_thread() ;

   ha_statistic_increment( &SSV::ha_update_count ) ;

   rc = row_to_obj( new_data, new_obj ) ;
   if ( rc != 0 )
   {
      goto error ;
   }

   rule_obj = BSON( "$set" << new_obj ) ;
   rc = cl->update( rule_obj, cur_rec ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::delete_row( const uchar *buf )
{
   int rc = 0 ;
   bson::BSONObj obj ;
   check_thread() ;

   ha_statistic_increment( &SSV::ha_delete_count ) ;

   rc = cl->del( cur_rec) ;
   if ( rc != 0 )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::index_next(uchar *buf)
{
   int rc = 0 ;
   ha_statistic_increment( &SSV::ha_read_next_count ) ;
   rc = next_row( cur_rec, buf ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
   stats.records++ ;
done:
   return rc ;
error:
   goto done ;
}

   int ha_sdb::index_prev(uchar *buf)
   {
      return -1 ;
   }

   int ha_sdb::index_last(uchar *buf)
   {
      return -1 ;
   }

int ha_sdb::index_first(uchar *buf)
{
   int rc = 0 ;
   bson::BSONObj hint ;
   const char *idx_name = NULL ;
   idx_name = sdb_get_idx_name( table->key_info + keynr ) ;
   if ( idx_name )
   {
      hint = BSON( "" << idx_name ) ;
   }
   rc = cl->query( condition, sdbclient::_sdbStaticObject,
                   sdbclient::_sdbStaticObject, hint ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = index_next( buf ) ;
   switch(rc)
   {
      case SDB_OK:
         {
         	table->status = 0 ;
   		   break;
         }

      case SDB_DMS_EOC:
      case HA_ERR_END_OF_FILE:
         {
            rc = HA_ERR_KEY_NOT_FOUND ;
            table->status = STATUS_NOT_FOUND ;
            break ;
         }

      default:
         {
            table->status = STATUS_NOT_FOUND ;
            break ;
         }
   }
done:
   condition = empty_obj ;
   return rc ;
error:
   goto done ;
}

int ha_sdb::index_read_map( uchar *buf, const uchar *key_ptr,
                            key_part_map keypart_map,
                            enum ha_rkey_function find_flag )
{
   int rc = 0 ;
   bson::BSONObj orderbyObj, hint, condition_idx ;
   bson::BSONObjBuilder cond_builder ;
   const char *idx_name = NULL ;
   if ( NULL != key_ptr && keynr >= 0 )
   {
      rc = build_match_obj_by_start_stop_key( (uint)keynr, key_ptr,
                                         keypart_map, find_flag,
                                         end_range, table,
                                         condition_idx ) ;
   }
   if ( rc )
   {
      goto error ;
   }
   if ( !condition.isEmpty() )
   {
      cond_builder.appendElements( condition ) ;
      cond_builder.appendElements( condition_idx ) ;
      condition = cond_builder.obj() ;
   }
   else
   {
      condition = condition_idx ;
   }
   idx_name = sdb_get_idx_name( table->key_info + keynr ) ;
   if ( idx_name )
   {
      hint = BSON( "" << idx_name ) ;
   }
   rc = cl->query( condition, sdbclient::_sdbStaticObject,
                   sdbclient::_sdbStaticObject, hint ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = index_next( buf ) ;
   switch(rc)
   {
      case SDB_OK:
         {
         	table->status = 0 ;
   		   break;
         }

      case SDB_DMS_EOC:
      case HA_ERR_END_OF_FILE:
         {
            rc = HA_ERR_KEY_NOT_FOUND ;
            table->status = STATUS_NOT_FOUND ;
            break ;
         }

      default:
         {
            table->status = STATUS_NOT_FOUND ;
            break ;
         }
   }
done:
   condition = empty_obj ;
   return rc ;
error:
   goto done ;
}

int ha_sdb::index_init(uint idx, bool sorted)
{
   keynr = (int)idx ;
   active_index = idx ;
   if( !pushed_cond )
   {
      condition = empty_obj ;
   }
   return 0 ;
}

int ha_sdb::index_end()
{
   cl->close() ;
   keynr = -1 ;
   return 0 ;
}

double ha_sdb::scan_time()
{
   //TODO*********
   return 10 ;
}

double ha_sdb::read_time( uint index, uint ranges, ha_rows rows )
{
   //TODO********
   return rows ;
}

int ha_sdb::rnd_init(bool scan)
{
   stats.records= 0;
   first_read = TRUE ;
   if( !pushed_cond )
   {
      condition = empty_obj ;
   }
   return 0 ;
}

int ha_sdb::rnd_end()
{
   cl->close() ;
   return 0 ;
}

int ha_sdb::obj_to_row( bson::BSONObj &obj, uchar *buf )
{
   //TODO: parse other types
   //get filed by field-name, order by filed_index
   int rc = 0 ;
   bool read_all ;
   my_bitmap_map *org_bitmap;
   memset( buf, 0, table->s->null_bytes ) ;

   read_all= !bitmap_is_clear_all(table->write_set) ;

  /* Avoid asserts in ::store() for columns that are not going to be updated */
  org_bitmap= dbug_tmp_use_all_columns(table, table->write_set);

   for( Field **field=table->field ; *field ; field++ )
   {
      if ( !bitmap_is_set(table->read_set,(*field)->field_index)
           && !read_all )
      {
         continue ;
      }

      (*field)->reset() ;
      bson::BSONElement befield ;
      befield = obj.getField( (*field)->field_name ) ;
      if ( befield.eoo() || befield.isNull() )
      {
         (*field)->set_null() ;
         continue ;
      }
      switch ( befield.type() )
      {
         case bson::NumberInt:
         case bson::NumberLong:
            {
               longlong nr = befield.numberLong() ;
               (*field)->store( nr, false ) ;
               break ;
            }
         case bson::NumberDouble:
            {
               double nr = befield.numberDouble() ;
               (*field)->store( nr ) ;
               break ;
            }
         case bson::BinData:
            {
               int lenTmp = 0 ;
               const char * dataTmp = befield.binData( lenTmp ) ;
               if ( lenTmp < 0 )
               {
                  lenTmp = 0 ;
               }
               (*field)->store( dataTmp, lenTmp,
                                &my_charset_bin ) ;
               break ;
                              
            }
         case bson::String:
            {
               (*field)->store( befield.valuestr(),
                                befield.valuestrsize()-1,
                                &my_charset_bin ) ;
               break ;
                              
            }
         case bson::NumberDecimal:
            {
               bson::bsonDecimal valTmp = befield.numberDecimal() ;
               string strValTmp = valTmp.toString() ;
               (*field)->store( strValTmp.c_str(),
                                strValTmp.length(),
                                &my_charset_bin ) ;
               break ;
                              
            }
         case bson::Date:
         case bson::Timestamp:
            {
               longlong milTmp = 0 ;
               longlong microTmp = 0 ;
               struct timeval tv ;
               if ( bson::Timestamp == befield.type() )
               {
                  milTmp = (longlong)(befield.timestampTime()) ;
                  microTmp = befield.timestampInc() ;
               }
               else
               {
                  milTmp = (longlong)(befield.date()) ;
               }
               tv.tv_sec = milTmp / 1000 ;
               tv.tv_usec = milTmp % 1000 * 1000 + microTmp ;
               if( is_temporal_type_with_date_and_time((*field)->type()) )
               {
                  Field_temporal_with_date_and_time *f
                        = (Field_temporal_with_date_and_time *)(*field) ;
                  f->store_timestamp( &tv ) ;
               }
               else if( (*field)->type() == MYSQL_TYPE_TIMESTAMP2 )
               {
                  Field_temporal_with_date_and_timef *f
                        = (Field_temporal_with_date_and_timef *)(*field) ;
                  f->store_timestamp( &tv) ;
               }
               else if( is_temporal_type_with_date( (*field)->type() ))
               {
                  MYSQL_TIME myTime ;
                  struct tm tmTmp ;
                  localtime_r( (const time_t *)(&tv.tv_sec), &tmTmp ) ;
                  myTime.year = tmTmp.tm_year + 1900 ;
                  myTime.month = tmTmp.tm_mon + 1 ;
                  myTime.day = tmTmp.tm_mday ;
                  myTime.hour = tmTmp.tm_hour ;
                  myTime.minute = tmTmp.tm_min ;
                  myTime.second = tmTmp.tm_sec ;
                  myTime.second_part = 0 ;
                  myTime.neg = 0 ;
                  myTime.time_type = MYSQL_TIMESTAMP_DATETIME ;
                  Field_temporal_with_date *f
                        = (Field_temporal_with_date *)(*field) ;
                  f->store_time( &myTime, MYSQL_TIMESTAMP_TIME ) ;
               }
               else
               {
                  longlong nr
                     = (longlong)(befield.timestampTime()) * 1000
                        + befield.timestampInc() ;
                  (*field)->store( nr, false ) ;
               }
               break ;
            }
         case bson::Object:
         case bson::Bool:
         default:
            (*field)->store( "", 0, &my_charset_bin ) ;
            rc = SDB_ERR_TYPE_UNSUPPORTED ;
            goto error ;
      }
   }
done:
   dbug_tmp_restore_column_map(table->write_set, org_bitmap);
   return rc ;
error:
   goto done ;
}

int ha_sdb::cur_row( uchar *buf )
{
   int rc = 0 ;
   //assert(cl->get_tid() == ha_thd()->thread_id()) ;
   rc = cl->current( cur_rec ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
   rc = obj_to_row( cur_rec, buf ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::next_row( bson::BSONObj &obj,uchar *buf )
{
   int rc = 0 ;
   rc = cl->next( obj ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
   rc = obj_to_row( obj, buf ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::rnd_next(uchar *buf)
{
   int rc = 0 ;
   if ( first_read )
   {
      check_thread() ;
      rc = cl->query( condition ) ;
      condition = empty_obj ;
      if ( rc != 0 )
      {
         goto error ;
      }
      first_read = FALSE ;
   }
   assert(cl->get_tid() == ha_thd()->thread_id()) ;
   //assert( cl->get_tid() == (my_thread_id)(ha_thd()->active_vio->mysql_socket.fd )) ;
   ha_statistic_increment( &SSV::ha_read_rnd_next_count ) ;
   rc = next_row( cur_rec, buf ) ;
   if ( rc != 0 )
   {
      goto error ;
   }
   stats.records++ ;
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::rnd_pos( uchar *buf, uchar *pos )
{
   int rc = 0 ;
   bson::BSONObjBuilder objBuilder ;
   bson::OID tmpOid( (const char *)pos ) ;
   objBuilder.appendOID( "_id", &tmpOid ) ;
   bson::BSONObj oidObj = objBuilder.obj() ;

   //assert(cl->get_tid() == ha_thd()->thread_id()) ;
   rc = cl->query_one( cur_rec, oidObj ) ;
   if ( rc )
   {
      goto error ;
   }

   ha_statistic_increment( &SSV::ha_read_rnd_count ) ;
   rc = obj_to_row( cur_rec, buf ) ;
   if ( rc != 0 )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

void ha_sdb::position( const uchar *record )
{
   bson::BSONElement beField ;
   if( cur_rec.getObjectID( beField ))
   {
      bson::OID tmpOid ;
      tmpOid = beField.__oid() ;
      memcpy( ref, tmpOid.str().c_str(), SDB_ID_STR_LEN ) ;
      ref[SDB_ID_STR_LEN] = 0 ;
      if ( beField.type() != bson::jstOID )
      {
         sql_print_error("Unexpected _id's type: %d ",
				             beField.type() );
      }
   }
   return ;
}

int ha_sdb::info( uint flag )
{
   int rc = 0 ;
   //long long count = 0 ;

   //TODO: fill the stats with actual info.
   stats.data_file_length = 107374182400LL ; //100*1024*1024*1024
   stats.max_data_file_length = 1099511627776LL ; //1*1024*1024*1024*1024
   stats.index_file_length = 1073741824LL ; //1*1024*1024*1024
   stats.max_index_file_length = 10737418240LL ; //10*1024*1024*1024
   stats.delete_length = 0 ;
   stats.auto_increment_value = 0 ;

   /*rc = cl.getCount( count ) ;
   if ( rc != 0  )
   {
      goto error ;
   }*/

   stats.records = 10000 ;
   stats.deleted = 0 ;
   stats.mean_rec_length = 1024 ;
   stats.create_time = 0 ;
   stats.check_time = 0 ;
   stats.update_time = 0 ;
   stats.block_size = 0 ;
   stats.mrr_length_per_rec = 0 ;
   stats.table_in_mem_estimate = -1 ;

done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::extra( enum ha_extra_function operation )
{
   //TODO: extra hints
   return 0 ;
}

void ha_sdb::check_thread()
{
   int rc = 0 ;
   if ( cl->get_tid() != ha_thd()->thread_id() )
   //if ( cl->get_tid() != (my_thread_id)(ha_thd()->active_vio->mysql_socket.fd) )
   {
      //first_read = TRUE ;
      //stats.records= 0;
      sdb_conn_auto_ptr conn_tmp ;
      rc = SDB_CONN_MGR_INST->get_sdb_conn( ha_thd()->thread_id(),
                                            conn_tmp ) ;
      //rc = SDB_CONN_MGR_INST->get_sdb_conn( (my_thread_id)(ha_thd()->active_vio->mysql_socket.fd),
      //                                      conn_tmp ) ;
      if ( 0 != rc )
      {
         goto error ;
      }
      
      rc = conn_tmp->get_cl( db_name, table_name, cl ) ;
      if ( 0 != rc )
      {
         goto error ;
      }

      connection = conn_tmp ;
      //fd = ha_thd()->active_vio->mysql_socket.fd ;
      fd = ha_thd()->thread_id() ;
   }
done:
   return ;
error:
   assert( FALSE ) ;
   goto done ;
}

int ha_sdb::external_lock( THD *thd, int lock_type )
{
   int rc = 0 ;
   check_thread() ;

   ulonglong trxid = thd->thread_id() ;
   if( !thd_test_options( thd, OPTION_BEGIN ) )
   {
      goto done ;
   }

   rc = cl->begin_transaction() ;
   if ( rc != 0 )
   {
      goto error ;
   }

   //TODO: generate transaction-id
   trans_register_ha( thd, true, ht, &trxid ) ;
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::start_stmt( THD *thd, thr_lock_type lock_type )
{
   return external_lock( thd, lock_type ) ;
}

enum_alter_inplace_result ha_sdb::check_if_supported_inplace_alter(
                                          TABLE *altered_table,
                                          Alter_inplace_info *ha_alter_info)
{
   enum_alter_inplace_result rs ;
   KEY *keyInfo ;

   Alter_inplace_info::HA_ALTER_FLAGS inplace_online_operations
      = Alter_inplace_info::ADD_INDEX
      | Alter_inplace_info::DROP_INDEX
      | Alter_inplace_info::ADD_UNIQUE_INDEX
      | Alter_inplace_info::DROP_UNIQUE_INDEX
      | Alter_inplace_info::ADD_PK_INDEX
      | Alter_inplace_info::DROP_PK_INDEX ;

   if ( ha_alter_info->handler_flags & ~inplace_online_operations )
   {
      // include offline-operations
      rs = handler::check_if_supported_inplace_alter(
                                 altered_table, ha_alter_info ) ;
      goto done ;
   }

   keyInfo = ha_alter_info->key_info_buffer ;
   for( ; keyInfo < ha_alter_info->key_info_buffer + ha_alter_info->key_count ;
        keyInfo++ )
   {
      KEY_PART_INFO *keyPart ;
      KEY_PART_INFO *keyEnd ;
      /*if ( ( keyInfo->flags & HA_FULLTEXT )
         || ( keyInfo->flags & HA_PACK_KEY )
         || ( keyInfo->flags & HA_BINARY_PACK_KEY ))
      {
         rs = HA_ALTER_INPLACE_NOT_SUPPORTED ;
         goto done ;
      }*/
      keyPart = keyInfo->key_part ;
      keyEnd = keyPart + keyInfo->user_defined_key_parts ;
      for( ; keyPart < keyEnd ; keyPart++ )
      {
         keyPart->field = altered_table->field[keyPart->fieldnr] ;
         keyPart->null_offset = keyPart->field->null_offset() ;
         keyPart->null_bit = keyPart->field->null_bit ;
         if( keyPart->field->flags & AUTO_INCREMENT_FLAG )
         {
            rs = HA_ALTER_INPLACE_NOT_SUPPORTED ;
            goto done ;
         }
      }
   }
   
   rs = HA_ALTER_INPLACE_NO_LOCK ;
done:
   return rs ;
}

bool ha_sdb::prepare_inplace_alter_table( TABLE *altered_table,
                                          Alter_inplace_info *ha_alter_info )
{
   THD *thd = current_thd ;
   bool rs = false ;
   switch( thd_sql_command(thd) )
   {
   case SQLCOM_CREATE_INDEX:
   case SQLCOM_DROP_INDEX:
      rs = false ;
      break ;
   default:
      rs = true ;
      goto error ;
   }
done:
   return rs ;
error:
   goto done ;
}

int ha_sdb::create_index( Alter_inplace_info *ha_alter_info )
{
   const KEY_PART_INFO *keyPart ;
   const KEY_PART_INFO *keyEnd ;
   const KEY *keyInfo ;
   int rc = 0 ;

   for( uint i = 0 ; i < ha_alter_info->index_add_count ; i++ )
   {
      keyInfo
         = &ha_alter_info->key_info_buffer[ha_alter_info->index_add_buffer[i]] ;
      bson::BSONObjBuilder keyObjBuilder ;
      keyPart = keyInfo->key_part ;
      keyEnd = keyPart + keyInfo->user_defined_key_parts ;
      for( ; keyPart != keyEnd ; ++keyPart )
      {
         if ( keyPart->field->type() < MYSQL_TYPE_TINY
            || ( keyPart->field->type() > MYSQL_TYPE_DOUBLE
               && keyPart->field->type() != MYSQL_TYPE_LONGLONG
               && keyPart->field->type() != MYSQL_TYPE_INT24
               && ( keyPart->field->type() != MYSQL_TYPE_BLOB
                    || keyPart->field->binary() )))
         {
            rc = SDB_ERR_TYPE_UNSUPPORTED ;
            goto error ;
         }
         // TODO: ASC or DESC
         keyObjBuilder.append( keyPart->field->field_name,
                               1 ) ;
      }
      bson::BSONObj keyObj = keyObjBuilder.obj() ;

      //TODO: parse primary-key-index ;
      rc = cl->create_index( keyObj, keyInfo->name, FALSE, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }
   }
done:
   return rc ;
error :
   goto done ;
}

int ha_sdb::drop_index( Alter_inplace_info *ha_alter_info )
{
   int rc = 0 ;

   if ( NULL == ha_alter_info->index_drop_buffer )
   {
      goto done ;
   }

   for( uint i = 0 ; i < ha_alter_info->index_drop_count ; i++ )
   {
      KEY *keyInfo = NULL ;
      keyInfo = ha_alter_info->index_drop_buffer[i] ;
      rc = cl->drop_index( keyInfo->name ) ;
      if ( rc )
      {
         goto error ;
      }
   }
done:
   return rc ;
error :
   goto done ;
}

bool ha_sdb::inplace_alter_table( TABLE *altered_table,
                                  Alter_inplace_info *ha_alter_info )
{
   THD *thd = current_thd ;
   bool rs = false ;
   switch( thd_sql_command(thd) )
   {
   case SQLCOM_CREATE_INDEX:
      if ( 0 != create_index( ha_alter_info ) )
      {
         rs = true ;
      }
      break ;
   case SQLCOM_DROP_INDEX:
      if ( 0 != drop_index( ha_alter_info ) )
      {
         rs = true ;
      }
      break ;
   default:
      rs = true ;
      goto error ;
   }
done:
   return rs ;
error:
   goto done ;
}

int ha_sdb::delete_all_rows(void)
{
   check_thread() ;
   if ( cl->is_transaction())
   {
      return cl->del();
   }
   return this->truncate() ;
}

int ha_sdb::truncate()
{
   int rc = 0 ;
   rc = cl->truncate() ;
   if ( rc != 0 )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

ha_rows ha_sdb::records_in_range(uint inx, key_range *min_key,
                                 key_range *max_key)
{
   //TODO*********
   return 1 ;
}

int ha_sdb::delete_table(const char *from)
{
   int rc = 0 ;
   sdb_conn_auto_ptr conn_tmp ;

   rc = sdb_parse_table_name( from, db_name, CS_NAME_MAX_SIZE+1,
                              table_name, CL_NAME_MAX_SIZE+1 ) ;
   if ( rc != 0 )
   {
      goto error ;
   }


   rc = SDB_CONN_MGR_INST->get_sdb_conn( ha_thd()->thread_id(),
                                         conn_tmp ) ;
   //rc = SDB_CONN_MGR_INST->get_sdb_conn( (my_thread_id)(ha_thd()->active_vio->mysql_socket.fd),
   //                                      conn_tmp ) ;
   if ( 0 != rc )
   {
      goto error ;
   }
   
   rc = conn_tmp->get_cl( db_name, table_name, cl, FALSE ) ;
   if ( 0 != rc )
   {
      int rc_tmp = get_sdb_code( rc ) ;
      if ( SDB_DMS_NOTEXIST == rc_tmp
           || SDB_DMS_CS_NOTEXIST == rc_tmp )
      {
         rc = 0 ;
         goto done ;
      }
      goto error ;
   }

   rc = cl->drop() ;
   if ( 0 != rc )
   {
      goto error ;
   }

   cl.clear() ;

done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::rename_table(const char * from, const char * to)
{
   THD *thd = current_thd ;
   int rc = 0 ;
   switch( thd_sql_command(thd) )
   {
   case SQLCOM_CREATE_INDEX:
      //TODO:***********
      break ;
   case SQLCOM_DROP_INDEX:
      //TODO:************
      break ;
   default:
      rc = -1 ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int ha_sdb::create( const char *name, TABLE *form,
                    HA_CREATE_INFO *create_info)
{
   int rc = 0 ;
   sdbCollectionSpace cs ;
   uint str_field_len = 0 ;
   sdb_conn_auto_ptr conn_tmp ;
   bson::BSONObj options ;

   for( Field **field = form->field ; *field ; field++ )
   {
      if ( (*field)->key_length() > str_field_len
         && ( (*field)->type() == MYSQL_TYPE_VARCHAR
              || (*field)->type() == MYSQL_TYPE_STRING
              || (*field)->type() == MYSQL_TYPE_VAR_STRING
              || (*field)->type() == MYSQL_TYPE_BLOB
              || (*field)->type() == MYSQL_TYPE_TINY_BLOB
              || (*field)->type() == MYSQL_TYPE_MEDIUM_BLOB
              || (*field)->type() == MYSQL_TYPE_LONG_BLOB ))
      {
         str_field_len = (*field)->key_length() ;
         if ( str_field_len >= SDB_FIELD_MAX_LEN )
         {
            my_printf_error(ER_TOO_BIG_FIELDLENGTH, ER(ER_TOO_BIG_FIELDLENGTH),
                              MYF(0), (*field)->field_name,
                              static_cast<ulong>(SDB_FIELD_MAX_LEN-1));
            rc = -1 ;
            goto error ; 
         }
      }
   }

   db_name[CS_NAME_MAX_SIZE] = 0 ;
   strncpy( db_name, form->s->db.str, CS_NAME_MAX_SIZE+1 ) ;
   if ( db_name[CS_NAME_MAX_SIZE] != 0 )
   {
      rc = SDB_ERR_SIZE_OVF ;
      goto error ;
   }

   table_name[CL_NAME_MAX_SIZE] = 0 ;
   strncpy( table_name, form->s->table_name.str, CL_NAME_MAX_SIZE+1 ) ;
   if ( table_name[CL_NAME_MAX_SIZE] != 0 )
   {
      rc = SDB_ERR_SIZE_OVF ;
      goto error ;
   }

   if ( create_info && create_info->comment.str )
   {
      bson::BSONObj comments ;
      bson::BSONElement beOptions ;
      rc = bson::fromjson( create_info->comment.str, comments ) ;
      if ( 0 != rc )
      {
         goto error ;
      }
      beOptions = comments.getField( "cl_options" ) ;
      if ( beOptions.type() == bson::Object )
      {
         options = beOptions.embeddedObject() ;
      }
   }

   rc = SDB_CONN_MGR_INST->get_sdb_conn( ha_thd()->thread_id(),
                                         conn_tmp ) ;
   //rc = SDB_CONN_MGR_INST->get_sdb_conn( (my_thread_id)(ha_thd()->active_vio->mysql_socket.fd),
   //                                      conn_tmp ) ;
   if ( 0 != rc )
   {
      goto error ;
   }
   
   rc = conn_tmp->create_cl( db_name, table_name, cl, options ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   connection = conn_tmp ;
   //fd = ha_thd()->active_vio->mysql_socket.fd ;
   fd = ha_thd()->thread_id() ;

done:
   return rc ;
error:
   goto done ;
}

THR_LOCK_DATA **ha_sdb::store_lock( THD *thd, THR_LOCK_DATA **to,
                                    enum thr_lock_type lock_type)
{
   // TODO: to support commited-read, lock the record by s-lock while
   //       normal read(not update, difference by lock_type). If the
   //       record is not matched, unlock_row() will be called.
   //       if lock_type == TL_READ then lock the record by s-lock
   //       if lock_type == TL_WIRTE then lock the record by x-lock
/*  if (lock_type != TL_IGNORE && lock_data.type == TL_UNLOCK) 
  {
    /* 
      Here is where we get into the guts of a row level lock.
      If TL_UNLOCK is set 
      If we are not doing a LOCK TABLE or DISCARD/IMPORT
      TABLESPACE, then allow multiple writers 
    */

/*    if ((lock_type >= TL_WRITE_CONCURRENT_INSERT &&
         lock_type <= TL_WRITE) && !thd_in_lock_tables(thd)
        && !thd_tablespace_op(thd))
      lock_type = TL_WRITE_ALLOW_WRITE;

    /* 
      In queries of type INSERT INTO t1 SELECT ... FROM t2 ...
      MySQL would use the lock TL_READ_NO_INSERT on t2, and that
      would conflict with TL_WRITE_ALLOW_WRITE, blocking all inserts
      to t2. Convert the lock to a normal read lock to allow
      concurrent inserts to t2. 
    */

/*    if (lock_type == TL_READ_NO_INSERT && !thd_in_lock_tables(thd)) 
      lock_type = TL_READ;

    lock_data.type=lock_type;
  }

  *to++= &lock_data;*/
   return to ;
}

void ha_sdb::unlock_row()
{
   // TODO: this operation is not supported in sdb.
   //       unlock by _id or completed-record?
}

const Item *ha_sdb::cond_push( const Item *cond )
{
   const Item *remain_cond = cond ;
   sdb_cond_ctx sdb_condition ;
   if ( cond->used_tables() & ~table->pos_in_table_list->map() )
   {
      goto done ;
   }

   sdb_parse_condtion( cond, &sdb_condition ) ;
   sdb_condition.to_bson( condition ) ;
   if ( sdb_cond_supported == sdb_condition.status )
   {
      //TODO: build unanalysable condition
      remain_cond = NULL ;
   }
   else
   {
      condition = sdbclient::_sdbStaticObject ;
   }

done:
   return remain_cond;
}

Item *ha_sdb::idx_cond_push(uint keyno, Item* idx_cond)
{
   return idx_cond;
}

const char *ha_sdb::get_version()
{
   return sdb_ver_info ;
}

static handler *sdb_create_handler(handlerton *hton,
                                    TABLE_SHARE *table, 
                                    MEM_ROOT *mem_root)
{
  return new (mem_root) ha_sdb(hton, table);
}

#ifdef HAVE_PSI_INTERFACE

static PSI_memory_info all_sdb_memory[] =
{
   { &key_memory_sdb_share, "SDB_SHARE", PSI_FLAG_GLOBAL },
   { &sdb_key_memory_conf_coord_addrs, "coord_addrs", PSI_FLAG_GLOBAL }
};

static PSI_mutex_info all_sdb_mutexes[] = 
{
   { &key_mutex_sdb, "sdb", PSI_FLAG_GLOBAL },
   { &key_mutex_SDB_SHARE_mutex, "SDB_SHARE::mutex", 0 }
};

static void init_sdb_psi_keys(void)
{
   const char* category = "sequoiadb";
   int count;

   count = array_elements( all_sdb_mutexes ) ;
   mysql_mutex_register( category, all_sdb_mutexes, count ) ;

   count= array_elements( all_sdb_memory);
   mysql_memory_register( category, all_sdb_memory, count ) ;
}
#endif

/*****************************************************************//**
Commits a transaction in
@return 0 */
static
int
sdb_commit(
/*============*/
	handlerton*	hton,
	THD*		thd,
	bool		commit_trx) /*!< in: true - commit transaction
					false - the current SQL statement
					ended */
{
   int rc = 0 ;

   sdb_conn_auto_ptr connection ;

   if ( !commit_trx )
   {
      goto done ;
   }

   rc = SDB_CONN_MGR_INST->get_sdb_conn( thd->thread_id(),
                                         connection ) ;
   //rc = SDB_CONN_MGR_INST->get_sdb_conn( (my_thread_id)(thd->active_vio->mysql_socket.fd),
   //                                      connection ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   rc = connection->commit_transaction() ;

   if ( 0 != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/*****************************************************************//**
Rolls back a transaction
@return 0 if success */
static
int
sdb_rollback(
/*==============*/
	handlerton*	hton,
	THD*		thd,
	bool		rollback_trx) /*!< in: TRUE - rollback entire
					transaction FALSE - rollback the current
					statement only */
{
   int rc = 0 ;

   sdb_conn_auto_ptr connection ;

   if ( !rollback_trx )
   {
      goto done ;
   }

   rc = SDB_CONN_MGR_INST->get_sdb_conn( thd->thread_id(),
                                         connection ) ;
   //rc = SDB_CONN_MGR_INST->get_sdb_conn( (my_thread_id)(thd->active_vio->mysql_socket.fd),
   //                                      connection ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   rc = connection->rollback_transaction() ;

   if ( 0 != rc )
   {
      goto error ;
   }

done:
   //always return 0 because rollback will not be failed.
   return 0 ;
error:
   goto done ;
}

static int sdb_init_func(void *p)
{
   handlerton *sdb_hton ;
#ifdef HAVE_PSI_INTERFACE
   init_sdb_psi_keys();
#endif
   sdb_hton = (handlerton *)p ;
   mysql_mutex_init(key_mutex_sdb, &sdb_mutex, MY_MUTEX_INIT_FAST);
   (void) my_hash_init( &sdb_open_tables,system_charset_info,32,0,0,
                        (my_hash_get_key) sdb_get_key,0,0,
                        key_memory_sdb_share);
   sdb_hton->state= SHOW_OPTION_YES ;
   sdb_hton->db_type= DB_TYPE_UNKNOWN ;
   sdb_hton->create= sdb_create_handler ;
   sdb_hton->flags= ( HTON_SUPPORT_LOG_TABLES
                     | HTON_NO_PARTITION ) ;
   sdb_hton->commit = sdb_commit ;
   sdb_hton->rollback = sdb_rollback ;
   return SDB_CONF_INST->parse_conn_addrs( sdb_addr ) ;
}

static int sdb_done_func(void *p)
{
   //TODO************
   //SHOW_COMP_OPTION state;
   my_hash_free( &sdb_open_tables ) ;
   mysql_mutex_destroy(&sdb_mutex);
   return 0 ;
}

static int sdb_conn_addrs_validate( THD * thd,
                                    struct st_mysql_sys_var *var,
                                    void *save,
                                    struct st_mysql_value *value )
{
   const char * conn_addr_tmp = NULL ;
   char buff[ SDB_CONN_ADDR_SIZE_MAX ] = {0} ;
   int len = sizeof( buff ) ;
   assert( save != NULL ) ;
   assert( value != NULL ) ;
   conn_addr_tmp = value->val_str( value, buff, &len ) ;
   *static_cast<const char **>(save) = conn_addr_tmp ;

   return SDB_CONF_INST->parse_conn_addrs( conn_addr_tmp ) ;
}

static struct st_mysql_storage_engine sdb_storage_engine=
{ MYSQL_HANDLERTON_INTERFACE_VERSION };

static MYSQL_SYSVAR_STR( conn_addr, sdb_addr,
                           PLUGIN_VAR_STR|PLUGIN_VAR_MEMALLOC,
                           "Sequoiadb addr",
                           sdb_conn_addrs_validate,
                           NULL, SDB_ADDR_DFT ) ;
static struct st_mysql_sys_var *sdb_vars[]={
   MYSQL_SYSVAR(conn_addr),
   NULL
};

static char * get_sdb_plugin_info()
{
#define SDB_ENG_INFO          "SequoiaDB storage engine. "
   static char sdb_plugin_info[256] = SDB_ENG_INFO ;
   char *pPos = &sdb_plugin_info[strlen(SDB_ENG_INFO)] ;
   const char *pVersion = &sdb_ver_info[strlen(SDB_VER_INFO_NAME)] ;
   const char *pTmp = strchr( pVersion, '_' ) ;
   if ( pTmp != NULL )
   {
      strncpy( pPos, "Sequoiadb: ", strlen("Sequoiadb: ") ) ;
      pPos += strlen("Sequoiadb: ") ;
      strncpy( pPos, pVersion, pTmp - pVersion ) ;
      pPos += pTmp - pVersion ;
      strncpy( pPos, ", Plugin:", strlen(", Plugin:") ) ;
      pPos += strlen(", Plugin:") ;
      strncpy( pPos, pTmp + 1, strlen(pTmp+1) ) ;
      pPos[strlen(pTmp+1)] = 0 ;
   }
   return sdb_plugin_info ;
}

mysql_declare_plugin(sequoiadb)
{
  MYSQL_STORAGE_ENGINE_PLUGIN,
  &sdb_storage_engine,
  "SequoiaDB",
  "Jianhua Li, SequoiaDB",
  get_sdb_plugin_info(),
  PLUGIN_LICENSE_GPL,
  sdb_init_func, /* Plugin Init */
  sdb_done_func, /* Plugin Deinit */
  0x0100 /* 1.0 */,
  NULL,                       /* status variables                */
  sdb_vars,                   /* system variables                */
  NULL,                       /* config options                  */
  0,                          /* flags                           */
}
mysql_declare_plugin_end;
