#include "utilEnvCheck.hpp"
#include "pmd.hpp"

#include <iostream>

using namespace engine;
using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
   if(argc == 2)
   {
      //cout << "./envCheck.out logFilePath" << endl;
      sdbEnablePD(argv[1], 1000, 10) ;
   }
   if(argc == 1)
   {
      sdbEnablePD( pmdGetOptionCB()->getDiagLogPath(),
                   pmdGetOptionCB()->diagFileNum() ) ;
      setPDLevel( (PDLEVEL)( pmdGetOptionCB()->getDiagLevel() ) ) ;
   }
   else
   {
      cout << "wrong cmd format !" << endl ;
   }
   
  // sdbEnablePD(argv[1], 1, 100) ;
   //setPDLevel( (PDLEVEL)( pmdGetOptionCB()->getDiagLevel() ) );
   utilCheckEnv();
   return 0;
}
