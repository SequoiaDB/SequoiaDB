/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sdbExport.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          28/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#include "expOptions.hpp"
#include "expCL.hpp"
#include "expExport.hpp"
#include "expUtil.hpp"
#include "ossVer.h"
#include "pd.hpp"
#include <iostream>

using namespace exprt ;
using namespace std ;

#define EXP_LOG_PATH "sdbexport.log"

int main( int argc, char* argv[] )
{
   INT32 rc = SDB_OK ;
   expOptions options ;

   sdbEnablePD(EXP_LOG_PATH);
   setPDLevel(PDINFO);

   try
   {
      expRoutine routine(options) ;

      rc = options.parseCmd( argc, argv ) ;
      if (SDB_OK != rc)
      {
         cerr << "failed to parse options" << endl ;
         PD_LOG(PDERROR, "Failed to parse cmd options, rc=%d", rc) ;
         goto error ;
      }

      if ( options.hasHelp() )
      {
         options.printHelpInfo() ;
         goto done ;
      }
      if ( options.hasVersion() )
      {
         ossPrintVersion("sdbexport") ;
         goto done ;
      }
      
      rc = routine.run() ;
      routine.printStatistics() ;
      if ( SDB_OK != rc )
      {
         PD_LOG(PDERROR, "Routine running failure, rc=%d", rc) ;
         goto error ;
      }
   }
   catch ( std::exception &e )
   {
      PD_LOG( PDERROR, "Unexpected error happened:%s", e.what() );
      goto error ;
   }

   cout << "done!" << endl ;
done:
   return RC2ShellRC(rc) ;
error:
   goto done;
}