/**************************************************************
 * @Description: singleton to get arguments from console.
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#ifndef ARGUMENTS_HPP__
#define ARGUMENTS_HPP__

#define MAX_ARG_LEN 128

#include <client.hpp>

#define ARGS arguments::getInstance()

class arguments 
{
public:
   static arguments* getInstance() ;

   const CHAR* hostName() ;
   const CHAR* svcName() ;
   const INT32 port() ;
   const CHAR* user() ;
   const CHAR* passwd() ;

   const CHAR* coordUrl() ;

   const CHAR* changedPrefix() ;
   const CHAR* rsrvPortBegin() ;
   const CHAR* rsrvPortEnd() ;
   const CHAR* rsrvNodeDir() ;
   const CHAR* workDir() ;

   const CHAR* dsHostName() ;
   const CHAR* dsSvcName() ;
   const CHAR* dsCoordUrl() ;

   BOOLEAN forceClear() ;

   void print() ;
   
private:
   CHAR _hostName[ MAX_ARG_LEN ] ;
   CHAR _svcName[ MAX_ARG_LEN ] ;
   INT32 _port ;
   CHAR _user[ MAX_ARG_LEN ] ;
   CHAR _passwd[ MAX_ARG_LEN ] ;

   CHAR _coordUrl[ MAX_ARG_LEN ] ;

   CHAR _changedPrefix[ MAX_ARG_LEN ] ;
   CHAR _rsrvPortBegin[ MAX_ARG_LEN ] ;
   CHAR _rsrvPortEnd[ MAX_ARG_LEN ] ;
   CHAR _rsrvNodeDir[ MAX_ARG_LEN ] ;
   CHAR _workDir[ MAX_ARG_LEN ] ;

   CHAR _dsHostName[ MAX_ARG_LEN ] ;
   CHAR _dsSvcName[ MAX_ARG_LEN ] ;
   CHAR _dsCoordUrl[ MAX_ARG_LEN ] ;

   BOOLEAN _forceClear ;

   arguments() ;
   arguments( const arguments& ); 
   arguments& operator=( const arguments& );
} ;

#endif
