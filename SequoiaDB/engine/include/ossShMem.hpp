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

   Source File Name = ossShMem.hpp

   Descriptive Name = share memory

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/09/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#if defined (_LINUX)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#elif defined (_WINDOWS)

#endif


#if defined (_LINUX)
#define OSS_SHM_CREATE        IPC_CREAT
#define OSS_SHM_EXCL          IPC_EXCL

typedef UINT32    ossSHMMid;
typedef key_t     ossSHMKey;

#elif defined (_WINDOWS)
typedef HANDLE    ossSHMMid;
typedef CHAR*     ossSHMKey;

#define OSS_SHM_CREATE        0X01
#define OSS_SHM_EXCL          0X02

#endif


CHAR *ossSHMAlloc( ossSHMKey shmKey, UINT32 bufSize, INT32 shmFlag,
                   ossSHMMid &shmMid );

void ossSHMFree( ossSHMMid &shmMid, CHAR **ppBuf );

CHAR *ossSHMAttach( ossSHMKey shmKey, UINT32 bufSize, ossSHMMid &shmMid );

void ossSHMDetach( ossSHMMid & shmMid, CHAR **ppBuf );

