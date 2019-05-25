#include <string.h>
#include "my_global.h"
#include "sdb_conf.h"

PSI_memory_key sdb_key_memory_conf_coord_addrs ;

sdb_conf::sdb_conf()
{
}

sdb_conf::~sdb_conf()
{
   clear_coord_addrs( coord_num ) ;
   coord_num = 0 ;
}

sdb_conf *sdb_conf::get_instance()
{
   static sdb_conf sdb_conf_inst ;
   return &sdb_conf_inst ;
}

void sdb_conf::clear_coord_addrs( int num )
{
   for( int i = 0 ; i < num ; i++ )
   {
      if ( pAddrs[i] )
      {
         free( pAddrs[i] ) ;
      }
   }
}

int sdb_conf::parse_conn_addrs( const char *conn_addr )
{
   int rc = 0 ;
   int num = 0 ;
   const char *p = conn_addr ;

   if ( NULL == conn_addr )
   {
      rc = -1 ;
      goto error ;
   }

   // note: no lock here, the coord-addrs must set before used!
   //sdb_rw_lock_w lock( &addrs_mutex ) ;
   num = coord_num ;
   coord_num = 0 ;
   clear_coord_addrs( num ) ;

   while ( *p != 0 )
   {
      const char *pTmp = NULL ;
      size_t len = 0 ;
      if ( coord_num >= SDB_COORD_NUM_MAX )
      {
         goto done ;
      }

      pTmp = strchr(p, ',') ;
      if ( NULL == pTmp )
      {
         len = strlen( p ) ;
      }
      else
      {
         len = pTmp - p ;
      }
      if( len > 0 )
      {
         //pAddrs[coord_num] = (char *)my_malloc( sdb_key_memory_conf_coord_addrs,
         //                                       len+1, MYF(MY_WME) ) ;
         pAddrs[coord_num] = (char *)malloc( len+1 ) ;
         if ( NULL == pAddrs[coord_num] )
         {
            rc = -1 ;
            goto error ;
         }
         memcpy(pAddrs[coord_num], p, len ) ;
         pAddrs[coord_num][len] = 0 ;
         ++coord_num ;
      }
      p += len;
      if ( *p == ',' )
      {
         p++ ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

char **sdb_conf::get_coord_addrs()
{
   return pAddrs ;
}

int sdb_conf::get_coord_num()
{
   return coord_num ;
}