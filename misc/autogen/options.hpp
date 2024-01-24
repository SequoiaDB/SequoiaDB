#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <iostream>
using namespace std ;

class cmdOptions
{
public:
   cmdOptions() ;
   ~cmdOptions() ;

   int parse( int argc, char *argv[] ) ;

   int getLevel(){ return _level ; }
   bool getForce(){ return _force ; }
   string &getLang(){ return _lang ; }

private:
   /*
      0: SERVER
      1: ERROR
      2: WARNING
      3: INFO
   */
   int      _level ;
   bool     _force ;
   string   _lang ;
} ;

cmdOptions *getCmdOptions() ;


#endif