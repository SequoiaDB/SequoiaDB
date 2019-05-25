#include "core.hpp"
#include "rcgen.h"
#include "filenamegen.h"
#include "optgen.h"
#include "tracegen.h"
#include "buildgen.h"
#include "dbConfForWeb.h"
#include "vergen.h"
#include <iostream>
using std::cout;
using std::endl;

void displayUsage (const char* argv)
{
    cout<<"Usage: "<<argv<<" <language>"<<endl
        <<"    <language>   desciption language as below ( default is english ):"<<endl
        <<"           en    english descriptions"<<endl
        <<"           cn    chinese desciptions"<<endl;
}

static void genRC ( const char *lang )
{
   RCGen xml ( lang ) ;
   xml.run () ;
}

static void genOpt ( const char *lang )
{
   OptGen xml ( lang ) ;
   xml.run () ;
}

static void genTrace ()
{
   TraceGen::genList () ;
}

static void genVer()
{
   VerGen verGen ;
   verGen.run(false) ;
}

enum supportedLangs
{
   LANG_CN = 0,
   LANG_EN,
   LANG_MAX
} ;

const CHAR *pLang[] = {
   "cn",
   "en"
} ;

static void genDoc ( const char *lang )
{
   RCGen rcGen ( lang ) ;
   rcGen.genDoc() ;

   OptGenForWeb optGen ( lang ) ;
   optGen.run () ;

   VerGen verGen ;
   verGen.run(true) ;

   for ( int i = 0; i < LANG_MAX; ++i )
   {
      RCGen xml ( pLang[i] ) ;
      xml.genWeb() ;  
   }
}

static void genBuild ()
{
   BuildGen gen ;
   gen.run () ;
}

static void genFileName ()
{
   FileNameGen::genList() ;
}

int main (int argc, char** argv)
{
   if ( ( 2 == argc && argv[1][0] == '?' ) ||
        ( 2 < argc ))
   {
      if ( argv[1][0] == '?' )
         displayUsage ( argv[0] ) ;
      return 0 ;
   }
   const char * lang = "en" ;
   if ( 2 == argc )
   {
      lang = argv[1] ;
   }
   genRC ( lang ) ;
   genFileName () ;
   genOpt ( lang ) ;
   genTrace () ;
   genDoc ( "cn" ) ;
   genBuild () ;
   genVer() ;
   return 0;
}
