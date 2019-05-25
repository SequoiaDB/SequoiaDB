#include <my_base.h>
#include "sdb_cl.h"
#include "sdb_cl_ptr.h"
#include "sdb_conn.h"
#include "sdb_conn_ptr.h"
#include "sdb_err_code.h"
#include "sdb_adaptor.h"

using namespace sdbclient ;

sdb_cl::sdb_cl()
{
}

sdb_cl::~sdb_cl()
{
   //assert( cursor.pCursor == NULL ) ;
   //cursor.close() ;
   //cursor.pCursor = NULL ;
}

int sdb_cl::init( sdb_conn *connection,
                  char *cs, char *cl, bool create,
                  const bson::BSONObj &options )
{
   int rc = SDB_ERR_OK ;
   if ( NULL == connection || NULL == cs || NULL == cl )
   {
      rc = SDB_ERR_INVALID_ARG ;
      goto error ;
   }

   //cursor.pCursor = NULL ;

   p_conn = connection ;
   cs_name[CS_NAME_MAX_SIZE] = 0 ;
   strncpy( cs_name, cs, CS_NAME_MAX_SIZE ) ;
   if ( cs_name[CS_NAME_MAX_SIZE] != 0 )
   {
      rc = SDB_ERR_SIZE_OVF ;
      goto error ;
   }
   cl_name[CL_NAME_MAX_SIZE] = 0 ;
   strncpy( cl_name, cl, CL_NAME_MAX_SIZE ) ;
   if ( cl_name[CL_NAME_MAX_SIZE] != 0 )
   {
      rc = SDB_ERR_SIZE_OVF ;
      goto error ;
   }

   rc = re_init( create, options ) ;

done:
   return rc ;
error:
   goto done ;
}

int sdb_cl::re_init( bool create,
                     const bson::BSONObj &options )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
   sdbCollectionSpace cs ;

retry:
   rc = p_conn->get_sdb().getCollectionSpace( cs_name, cs ) ;
   if ( SDB_DMS_CS_NOTEXIST == rc && create )
   {
      rc = p_conn->get_sdb().createCollectionSpace( cs_name,
                                          4096, cs ) ;
   }
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }

   rc = cs.getCollection( cl_name, cl ) ;
   if ( SDB_DMS_NOTEXIST == rc && create )
   {
      rc = cs.createCollection( cl_name, options, cl ) ;
   }
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
   
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::check_connect( int rc )
{
   if ( SDB_NETWORK == rc || SDB_NOT_CONNECTED == rc )
   {
      return p_conn->connect() ;
   }
}

int sdb_cl::begin_transaction()
{
   return p_conn->begin_transaction() ;
}

int sdb_cl::commit_transaction()
{
   return p_conn->commit_transaction() ;
}

int sdb_cl::rollback_transaction()
{
   return p_conn->rollback_transaction() ;
}

bool sdb_cl::is_transaction()
{
   return p_conn->is_transaction() ;
}

char * sdb_cl::get_cs_name()
{
   return cs_name ;
}

char * sdb_cl::get_cl_name()
{
   return cl_name ;
}

int sdb_cl::query( const bson::BSONObj &condition,
                   const bson::BSONObj &selected,
                   const bson::BSONObj &orderBy,
                   const bson::BSONObj &hint,
                   INT64 numToSkip,
                   INT64 numToReturn,
                   INT32 flags )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.query( cursor, condition, selected, orderBy,
                  hint, numToSkip, numToReturn, flags ) ;
   if ( SDB_ERR_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::query_one( bson::BSONObj &obj,
                       const bson::BSONObj &condition,
                       const bson::BSONObj &selected,
                       const bson::BSONObj &orderBy,
                       const bson::BSONObj &hint,
                       INT64 numToSkip,
                       INT32 flags )
{
   int rc = SDB_ERR_OK ;
   sdbclient::sdbCursor cursor_tmp ;
   int retry_times = 2 ;
retry:
   rc = cl.query( cursor_tmp, condition, selected, orderBy,
                  hint, numToSkip, 1, flags ) ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }

   rc = cursor_tmp.next( obj ) ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }

done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::current( bson::BSONObj &obj )
{
   int rc = SDB_ERR_OK ;
   rc = cursor.current( obj ) ;
   if ( rc != SDB_ERR_OK )
   {
      if ( SDB_DMS_EOC == rc )
      {
         rc = HA_ERR_END_OF_FILE ;
      }
      goto error ;
   }

done:
   return rc ;
error:
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::next( bson::BSONObj & obj )
{
   int rc = SDB_ERR_OK ;
   rc = cursor.next( obj ) ;
   if ( rc != SDB_ERR_OK )
   {
      if ( SDB_DMS_EOC == rc )
      {
         rc = HA_ERR_END_OF_FILE ;
      }
      goto error ;
   }

done:
   return rc ;
error:
   /*if ( cursor.pCursor != NULL )
   {
      delete cursor.pCursor ;
      cursor.pCursor = NULL ;
   }*/
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::insert( bson::BSONObj &obj )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.insert( obj ) ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::update( const bson::BSONObj &rule,
                    const bson::BSONObj &condition,
                    const bson::BSONObj &hint,
                    INT32 flag )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.update( rule, condition, hint, flag ) ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::del( const bson::BSONObj &condition,
                 const bson::BSONObj &hint )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.del( condition, hint ) ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::create_index( const bson::BSONObj &indexDef,
                          const CHAR *pName,
                          BOOLEAN isUnique,
                          BOOLEAN isEnforced )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.createIndex( indexDef, pName, isUnique, isEnforced ) ;
   if ( SDB_IXM_REDEF == rc )
   {
      rc = SDB_ERR_OK ;
   }
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::drop_index( const char *pName )
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.dropIndex( pName ) ;
   if ( SDB_IXM_NOTEXIST == rc )
   {
      rc = SDB_ERR_OK ;
   }
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

int sdb_cl::truncate()
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.truncate() ;
   if ( rc != SDB_ERR_OK )
   {
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

void sdb_cl::close()
{
   cursor.close() ;
   //cursor.pCursor = NULL ;
}

my_thread_id sdb_cl::get_tid()
{
   return p_conn->get_tid() ;
}

int sdb_cl::drop()
{
   int rc = SDB_ERR_OK ;
   int retry_times = 2 ;
retry:
   rc = cl.drop() ;
   if ( rc != SDB_ERR_OK )
   {
      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = SDB_ERR_OK ;
         goto done ;
      }
      goto error ;
   }
done:
   return rc ;
error:
   if ( IS_SDB_NET_ERR(rc) )
   {
      bool is_transaction = p_conn->is_transaction() ;
      if( 0 == p_conn->connect() && !is_transaction
          && retry_times-- > 0 )
      {
         goto retry ;
      }
   }
   convert_sdb_code( rc ) ;
   goto done ;
}

sdb_cl_ref_ptr::sdb_cl_ref_ptr( sdb_cl *collection )
{
   DBUG_ASSERT( collection != NULL ) ;
   sdb_collection = collection ;
   ref = 1 ;
}

sdb_cl_ref_ptr::~sdb_cl_ref_ptr()
{
   if ( sdb_collection )
   {
      delete sdb_collection ;
      sdb_collection = NULL ;
   }
   ref = 0 ;
}


sdb_cl_auto_ptr::sdb_cl_auto_ptr()
:ref_ptr( NULL )
{
}

sdb_cl_auto_ptr::sdb_cl_auto_ptr( sdb_cl *collection )
{
   ref_ptr = new sdb_cl_ref_ptr( collection ) ;
}

sdb_cl_auto_ptr::sdb_cl_auto_ptr( const sdb_cl_auto_ptr &other )
{
   this->ref_ptr = other.ref_ptr ;
   if ( ref_ptr )
   {
      ref_ptr->ref.atomic_add(1) ;
   }
}

sdb_cl_auto_ptr::~sdb_cl_auto_ptr()
{
   clear() ;
}

void sdb_cl_auto_ptr::clear()
{
   int ref_tmp = 0 ;
   if ( NULL == ref_ptr )
   {
      goto done ;
   }

   // Note: ref_tmp is the old-value
   ref_tmp = ref_ptr->ref.atomic_add(-1) ;

   if ( 2 == ref_tmp )
   {
      // there is no table-handler use cl-instance,
      // only one in cl_list which in sdb_conn,
      // then delete it from cl_list.
      if ( NULL != ref_ptr->sdb_collection )
      {
         /*sdb_conn_auto_ptr tmp_conn = ref_ptr->sdb_collection->get_connection() ;
         tmp_conn->clear_cl( ref_ptr->sdb_collection->get_cs_name(),
                             ref_ptr->sdb_collection->get_cl_name() ) ;*/
         SDB_CONN_MGR_INST->del_sdb_conn( ref_ptr->sdb_collection->get_tid() ) ;
      }
   }
   else if ( 1 == ref_tmp )
   {
      delete ref_ptr ;
   }
   ref_ptr = NULL ;
done:
   return ;
}

sdb_cl_auto_ptr & sdb_cl_auto_ptr::operator = ( sdb_cl_auto_ptr &other )
{
   clear() ;
   this->ref_ptr = other.ref_ptr ;
   DBUG_ASSERT( ref_ptr != NULL ) ;
   DBUG_ASSERT( ref_ptr->sdb_collection != NULL ) ;
   ref_ptr->ref.atomic_add(1) ;
   return *this ;
}

sdb_cl & sdb_cl_auto_ptr::operator *()
{
   return *ref_ptr->sdb_collection ;
}

sdb_cl * sdb_cl_auto_ptr::operator ->()
{
   return ref_ptr->sdb_collection ;
}

int sdb_cl_auto_ptr::ref()
{
   if ( ref_ptr )
   {
      return ref_ptr->ref.atomic_get() ;
   }
   return 0 ;
}

