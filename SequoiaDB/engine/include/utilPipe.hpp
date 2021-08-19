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

   Source File Name = utilPipe.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

// this file is only intened to be included by sdb.cpp and sdbbp.cpp
#ifndef UTILPIPE_HPP__
#define UTILPIPE_HPP__

#include "core.h"


#define SH_VERIFY_RC                                             \
   if ( rc != SDB_OK ) {                                          \
      PD_LOG ( PDERROR , "%s, rc = %d" , getErrDesp(rc) , rc ) ;  \
      goto error ;                                                \
   }

#define SH_VERIFY_COND(cond, ret)  \
   if (!(cond)) {                   \
      rc=ret;                       \
      SH_VERIFY_RC                 \
   }

#define SDB_SHELL_WAIT_PIPE_PREFIX        "sdb-shell-wait-"
#define SDB_SHELL_F2B_PIPE_PREFIX         "sdb-shell-f2b-"
#define SDB_SHELL_B2F_PIPE_PREFIX         "sdb-shell-b2f-"
#define SDB_SHELL_CTL_F2B_PIPE_PREFIX     "sdb-shell-ctl-f2b-"
#define SDB_SHELL_CTL_B2F_PIPE_PREFIX     "sdb-shell-ctl-b2f-"

INT32 getWaitPipeName ( const OSSPID & ppid , CHAR * buf , UINT32 bufSize ) ;

INT32 getPipeNames( const OSSPID &ppid,
                    CHAR *f2bName, UINT32 f2bSize,
                    CHAR *b2fName, UINT32 b2fSize,
                    CHAR *f2bCtlName, UINT32 f2bCtlSize,
                    CHAR *b2fCtlName, UINT32 b2fCtlSize ) ;
INT32 getPipeNames2( const OSSPID &ppid, const OSSPID &pid,
                     CHAR *f2bName, UINT32 f2bSize,
                     CHAR *b2fName, UINT32 b2fSize,
                     CHAR *f2bCtlName, UINT32 f2bCtlSize,
                     CHAR *b2fCtlName, UINT32 b2fCtlSize ) ;

INT32 getPipeNames1( CHAR *bpf2bName, UINT32 bpf2bSize,
                     CHAR *bpb2fName, UINT32 bpb2fSize,
                     CHAR *bpf2bCtlName, UINT32 bpf2bCtlSize,
                     CHAR *bpb2fCtlName, UINT32 bpb2fCtlSize,
                     CHAR *f2bName, CHAR *b2fName,
                     CHAR *f2bCtlName, CHAR *b2fCtlName ) ;

void  clearDirtyShellPipe( const CHAR *prefix ) ;


#endif //UTILPIPE_HPP__

