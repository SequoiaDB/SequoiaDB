/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = pmdSignalHandler.cpp

   Descriptive Name = Process MoDel Signal Handler

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains function to handle signals.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/04/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdSignalHandler.hpp"

namespace engine
{

   pmdSignalInfo& pmdGetSignalInfo( INT32 signum )
   {
      static pmdSignalInfo s_signalHandleMap [] = {
         { "Unknow", 0 },
         { "SIGHUP", 1 },     //1
         { "SIGINT", 1 },     //2
         { "SIGQUIT", 1 },    //3
         { "SIGILL", 1 },     //4
         { "SIGTRAP", 1 },    //5
         { "SIGABRT", 1 },    //6
         { "SIGBUS", 1 },     //7
         { "SIGFPE", 1 },     //8
         { "SIGKILL", 1 },    //9
         { "SIGUSR1", 0 },    //10     ---for dump the thread stack and memory info
         { "SIGSEGV", 1 },    //11
         { "SIGUSR2", 0 },    //12
         { "SIGPIPE", 0 },    //13
         { "SIGALRM", 0 },    //14
         { "SIGTERM", 1 },    //15
         { "SIGSTKFLT", 0 },  //16
         { "SIGCHLD", 0 },    //17
         { "SIGCONT", 0 },    //18
         { "SIGSTOP", 1 },    //19
         { "SIGTSTP", 0 },    //20
         { "SIGTTIN", 0 },    //21
         { "SIGTTOU", 0 },    //22
         { "SIGURG", 0 },     //23     ---for notify all thread the singal 10
         { "SIGXCPU", 0 },    //24
         { "SIGXFSZ", 0 },    //25
         { "SIGVTALRM", 0 },  //26
         { "SIGPROF", 0 },    //27
         { "SIGWINCH", 0 },   //28
         { "SIGIO", 0 },      //29
         { "SIGPWR", 1 },     //30
         { "SIGSYS", 1 },     //31
         { "UNKNOW", 0 },     //32
         { "UNKNOW", 0 },     //33
         { "SIGRTMIN", 0 },   //34
         { "SIGRTMIN+1", 0 }, //35     ---for notify all thread the signal 36
         { "SIGRTMIN+2", 0 }, //36     ---do nothing for self thread
         { "SIGRTMIN+3", 0 }, //37     ---for notify all thread the signal 38
         { "SIGRTMIN+4", 0 }, //38     ---freeze or resume thread
         { "SIGTTMIN+5", 0 }, //39     ---for notify all thread the signal 40
         { "SIGRTMIN+6", 0 }, //40     ---dump the memory info
         { "SIGRTMIN+7", 0 }, //41     ---do memory trim and notify all thread the signal 42
         { "SIGTTMIN+8", 0 }, //42     ---do thread memory trim
         { "SIGRTMIN+9", 0 }, //43
         { "SIGRTMIN+10", 0 },//44
         { "SIGRTMIN+11", 0 },//45
         { "SIGRTMIN+12", 0 },//46
         { "SIGRTMIN+13", 0 },//47
         { "SIGRTMIN+14", 0 },//48
         { "SIGRTMIN+15", 0 },//49
         { "SIGRTMAX-14", 0 },//50
         { "SIGRTMAX-13", 0 },//51
         { "SIGRTMAX-12", 0 },//52
         { "SIGRTMAX-11", 0 },//53
         { "SIGRTMAX-10", 0 },//54
         { "SIGRTMAX-9", 0 }, //55
         { "SIGRTMAX-8", 0 }, //56
         { "SIGRTMAX-7", 0 }, //57
         { "SIGRTMAX-6", 0 }, //58
         { "SIGRTMAX-5", 0 }, //59
         { "SIGRTMAX-4", 0 }, //60
         { "SIGRTMAX-3", 0 }, //61
         { "SIGRTMAX-2", 0 }, //62
         { "SIGRTMAX-1", 0 }, //63
         { "SIGRTMAX", 0 },   //64
      } ;

      if ( signum > 0 &&
           (UINT32)signum < sizeof(s_signalHandleMap)/sizeof(pmdSignalInfo) )
      {
         return s_signalHandleMap[ signum ] ;
      }
      return s_signalHandleMap[ 0 ] ;
   }

}

