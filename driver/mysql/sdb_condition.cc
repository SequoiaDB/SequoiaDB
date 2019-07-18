
#ifndef MYSQL_SERVER
   #define MYSQL_SERVER
#endif

#include "sdb_condition.h"
#include "sdb_err_code.h"

sdb_cond_ctx::sdb_cond_ctx()
{
   cur_item = NULL ;
   status = sdb_cond_supported ;
}

sdb_cond_ctx::~sdb_cond_ctx()
{
   clear() ;
}

void sdb_cond_ctx::clear()
{
   sdb_item *item_tmp = NULL ;

   if ( cur_item != NULL )
   {
      delete cur_item ;
      cur_item = NULL ;
   }

   while( (item_tmp = item_list.pop()) != NULL )
   {
      delete item_tmp ;
   }
}

void sdb_cond_ctx::pop_all()
{
   sdb_item *item_tmp = NULL ;

   if ( sdb_cond_unsupported == status )
   {
      clear() ;
      goto done ;
   }

   while ( !item_list.is_empty() )
   {
      if ( NULL == cur_item )
      {
         cur_item = item_list.pop() ;
         continue ;
      }

      if ( !cur_item->finished() )
      {
         if ( Item_func::COND_AND_FUNC == cur_item->type() )
         {
            cur_item->push( (Item *)NULL ) ;
         }
         else
         {
            delete cur_item ;
            cur_item = NULL ;
            update_stat( SDB_ERR_COND_INCOMPLETED ) ;
            continue ;
         }
      }

      item_tmp = cur_item ;
      cur_item = item_list.pop() ;
      if ( 0 != cur_item->push( item_tmp ) )
      {
         delete item_tmp ;
      }
   }

done:
   return ;
}

void sdb_cond_ctx::pop()
{
   int rc = SDB_ERR_OK ;
   sdb_item *item_tmp = NULL ;

   if ( !keep_on() || item_list.is_empty() )
   {
      goto done ;
   }

   while( !item_list.is_empty() )
   {
      if ( NULL == cur_item )
      {
         cur_item = item_list.pop() ;
         continue ;
      }

      item_tmp = cur_item ;
      cur_item = item_list.pop() ;
      rc = cur_item->push( item_tmp ) ;
      if ( 0 != rc )
      {
         delete item_tmp ;
         goto error ;
      }

      if ( cur_item->finished() )
      {
         pop() ;
      }
      break ;
   }

done:
   return ;
error:
   update_stat( rc ) ;
   goto done ;
}

void sdb_cond_ctx::update_stat( int rc )
{
   if ( SDB_ERR_OK == rc )
   {
      goto done ;
   }

   if ( sdb_cond_unsupported == status
      || sdb_cond_beforesupported == status )
   {
      goto done ;
   }

   if ( NULL == cur_item
        || Item_func::COND_OR_FUNC == cur_item->type() )
   {
      status = sdb_cond_unsupported ;
      goto done ;
   }

   if ( Item_func::COND_AND_FUNC == cur_item->type()
        && SDB_ERR_COND_PART_UNSUPPORTED == rc )
   {
      status = sdb_cond_partsupported ;
   }
   else
   {
      status = sdb_cond_beforesupported ;
   }

done:
   return ;
}

bool sdb_cond_ctx::keep_on()
{
   if ( sdb_cond_unsupported == status
      || sdb_cond_beforesupported == status )
   {
      return FALSE ;
   }
   return true ;
}

void sdb_cond_ctx::push( Item *cond_item )
{
   int rc = SDB_ERR_OK ;
   sdb_item *item_tmp = NULL ;
   if ( !keep_on() )
   {
      goto done ;
   }

   if ( NULL != cur_item )
   {
/*      if ( NULL == cond_item
           || ( Item::COND_ITEM != cond_item->type()
                && ( Item::FUNC_ITEM != cond_item->type()
                     || cond_item->const_item() )))*/
      if ( NULL == cond_item
           || ( Item::FUNC_ITEM != cond_item->type()
                && Item::COND_ITEM != cond_item->type()) )
      {
         rc = cur_item->push( cond_item ) ;
         if ( 0 != rc )
         {
            goto error ;
         }

         if ( cur_item->finished() )
         {
            // finish the current item
            pop() ;
            if ( !keep_on() )
            {
               // Occur unsupported scene while finish the current item.
               // the status will be set in pop(), so keep rc=0 and skip update_stat()
               goto error ;
            }
         }

         /*//func_item should go on to skip the param by create unkonw_func_item
         if ( NULL == cond_item || Item::FUNC_ITEM != cond_item->type() )
         {
            goto done ;
         }*/
         goto done ;
      }
   }
         
   item_tmp = create_sdb_item( (Item_func *)cond_item ) ;
   if ( NULL == item_tmp )
   {
      rc = SDB_ERR_COND_UNSUPPORTED ;
      goto error ;
   }
   if ( cur_item != NULL )
   {
      item_list.push_front( cur_item ) ; // netsted func
   }
   cur_item = item_tmp ;

done:
   return ;
error:
   update_stat( rc ) ;
   goto done ;
}

sdb_item *sdb_cond_ctx::create_sdb_item( Item_func *cond_item )
{
   sdb_item *item = NULL ;
   switch( cond_item->functype() )
   {
      case Item_func::COND_AND_FUNC:
         {
            item = new sdb_and_item() ;
            break ;
         }
      case Item_func::COND_OR_FUNC:
         {
            item = new sdb_or_item() ;
            break ;
         }
      case Item_func::EQ_FUNC:
         {
            item = new sdb_func_eq() ;
            break ;
         }
      case Item_func::NE_FUNC:
         {
            item = new sdb_func_ne() ;
            break ;
         }
      case Item_func::LT_FUNC:
         {
            item = new sdb_func_lt() ;
            break ;
         }
      case Item_func::LE_FUNC:
         {
            item = new sdb_func_le() ;
            break ;
         }
      case Item_func::GT_FUNC:
         {
            item = new sdb_func_gt() ;
            break ;
         }
      case Item_func::GE_FUNC:
         {
            item = new sdb_func_ge() ;
            break ;
         }
      case Item_func::BETWEEN:
         {
            item = new sdb_func_between( 
                        ((Item_func_between *)cond_item)->negated ) ;
            break ;
         }
      case Item_func::ISNULL_FUNC:
         {
            item = new sdb_func_isnull() ;
            break ;
         }
      case Item_func::ISNOTNULL_FUNC:
         {
            item = new sdb_func_isnotnull() ;
            break ;
         }
      case Item_func::IN_FUNC:
         {
            Item_func_in *item_func = (Item_func_in *)cond_item ;
            item = new sdb_func_in( item_func->negated,
                                    item_func->arg_count ) ;
            break ;
         }
      default:
         {
            item = new sdb_func_unkown( cond_item ) ;
            //update_stat( SDB_ERR_COND_PART_UNSUPPORTED ) ;
            break ;
         }
   }
   return item ;
}

int sdb_cond_ctx::to_bson( bson::BSONObj &obj )
{
   static bson::BSONObj empty_obj ;
   int rc = 0 ;
   if ( NULL != cur_item )
   {
      rc = cur_item->to_bson( obj ) ;
      if ( 0 == rc )
      {
         goto done ;
      }
      update_stat( rc ) ;
   }
   obj = empty_obj ;

done:
   return rc ;
}

static void sdb_traverse_cond( const Item *cond_item, void *arg )
{
   sdb_cond_ctx *sdb_ctx = (sdb_cond_ctx *)arg ;

   if ( sdb_cond_unsupported == sdb_ctx->status
      || sdb_cond_beforesupported == sdb_ctx->status )
   {
      // skip all while occured unsupported-condition
      goto done ;
   }

   sdb_ctx->push( (Item *)cond_item ) ;
done:
   return ;
}

void sdb_parse_condtion( const Item *cond_item, sdb_cond_ctx *sdb_ctx )
{
   ((Item *)cond_item)->traverse_cond( &sdb_traverse_cond,
                                       (void *)sdb_ctx,
                                       Item::PREFIX ) ;
   sdb_ctx->pop_all() ;
}