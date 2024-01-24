#include "options.hpp"
#include "generateInterface.hpp"

int main( int argc, char** argv )
{
   int rc = 0 ;
   cmdOptions *opt = getCmdOptions() ;
   generateRunner *runner = getGenerateRunner() ;

   rc = opt->parse( argc, argv ) ;
   if ( -1 == rc )
   {
      rc = 0 ;
      goto done ;
   }
   else if ( rc )
   {
      goto error ;
   }

   rc = runner->init() ;
   if ( rc )
   {
      goto error ;
   }

   rc = runner->run() ;
   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}
