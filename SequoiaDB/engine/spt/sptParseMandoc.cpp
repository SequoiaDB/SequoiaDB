#include <string.h>
#include "core.h"
#include "ossErr.h"
#include "sptParseMandoc.hpp"
SDB_EXTERN_C_START
#include "parseMandoc.h"
SDB_EXTERN_C_END

_sptParseMandoc& _sptParseMandoc::getInstance()
{
   static _sptParseMandoc _instance ;
   return _instance ;
}

INT32 _sptParseMandoc::parse(const CHAR* filename)
{
   INT32 rc = SDB_OK ;
#if defined _WIN32
   const CHAR *argv[6] = { "sdb", "-K", "utf-8", "-T", "locale", filename } ;
#else
   const CHAR *argv[6] = { "sdb", "-K", "utf-8", "-T", "utf8", filename } ;
#endif
   rc = parse_mandoc(6, (const CHAR **)argv) ;
   if ( rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

