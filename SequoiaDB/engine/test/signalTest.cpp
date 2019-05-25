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

*******************************************************************************/

#include "core.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossSignal.hpp"
#include "ossStackDump.hpp"
#include "ossUtil.hpp"
#include <stdio.h>
#if defined (_LINUX)
void dummyCore()
{
  OSS_INSTRUCTION_PTR ppAddress[100] ;
  char funcName[256] ;
  UINT32_64 offset = 0 ;
  CHAR *p = NULL ;

  ossWalkStack( 0, ppAddress, 20 ) ;
  for ( int i = 0 ; i < 20 ; i++ )
  {
      ossGetSymbolNameFromAddress( ppAddress[i],
                                   funcName,
                                   sizeof( funcName ),
                                   &offset ) ;
      printf("[%3d] 0x%16p  %s  + 0x%llx\n",
             i, (void*)ppAddress[i], funcName, (UINT64)offset ) ;
  }
  getchar();
  *p = 10 ;
}


void dummyf5()
{
  dummyCore() ;
}

int dummyf4()
{
   int y=1 ;
   dummyf5() ;
   return  y ;
}

void dummyf3()
{
   dummyf4() ;
}

void dummyf2()
{
   dummyf3() ;
}

void dummyf1()
{
   dummyf2() ;
}

inline void baz()
{
  dummyf1() ;
}

inline void bar()
{
  baz() ;
}

void foo( )
{
  bar() ;
}

void myHdl( OSS_HANDPARMS )
{
   ossPrimitiveFileOp trapFile ;

   trapFile.Open( "/home/taoewang/repos/trap.txt" ) ;
   ossDumpStackTrace( OSS_HANDARGS, &trapFile ) ;
   trapFile.Close() ;

   ossRestoreSystemSignal( signum, false, "/home/taoewang/repos" ) ;
   return ;
}
#endif
int main()
{
   int rc = 0 ;
#if defined (_LINUX)
   struct sigaction newact ;

   sigemptyset (&newact.sa_mask) ;
   newact.sa_sigaction = ( OSS_SIGFUNCPTR ) myHdl ;
   newact.sa_flags |= SA_SIGINFO ;
   newact.sa_flags |= SA_ONSTACK ; ;

   sigaction (SIGSEGV, &newact, NULL) ;

   foo() ;
#endif
   return rc ;

}
