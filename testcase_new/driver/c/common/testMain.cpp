/**************************************************************
 * @Description: main function 
 * @Modify     : Liang xuewang Init
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include "arguments.hpp"

using testing::InitGoogleTest ;

int main( int argc, char** argv )
{
   InitGoogleTest( &argc, argv ) ;
   
   ARGS->print() ;

   return RUN_ALL_TESTS() ;
}
