#include "sdb_err_code.h"

void convert_sdb_code( int &rc )
{
   if ( rc < 0)
   {
      rc += SDB_ERR_INNER_CODE_END ;
   }
}

int get_sdb_code( int rc )
{
   if ( rc > SDB_ERR_INNER_CODE_BEGIN && rc < SDB_ERR_INNER_CODE_END )
   {
      return rc - SDB_ERR_INNER_CODE_END ;
   }
   return rc ;
}

