#ifndef MYSQL_SERVER
   #define MYSQL_SERVER
#endif

#include "sdb_idx.h"

const char * sdb_get_idx_name( KEY * key_info )
{
   if ( key_info )
   {
      return key_info->name ;
   }
   return NULL ;
}

typedef union _sdb_key_common_type
{
   char     sz_data[8] ;
   int8     int8_val ;
   uint8    uint8_val ;
   int16    int16_val ;
   uint16   uint16_val ;
   int32    int24_val ; 
   uint32   uint24_val ;
   int32    int32_val ;
   uint32   uint32_val ;
   int64    int64_val ;
   uint64   uint64_val ;
}sdb_key_common_type ;

void get_unsigned_key_val( const uchar *key_ptr,
                           key_part_map key_part_map_val,
                           const KEY_PART_INFO *key_part,
                           bson::BSONObjBuilder &obj_builder,
                           const char *op_str )
{
   sdb_key_common_type val_tmp ;
   val_tmp.uint64_val = 0 ;
   if ( key_part->length > sizeof( val_tmp ) )
   {
      goto done ;
   }

   if( key_part_map_val & 1  && ( !key_part->null_bit || 0 == *key_ptr )) 
   { 
      memcpy( &(val_tmp.sz_data[0]),
              key_ptr+key_part->store_length-key_part->length, 
              key_part->length ); 
      switch( key_part->length )
      {
         case 1:
            {
               obj_builder.append( op_str, val_tmp.uint8_val ) ;
               break ;
            }
         case 2:
            {
               obj_builder.append( op_str, val_tmp.uint16_val ) ;
               break ;
            }
         case 3:
         case 4:
            {
               obj_builder.append( op_str, val_tmp.uint32_val ) ;
               break ;
            }
         case 8:
            {
               if( val_tmp.int64_val >= 0)
               {
                  obj_builder.append( op_str, val_tmp.int64_val ) ;
               }
               break ;
            }
         default:
            break ;
      }
   }

done:
   return ;
}

void get_unsigned_key_range_obj( const uchar *start_key_ptr,
                                 key_part_map start_key_part_map,
                                 enum ha_rkey_function start_find_flag,
                                 const uchar *end_key_ptr,
                                 key_part_map end_key_part_map,
                                 enum ha_rkey_function end_find_flag,
                                 const KEY_PART_INFO *key_part,
                                 bson::BSONObj &obj )
{
   bson::BSONObjBuilder obj_builder ;
   if ( HA_READ_KEY_EXACT == start_find_flag )
   {
      get_unsigned_key_val( start_key_ptr, start_key_part_map,
                            key_part, obj_builder, "$et" ) ;
   }
   else
   {
      get_unsigned_key_val( start_key_ptr, start_key_part_map,
                            key_part, obj_builder, "$gte" ) ;
      get_unsigned_key_val( end_key_ptr, end_key_part_map,
                            key_part, obj_builder, "$lte" ) ;
   }
   obj = obj_builder.obj() ;
}

void get_signed_key_val( const uchar *key_ptr,
                         key_part_map key_part_map_val,
                         const KEY_PART_INFO *key_part,
                         bson::BSONObjBuilder &obj_builder,
                         const char *op_str )
{
   sdb_key_common_type val_tmp ;
   val_tmp.uint64_val = 0 ;
   if ( key_part->length > sizeof( val_tmp ) )
   {
      goto done ;
   }

   if( key_part_map_val & 1  && ( !key_part->null_bit || 0 == *key_ptr )) 
   { 
      memcpy( &(val_tmp.sz_data[0]),
              key_ptr+key_part->store_length-key_part->length, 
              key_part->length ); 
      switch( key_part->length )
      {
         case 1:
            {
               obj_builder.append( op_str, val_tmp.int8_val ) ;
               break ;
            }
         case 2:
            {
               obj_builder.append( op_str, val_tmp.int16_val ) ;
               break ;
            }
         case 3:
            {
               if ( val_tmp.int32_val & 0X800000 )
               {
                  val_tmp.sz_data[3] = 0XFF ;
               }
               obj_builder.append( op_str, val_tmp.int32_val ) ;
               break ;
            }
         case 4:
            {
               obj_builder.append( op_str, val_tmp.int32_val ) ;
               break ;
            }
         case 8:
            {
                obj_builder.append( op_str, val_tmp.int64_val ) ;
               break ;
            }
         default:
            break ;
      }
   }

done:
   return ;
}

void get_signed_key_range_obj( const uchar *start_key_ptr,
                               key_part_map start_key_part_map,
                               enum ha_rkey_function start_find_flag,
                               const uchar *end_key_ptr,
                               key_part_map end_key_part_map,
                               enum ha_rkey_function end_find_flag,
                               const KEY_PART_INFO *key_part,
                               bson::BSONObj &obj )
{
   bson::BSONObjBuilder obj_builder ;
   if ( HA_READ_KEY_EXACT == start_find_flag )
   {
      get_signed_key_val( start_key_ptr, start_key_part_map,
                          key_part, obj_builder, "$et" ) ;
   }
   else
   {
      get_signed_key_val( start_key_ptr, start_key_part_map,
                          key_part, obj_builder, "$gte" ) ;
      get_signed_key_val( end_key_ptr, end_key_part_map,
                          key_part, obj_builder, "$lte" ) ;
   }
   obj = obj_builder.obj() ;
}

void get_text_key_val( const uchar *key_ptr,
                       key_part_map key_part_map_val,
                       const KEY_PART_INFO *key_part,
                       bson::BSONObjBuilder &obj_builder,
                       const char *op_str )
{
   if( key_part_map_val & 1  && ( !key_part->null_bit || 0 == *key_ptr )) 
   { 
      /*if ( key_part->length >= SDB_FIELD_MAX_LEN )
      {
         // bson is not support so long filed,
         // skip the condition
         goto done ;
      }*/

      // TODO: Do we need to process the scene: end with space
      /*if ( key_part->length > 0 && ' ' == str_field_buf[key_part->length-1] )
      {
         uint16 pos = key_part->length - 2 ;
         char tmp[] = "( ){0,}$" ;
         while( pos >= 0 && ' ' == str_field_buf[pos] )
         {
            --pos ;
         }
         ++pos ;
         if ( 'e' == op_str[1] ) // $et
         {
            strcpy( str_field_buf + pos + 1, tmp ) ;
            while( pos > 0 )
            {
               str_field_buf[pos] = str_field_buf[pos-1] ;
               --pos ;
            }
            str_field_buf[0] = '^' ;
            obj_builder.append( "$regex", str_field_buf ) ;
            goto done ;
         }
         else if ( 'g' == op_str[1] ) // $gte
         {
            str_field_buf[pos] = 0 ;
         }
      }*/
      obj_builder.appendStrWithNoTerminating( op_str,
                                              (const char*)key_ptr
                                                           + key_part->store_length
                                                           - key_part->length,
                                              key_part->length ) ;
   }
   return ;
}

void get_text_key_range_obj( const uchar *start_key_ptr,
                             key_part_map start_key_part_map,
                             enum ha_rkey_function start_find_flag,
                             const uchar *end_key_ptr,
                             key_part_map end_key_part_map,
                             enum ha_rkey_function end_find_flag,
                             const KEY_PART_INFO *key_part,
                             bson::BSONObj &obj )
{
   bson::BSONObjBuilder obj_builder ;
   if ( HA_READ_KEY_EXACT == start_find_flag )
   {
      get_text_key_val( start_key_ptr, start_key_part_map,
                        key_part, obj_builder, "$et" ) ;
   }
   else
   {
      get_text_key_val( start_key_ptr, start_key_part_map,
                        key_part, obj_builder, "$gte" ) ;
      get_text_key_val( end_key_ptr, end_key_part_map,
                        key_part, obj_builder, "$lte" ) ;
   }

   obj = obj_builder.obj() ;
}

void get_float_key_val( const uchar *key_ptr,
                        key_part_map key_part_map_val,
                        const KEY_PART_INFO *key_part,
                        bson::BSONObjBuilder &obj_builder,
                        const char *op_str )
{
   if( key_part_map_val & 1  && ( !key_part->null_bit || 0 == *key_ptr )) 
   { 
      if ( 4 == key_part->length )
      {
         float tmp = *((float *)(key_ptr+key_part->store_length-key_part->length)) ;
         obj_builder.append( op_str, tmp ) ;
      }
      else if ( 8 == key_part->length )
      {
         double tmp = *((double *)(key_ptr+key_part->store_length-key_part->length)) ;
         obj_builder.append( op_str, tmp ) ;
      }
   }
}

void get_float_key_range_obj( const uchar *start_key_ptr,
                              key_part_map start_key_part_map,
                              enum ha_rkey_function start_find_flag,
                              const uchar *end_key_ptr,
                              key_part_map end_key_part_map,
                              enum ha_rkey_function end_find_flag,
                              const KEY_PART_INFO *key_part,
                              bson::BSONObj &obj )
{
   bson::BSONObjBuilder obj_builder ;
   if ( HA_READ_KEY_EXACT == start_find_flag )
   {
      get_float_key_val( start_key_ptr, start_key_part_map,
                        key_part, obj_builder, "$et" ) ;
   }
   else
   {
      get_float_key_val( start_key_ptr, start_key_part_map,
                         key_part, obj_builder, "$gte" ) ;
      get_float_key_val( end_key_ptr, end_key_part_map,
                         key_part, obj_builder, "$lte" ) ;
   }

   obj = obj_builder.obj() ;
}

int build_match_obj_by_start_stop_key( uint keynr,
                                       const uchar *key_ptr,
                                       key_part_map keypart_map,
                                       enum ha_rkey_function find_flag,
                                       key_range *end_range,
                                       TABLE *table,
                                       bson::BSONObj &matchObj )
{
   int rc = 0 ;
   KEY *keyInfo ;
   const KEY_PART_INFO *keyPart ;
   const KEY_PART_INFO *keyEnd ;
   const uchar *startKeyPtr = key_ptr ;
   key_part_map startKeyPartMap = keypart_map ;
   const uchar *endKeyPtr = NULL ;
   key_part_map endKeyPartMap = 0 ;
   enum ha_rkey_function endFindFlag = HA_READ_INVALID ;
   bson::BSONObjBuilder objBuilder ;

   if ( MAX_KEY == keynr || table->s->keys <= 0 )
   {
      goto error ;
   }

   keyInfo = table->key_info + keynr ;
   if ( NULL == keyInfo || NULL == keyInfo->key_part )
   {
      goto done ;
   }

   if ( NULL != end_range )
   {
      endKeyPtr = end_range->key ;
      endKeyPartMap = end_range->keypart_map ;
      endFindFlag = end_range->flag ;
   }

   keyPart = keyInfo->key_part ;
   keyEnd = keyPart + keyInfo->user_defined_key_parts ;
   for( ; keyPart != keyEnd && (startKeyPartMap|endKeyPartMap);
        ++keyPart )
   {
      bson::BSONObj tmp_obj ;
      switch( keyPart->type )
      {
         case HA_KEYTYPE_SHORT_INT:
         case HA_KEYTYPE_LONG_INT:
         case HA_KEYTYPE_LONGLONG:
         case HA_KEYTYPE_INT8:
         case HA_KEYTYPE_INT24:
            {
               get_signed_key_range_obj( startKeyPtr,
                                         startKeyPartMap,
                                         find_flag,
                                         endKeyPtr,
                                         endKeyPartMap,
                                         endFindFlag,
                                         keyPart, tmp_obj ) ;
               break ;
            }
         case HA_KEYTYPE_USHORT_INT:
         case HA_KEYTYPE_ULONG_INT:
         case HA_KEYTYPE_ULONGLONG:
         case HA_KEYTYPE_UINT24:
            {
               get_unsigned_key_range_obj( startKeyPtr,
                                           startKeyPartMap,
                                           find_flag,
                                           endKeyPtr,
                                           endKeyPartMap,
                                           endFindFlag,
                                           keyPart, tmp_obj ) ;
               break ;
            }
         case HA_KEYTYPE_TEXT:
         case HA_KEYTYPE_VARTEXT1:
         case HA_KEYTYPE_VARTEXT2:
            {
               if ( !keyPart->field->binary() )
               {
                  get_text_key_range_obj( startKeyPtr,
                                          startKeyPartMap,
                                          find_flag,
                                          endKeyPtr,
                                          endKeyPartMap,
                                          endFindFlag,
                                          keyPart, tmp_obj ) ;
               }//TODO: process the binary
               break ;
            }
         case HA_KEYTYPE_FLOAT:
         case HA_KEYTYPE_DOUBLE:
            {
               get_float_key_range_obj( startKeyPtr,
                                        startKeyPartMap,
                                        find_flag,
                                        endKeyPtr,
                                        endKeyPartMap,
                                        endFindFlag,
                                        keyPart, tmp_obj ) ;
               break ;
            }
         case HA_KEYTYPE_NUM:
         default:
            rc = HA_ERR_UNSUPPORTED ;
            break ;
      }
      if ( !tmp_obj.isEmpty() )
      {
         objBuilder.append( keyPart->field->field_name,
                            tmp_obj ) ;
      }
      startKeyPtr += keyPart->store_length ;
      endKeyPtr += keyPart->store_length ;
      startKeyPartMap >>= 1 ;
      endKeyPartMap >>= 1 ;
   }
   matchObj = objBuilder.obj() ;

done:
   return rc ;
error:
   goto done ;
}