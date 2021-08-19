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

   Source File Name = sdbImport.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impOptions.hpp"
#include "impRoutine.hpp"
#include "impUtil.hpp"
#include "ossVer.h"
#include "pd.hpp"

using namespace import;

#define IMP_LOG_PATH "sdbimport.log"

int main(int argc, char* argv[])
{
   Options options;
   INT32 rc = SDB_OK;

   sdbEnablePD(IMP_LOG_PATH);
   setPDLevel(PDINFO);

   rc = options.parse(argc, argv);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to parse options, rc=%d", rc);
      goto error;
   }

   if (options.hasHelp())
   {
      options.printHelpInfo();
      goto done;
   }

   if (options.hasVersion())
   {
      ossPrintVersion("sdbimport");
      goto done;
   }

   if (options.hasHelpfull())
   {
      options.printHelpfullInfo();
      goto done;
   }

   try
   {
      Routine routine(options);

      rc = routine.run();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "routine running failure, rc=%d", rc);
      }

      routine.printStatistics();
   }
   catch(std::exception &e)
   {
      PD_LOG(PDERROR, "unexpected error happened:%s", e.what());
   }

done:
   return RC2ShellRC(rc);
error:
   goto done;
}

