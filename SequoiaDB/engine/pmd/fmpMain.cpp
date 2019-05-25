/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = fmpMain.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "fmpController.hpp"
#include <string>
#include "pmdOptions.h"
#include "pmdDef.hpp"
#include "utilParam.hpp"
#include "ossVer.h"

using namespace std ;

#define COMMANDS_OPTIONS \
    ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
    ( PMD_OPTION_VERSION, "version" )

void init ( po::options_description &desc )
{
   PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
      COMMANDS_OPTIONS
   PMD_ADD_PARAM_OPTIONS_END
}

void displayArg ( po::options_description &desc )
{
   std::cout << "Usage:  sdbfmp [OPTION]" <<std::endl;
   std::cout << desc << std::endl ;
}

INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   fmpController controller ;
   po::options_description desc ( "Command options" ) ;
   po::variables_map vm ;

   init ( desc ) ;

   rc = engine::utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Invalid arguments, rc: %d", rc ) ;
      displayArg ( desc ) ;
      goto done ;
   }
   if ( vm.count( PMD_OPTION_HELP ) )
   {
      displayArg( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
   if ( vm.count( PMD_OPTION_VERSION ) )
   {
      ossPrintVersion( "Sdb fmp version" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

   rc = controller.run() ;

done:
   return SDB_OK == rc ? 0 : 127 ;
}

