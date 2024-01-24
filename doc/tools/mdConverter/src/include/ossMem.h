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

   Source File Name = ossMem.h

   Descriptive Name = Operating System Services Memory Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for all memory
   allocation/free operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSMEM_H_
#define OSSMEM_H_
#include "core.h"

/*
 * [x bytes start][8 bytes data size][4 bytes guard size][4 bytes file hash][4
 * bytes line num][data][1 byte end][x-1 bytes stop]*/
#define SDB_MEMDEBUG_MINGUARDSIZE 256
#define SDB_MEMDEBUG_MAXGUARDSIZE 4194304
#define SDB_MEMDEBUG_GUARDSTART ((CHAR)0xBE)
#define SDB_MEMDEBUG_GUARDSTOP  ((CHAR)0xBF)
#define SDB_MEMDEBUG_GUARDEND   ((CHAR)0xBD)
#define SDB_MEMHEAD_EYECATCHER1 0xFABD0538
#define SDB_MEMHEAD_EYECATCHER2 0xFACE7352

#define SDB_OSS_MALLOC(x)       ossMemAlloc(x,__FILE__,__LINE__)
#define SDB_OSS_FREE(x)         ossMemFree(x)
#define SDB_OSS_ORIGINAL_FREE(x) free(x)
#define SDB_OSS_REALLOC(x,y)    ossMemRealloc(x,y,__FILE__,__LINE__)

#define SDB_OSS_MALLOC3(x,y,z)  ossMemAlloc(x,y,z)

#define SAFE_OSS_FREE(p)      \
   do {                       \
      if (p) {                \
         SDB_OSS_FREE(p) ;    \
         p = NULL ;           \
      }                       \
   } while (0)

SDB_EXTERN_C_START

void  ossEnableMemDebug( BOOLEAN debugEnable, UINT32 memDebugSize ) ;

void* ossMemAlloc ( size_t size, const CHAR* file, UINT32 line ) ;

void* ossMemRealloc ( void* pOld, size_t size,
                      const CHAR* file, UINT32 line ) ;

void ossMemFree ( void *p ) ;

void ossMemTrack ( void *p ) ;

void ossMemUnTrack ( void *p ) ;

void ossMemTrace ( const CHAR *pPath ) ;

void *ossAlignedAlloc( UINT32 alignment, UINT32 size ) ;

SDB_EXTERN_C_END
#endif
