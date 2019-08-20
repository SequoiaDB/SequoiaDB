
#ifndef MYSQL_SERVER
   #define MYSQL_SERVER
#endif

#include "include/client.hpp"
#include "sql_class.h"
#include "sdb_conn.h"
#include "sdb_conn_ptr.h"
#include "sdb_cl.h"
#include "sdb_cl_ptr.h"
#include "sdb_conf.h"
#include "sdb_util.h"
#include "sdb_err_code.h"
#include "sdb_adaptor.h"


sdb_conn::sdb_conn( my_thread_id _tid )
:transactionon(false),
tid(_tid)
{
   pthread_rwlock_init( &rw_mutex, NULL ) ;
}

sdb_conn::~sdb_conn()
{
   clear_all_cl() ;
   pthread_rwlock_destroy( &rw_mutex ) ;
}

sdbclient::sdb & sdb_conn::get_sdb()
{
   return connection ;
}

my_thread_id sdb_conn::get_tid()
{
   return tid ;
}

int sdb_conn::connect()
{
   int rc = 0 ;
   if ( !connection.isValid() )
   {
      std::map<std::string, sdb_cl_auto_ptr>::iterator iter ;
      transactionon = false ;
      //MYSQL_SECURITY_CONTEXT ctx = current_thd->security_context();
      rc = connection.connect( (const CHAR **)(SDB_CONF_INST->get_coord_addrs()),
                               SDB_CONF_INST->get_coord_num(),
                               "", "" ) ; 
      if ( rc != SDB_ERR_OK )
      {
         goto error ;
      }

      // get the r_lock because the cl_list will not change
      sdb_rw_lock_r r_lock( &rw_mutex ) ;
      iter = cl_list.begin() ;
      while ( iter != cl_list.end() )
      {
         iter->second->re_init() ;
         ++iter ;
      }
   }
done:
   return rc ;
error:
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_conn::begin_transaction()
{
   int rc = 0 ;
   int retry_times = 2 ;
   while ( !transactionon )
   {
      rc = connection.transactionBegin() ;
      if ( 0 == rc )
      {
         transactionon = true ;
         break ;
      }
      else if ( IS_SDB_NET_ERR( rc ) && --retry_times > 0 )
      {
         connect() ;
      }
      else
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_conn::commit_transaction()
{
   int rc = 0 ;
   if ( transactionon )
   {
      transactionon = false ;
      rc = connection.transactionCommit() ;
      if ( rc != SDB_ERR_OK )
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR( rc ) )
   {
      connect() ;
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_conn::rollback_transaction()
{
   if ( transactionon )
   {
      int rc = 0 ;
      transactionon = false ;
      rc = connection.transactionRollback() ;
      if ( IS_SDB_NET_ERR( rc ) )
      {
         connect() ;
      }
   }
   return 0 ;
}

bool sdb_conn::is_transaction()
{
   return transactionon ;
}

int sdb_conn::get_cl( char *cs_name, char *cl_name,
                      sdb_cl_auto_ptr &cl_ptr,
                      bool create,
                      const bson::BSONObj &options )
{
   int rc = SDB_ERR_OK ;
   std::map<std::string, sdb_cl_auto_ptr>::iterator iter ;
   cl_ptr.clear() ;
   std::string str_tmp( cs_name ) ;
   str_tmp.append( "." ) ;
   str_tmp.append( cl_name ) ;

   {
   //TODO: improve performence
   std::pair <std::multimap<std::string, sdb_cl_auto_ptr>::iterator,
              std::multimap<std::string, sdb_cl_auto_ptr>::iterator > ret ;
   sdb_rw_lock_r r_lock( &rw_mutex ) ;
   ret = cl_list.equal_range( str_tmp );
   iter = ret.first ;
   while( iter != ret.second )
   {
      if ( iter->second.ref() == 1 )
      {
         cl_ptr = iter->second ;
         if ( create )
         {
            rc = cl_ptr->init( this, cs_name, cl_name,
                               create, options ) ;
            if ( rc != SDB_ERR_OK )
            {
               goto error ;
            }
         }
         goto done ;
      }
      ++iter ;
   }
   }

   {
   sdb_cl_auto_ptr tmp_cl( new sdb_cl() ) ;
   rc = tmp_cl->init( this, cs_name, cl_name, create, options ) ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
   cl_ptr = tmp_cl ;
   sdb_rw_lock_w w_lock( &rw_mutex ) ;
   cl_list.insert(
      std::pair< std::string, sdb_cl_auto_ptr>( str_tmp, cl_ptr )) ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR( rc ) )
   {
      connect() ;
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_conn::create_cl( char *cs_name, char *cl_name,
                         sdb_cl_auto_ptr &cl_ptr,
                         const bson::BSONObj &options )
{
   int rc = 0 ;
   rc = this->get_cl( cs_name, cl_name, cl_ptr, TRUE, options ) ;
   if( rc != 0 )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR( rc ) )
   {
      connect() ;
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

void sdb_conn::clear_cl( char *cs_name, char *cl_name )
{
   std::map<std::string, sdb_cl_auto_ptr>::iterator iter ;
   std::string str_tmp( cs_name ) ;
   str_tmp.append( "." ) ;
   str_tmp.append( cl_name ) ;

   {
   sdb_rw_lock_w w_lock( &rw_mutex ) ;
   std::pair <std::multimap<std::string, sdb_cl_auto_ptr>::iterator,
              std::multimap<std::string, sdb_cl_auto_ptr>::iterator > ret ;
   ret = cl_list.equal_range( str_tmp );
   iter = ret.first ;
   while( iter != ret.second )
   {
      if ( iter->second.ref() <= 1 )
      {
         cl_list.erase( iter++ ) ;
         continue ;
      }
      ++iter ;
   }
   }
done:
   return ;
}

void sdb_conn::clear_all_cl()
{
   int rc = SDB_ERR_OK ;
   std::map<std::string, sdb_cl_auto_ptr>::iterator iter ;
   sdb_rw_lock_w w_lock( &rw_mutex ) ;
   iter = cl_list.begin() ;
   while( iter != cl_list.end() )
   {
      assert( iter->second.ref() == 1 ) ;
      cl_list.erase( iter++ ) ;
   }
done:
   return ;
}

bool sdb_conn::is_idle()
{
   std::map<std::string, sdb_cl_auto_ptr>::iterator iter ;
   sdb_rw_lock_r r_lock( &rw_mutex ) ;
   iter = cl_list.begin() ;
   while ( iter != cl_list.end() )
   {
      if ( iter->second.ref() > 1 )
      {
         return FALSE ;
      }
      ++iter ;
   }
   return TRUE ;
}

sdb_conn_ref_ptr::sdb_conn_ref_ptr( sdb_conn * connection )
{
   DBUG_ASSERT( connection != NULL ) ;
   sdb_connection = connection ;
   ref = 1 ;
}

sdb_conn_ref_ptr::~sdb_conn_ref_ptr()
{
   if ( sdb_connection )
   {
      delete sdb_connection ;
      sdb_connection = NULL ;
   }
   ref = 0 ;
}


sdb_conn_auto_ptr::sdb_conn_auto_ptr()
:ref_ptr( NULL )
{
}

sdb_conn_auto_ptr::sdb_conn_auto_ptr( sdb_conn *connection )
{
   ref_ptr = new sdb_conn_ref_ptr( connection ) ;
}

sdb_conn_auto_ptr::sdb_conn_auto_ptr( const sdb_conn_auto_ptr &other )
{
   this->ref_ptr = other.ref_ptr ;
   if ( ref_ptr )
   {
      ref_ptr->ref.atomic_add(1) ;
   }
}

sdb_conn_auto_ptr::~sdb_conn_auto_ptr()
{
   clear() ;
}

void sdb_conn_auto_ptr::clear()
{
   int ref_tmp = 0 ;
   if ( NULL == ref_ptr )
   {
      goto done ;
   }

   // Note: ref_tmp is the old-value
   ref_tmp = ref_ptr->ref.atomic_add(-1) ;

   if ( 1 == ref_tmp )
   {
      delete ref_ptr ;
   }
   else if ( 2 == ref_tmp )
   {
      // there is no table-handler use sdb-instance,
      // only one in conn_list which in sdb_conn_mgr,
      // then delete it from conn_list.
      if ( NULL != ref_ptr->sdb_connection )
      {
         SDB_CONN_MGR_INST->del_sdb_conn( ref_ptr->sdb_connection->get_tid() ) ;
      }
   }

   ref_ptr = NULL ;
done:
   return ;
}

sdb_conn_auto_ptr & sdb_conn_auto_ptr::operator = ( sdb_conn_auto_ptr &other )
{
   clear() ;
   this->ref_ptr = other.ref_ptr ;
   DBUG_ASSERT( ref_ptr != NULL ) ;
   DBUG_ASSERT( ref_ptr->sdb_connection != NULL ) ;
   ref_ptr->ref.atomic_add(1) ;
   return *this ;
}

sdb_conn & sdb_conn_auto_ptr::operator *()
{
   return *ref_ptr->sdb_connection ;
}

sdb_conn * sdb_conn_auto_ptr::operator ->()
{
   return ref_ptr->sdb_connection ;
}

int sdb_conn_auto_ptr::ref()
{
   if ( ref_ptr )
   {
      return ref_ptr->ref.atomic_get() ;
   }
   return 0 ;
}
