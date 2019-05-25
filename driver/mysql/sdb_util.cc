#include "sdb_util.h"
#include <string.h>

int sdb_parse_table_name( const char * from,
                          char *db_name, int db_name_size,
                          char *table_name, int table_name_size )
{
   int rc = 0, len = 0 ;
   const char *pBegin , *pEnd ;
   pBegin = from + 2 ; //skip "./"
   pEnd = strchr( pBegin, '/' ) ;
   if ( NULL == pEnd )
   {
      rc = -1 ;
      goto error ;
   }
   len = pEnd - pBegin ;
   if ( len >= db_name_size )
   {
      rc = -1 ;
      goto error ;
   }
   memcpy( db_name, pBegin, len ) ;
   db_name[len] = 0 ;

   pBegin = pEnd + 1 ;
   pEnd = strchrnul( pBegin, '/' ) ;
   len = pEnd - pBegin ;
   if ( *pEnd != 0 || len >= table_name_size )
   {
      rc = -1 ;
      goto error ;
   }
   memcpy( table_name, pBegin, len ) ;
   table_name[len] = 0 ;

done:
   return rc ;
error:
   goto done ;
}