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

   Source File Name = sdbReplay.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplOptions.hpp"
#include "rplReplayer.hpp"
#include "ossCmdRunner.hpp"
#include "ossVer.h"
#include "pd.hpp"
#include "utilCommon.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace replay;

#define REPLAY_LOG_PATH "sdbreplay.log"

int main(int argc, char* argv[])
{
   Options options;
   INT32 rc = SDB_OK;

   sdbEnablePD(REPLAY_LOG_PATH);
   setPDLevel(PDINFO);

   rc = options.parse(argc, argv);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to parse arguments, rc=%d", rc);
      goto error;
   }

   if (options.hasHelp())
   {
      options.printHelp();
      goto done;
   }

   if (options.hasVersion())
   {
      ossPrintVersion("sdbreplay");
      goto done;
   }

   if (options.hasHelpfull())
   {
      options.printHelpfull();
      goto done;
   }

   if (options.debug())
   {
      setPDLevel(PDDEBUG);
   }

   if (options.daemon())
   {
      engine::ossCmdRunner runner;
      string cmd = options.buildBackgroundCmd(argc, argv);
      UINT32 exitCode = 0;

      if (options.debug())
      {
         std::cout << "start background process: " << cmd.c_str() << std::endl;
      }

      rc = runner.exec(cmd.c_str(), exitCode, TRUE, -1, FALSE, NULL, TRUE, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to run in background, rc=%d", rc);
         goto error;
      }

      if (options.debug())
      {
         std::cout << "background pid is " << runner.getPID() << std::endl;
      }

      goto done;
   }

   {
      string cmd = options.buildPrintableCmd(argc, argv);
#if defined (_LINUX)
      if (!options.password().empty())
      {
         ossEnableNameChanges(argc, argv);
         ossRenameProcess(cmd.c_str());
      }
#endif // _LINUX
      PD_LOG(PDEVENT, "command: %s", cmd.c_str());
   }

   try
   {
      Replayer replayer;

      rc = replayer.init(options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init replayer, rc=%d", rc);
         std::cerr << "Failed to init, see detail in sdbreplay.log"
                   << std::endl;
         goto error;
      }

      rc = replayer.run();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to run replayer, rc=%d", rc);
         if (SDB_INTERRUPT != rc)
         {
            std::cerr << "Failed to run, see detail in sdbreplay.log"
                      << std::endl;
         }
         goto error;
      }
   }
   catch(std::exception &e)
   {
      rc = SDB_SYS ;
      PD_LOG(PDERROR, "unexpected error happened:%s", e.what());
      std::cerr << "unexpected error happened: "
                << e.what()
                << std::endl;
   }

done:
   return engine::utilRC2ShellRC(rc);
error:
   goto done;
}


