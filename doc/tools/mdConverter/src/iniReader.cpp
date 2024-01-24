
#include "iniReader.hpp"
#include "inih/ini.h"

static INT32 handler( void *pCtrl, const CHAR *pSection,
                      const CHAR *pKey, const CHAR *pValue )
{
   map<string, string> *pConfig = (map<string, string>*)pCtrl ;
   if( strcmp( pSection, "config" ) == 0 )
   {
      pConfig->insert( map<string, string>::value_type( pKey, pValue ) ) ;
   }
   else
   {
      return 0 ;
   }
   return 1 ;
}

INT32 iniReader::init( string configName )
{
   INT32 rc = SDB_OK ;
   rc = ini_parse( configName.c_str(), handler, &_config ) ;
   if( rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 iniReader::get( string key, string &value )
{
   INT32 rc = SDB_OK ;
   map<string, string>::iterator item ;
   item = _config.find( key ) ;
   if( item == _config.end() )
   {
      rc = SDB_UTIL_NOT_FIND_FIELD ;
      goto error ;
   }
   value = _config[key] ;
done:
   return rc ;
error:
   goto done ;
}
