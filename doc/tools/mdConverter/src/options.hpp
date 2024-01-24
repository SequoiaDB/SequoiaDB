
#ifndef OPTIONS_HPP__
#define OPTIONS_HPP__

#include <string>

#ifdef linux
#include <string.h>
#endif

#include <vector>
#include "core.h"
using namespace std ;

typedef struct{
   string key ;
   string value ;
   string defaultVal ;
   string desc ;
} optionConf ;

class options
{
private:
   vector<optionConf> _optionList ;
   vector<optionConf> _options ;
private:
   void _explode( string str ) ;
public:
   void setOptions( string key,
                    string defaultVal,
                    string desc ) ;
   void printHelp() ;
   INT32 parse( INT32 argc, CHAR *argv[] ) ;
   INT32 getOption( string key, string &value, BOOLEAN *pIsDefault = NULL ) ;
   
} ;

#endif