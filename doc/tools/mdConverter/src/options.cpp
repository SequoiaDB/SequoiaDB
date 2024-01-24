
#include <iostream>
#include <iomanip>
#include "options.hpp"

void options::setOptions( string key,
                          string defaultVal,
                          string desc )
{
   optionConf opt ;
   opt.key        = key ;
   opt.defaultVal = defaultVal ;
   opt.desc       = desc ;
   _optionList.push_back( opt ) ;
}

void options::_explode( string str )
{
   BOOLEAN isFirst = TRUE ;
   INT32 length = str.size() ;
   CHAR *pStr = strdup( str.c_str() ) ;
   CHAR chr = 0 ;

   while( TRUE )
   {
      if( length > 64 )
      {
         if( isFirst )
         {
            isFirst = FALSE ;
         }
         else
         {
            cout << setw(15) << " " ;
         }
         chr = pStr[64] ;
         pStr[64] = 0 ;
         cout << std::left << setw(64) << pStr << endl ;
         pStr[64] = chr ;
         pStr += 64 ;
         length -= 64 ;
      }
      else
      {
         if( !isFirst )
         {
            cout << setw(15) << " " ;
         }
         cout << std::left << setw(64) << pStr << endl ;
         break ;
      }
   }

}

void options::printHelp()
{
   optionConf opt ;
   vector<optionConf>::iterator iter ;

   for( iter = _optionList.begin(); iter != _optionList.end(); ++iter )
   {
      opt = *iter ;
      if( opt.key.size() == 0 )
      {
         cout << opt.desc << endl ;
      }
      else
      {
         cout << std::left << setw(10) << "  -" + opt.key ;
         cout << "     " ;
         _explode( opt.desc ) ;
      }
   }
}

INT32 options::parse( INT32 argc, CHAR *argv[] )
{
   INT32 rc = SDB_OK ;
   INT32 i = 1 ;
   optionConf opt ;
   vector<optionConf>::iterator iter ;

   if( argc > 1 )
   {
      for( i = 1; i < argc; ++i )
      {
         for( iter = _optionList.begin(); iter != _optionList.end(); ++iter )
         {
            opt = *iter ;
            if( opt.key.size() > 0 && strlen( argv[i] ) == 2 )
            {
               if( argv[i][0] == '-' && argv[i][1] == 'h' )
               {
                  opt.value = "help" ;
                  _options.push_back( opt ) ;
               }
               else if( argv[i][0] == '-' && argv[i][1] == opt.key.c_str()[0] )
               {
                  if( i + 1 < argc )
                  {
                     if( strlen( argv[i+1] ) != 2 || argv[i+1][0] != '-' )
                     {
                        opt.value = argv[i+1] ;
                        ++i ;
                     }
                     else
                     {
                        rc = SDB_INVALIDARG ;
                        goto error ;
                     }
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  _options.push_back( opt ) ;
                  break ;
               }
            }
         }
      }
   }
   for( iter = _optionList.begin(); iter != _optionList.end(); ++iter )
   {
      opt = *iter ;
      if( opt.key.size() > 0 && opt.defaultVal.size() > 0 )
      {
         _options.push_back( opt ) ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 options::getOption( string key, string &value, BOOLEAN *pIsDefault )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isFind = FALSE ;
   optionConf opt ;
   vector<optionConf>::iterator iter ;

   for( iter = _options.begin(); iter != _options.end(); ++iter )
   {
      opt = *iter ;
      if( key == opt.key )
      {
         if( opt.value.size() > 0 )
         {
            value = opt.value ;
            if( pIsDefault )
            {
               *pIsDefault = FALSE ;
            }
         }
         else if( opt.defaultVal.size() > 0 )
         {
            value = opt.defaultVal ;
            if( pIsDefault )
            {
               *pIsDefault = TRUE ;
            }
         }
         else
         {
            continue ;
         }
         isFind = TRUE ;
         break ;
      }
   }

   if( isFind == FALSE )
   {
      rc = SDB_UTIL_NOT_FIND_FIELD ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}
