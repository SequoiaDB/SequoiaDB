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
   
   utilCheckEnv();
   return 0;
}
