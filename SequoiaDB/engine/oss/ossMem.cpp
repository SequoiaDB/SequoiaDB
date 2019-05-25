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

   Source File Name = ossMem.cpp

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
#include "ossMem.hpp"
#include "ossMem.c"

#if defined (__cplusplus) && defined (SDB_ENGINE)

#include "ossLatch.hpp"
#include "ossPrimitiveFileOp.hpp"
#include <set>

static BOOLEAN ossMemTrackCBInit = FALSE ;
struct _ossMemTrackCB
{
   ossSpinXLatch  _memTrackMutex ;
   std::set<void*> _memTrackMap ;
   _ossMemTrackCB () { ossMemTrackCBInit = TRUE ; }
   ~_ossMemTrackCB () { ossMemTrackCBInit = FALSE ; }
} ;
typedef struct _ossMemTrackCB ossMemTrackCB ;
static ossMemTrackCB gMemTrackCB ;

void ossMemTrack ( void *p )
{
   if ( ossMemTrackCBInit )
   {
      gMemTrackCB._memTrackMutex.get() ;
      gMemTrackCB._memTrackMap.insert(p) ;
      gMemTrackCB._memTrackMutex.release() ;
   }
}
void ossMemUnTrack ( void *p )
{
   if ( ossMemTrackCBInit )
   {
      gMemTrackCB._memTrackMutex.get() ;
      gMemTrackCB._memTrackMap.erase(p) ;
      gMemTrackCB._memTrackMutex.release() ;
   }
}

#define OSSMEMTRACEDUMPBUFSZ 1024
static UINT64 ossMemTraceDump ( void *p, ossPrimitiveFileOp &trapFile )
{
   CHAR lineBuffer [ OSSMEMTRACEDUMPBUFSZ + 1 ] = {0} ;
   CHAR *pAddr = (CHAR*)p ;
   ossMemset ( lineBuffer, 0, sizeof(lineBuffer) ) ;
   ossSnprintf ( lineBuffer, sizeof(lineBuffer),
                 " Address: %p\n", pAddr ) ;
   trapFile.Write ( lineBuffer ) ;
   ossMemset ( lineBuffer, 0, sizeof(lineBuffer) ) ;
   ossSnprintf ( lineBuffer, sizeof(lineBuffer),
                 " Freed: %s, Size: %lld, DebugSize: %d\n",
                 (*(UINT32*)(pAddr+OSS_MEM_HEAD_FREEDOFFSET))==0?"false":"true",
                 (*(UINT64*)(pAddr+OSS_MEM_HEAD_SIZEOFFSET)),
                 (*(UINT32*)(pAddr+OSS_MEM_HEAD_DEBUGOFFSET)) ) ;
   trapFile.Write ( lineBuffer ) ;
   ossMemset ( lineBuffer, 0, sizeof(lineBuffer) ) ;
   ossSnprintf ( lineBuffer, sizeof(lineBuffer),
                 " File: 0x%x, Line: %d\n",
                 (*(UINT32*)(pAddr+OSS_MEM_HEAD_FILEOFFSET)),
                 (*(UINT32*)(pAddr+OSS_MEM_HEAD_LINEOFFSET)) ) ;
   trapFile.Write ( lineBuffer ) ;
   trapFile.Write ( "\n" ) ;
   return (*(UINT64*)(pAddr+OSS_MEM_HEAD_SIZEOFFSET)) ;
}

void ossMemTrace ( const CHAR *pPath )
{
   ossPrimitiveFileOp trapFile ;
   CHAR fileName [ OSS_MAX_PATHSIZE + 1 ] = {0} ;
   UINT64 totalSize                       = 0 ;
   if ( !ossMemTrackCBInit )
   {
      return ;
   }
   gMemTrackCB._memTrackMutex.get() ;

   if ( OSS_MAX_PATHSIZE <
        ossStrlen ( pPath ) + ossStrlen ( OSS_PRIMITIVE_FILE_SEP ) +
        ossStrlen ( SDB_OSS_MEMDUMPNAME ) )
   {
      goto error ;
   }
   ossMemset ( fileName, 0, sizeof ( fileName ) ) ;
   ossSnprintf ( fileName, sizeof(fileName), "%s%s%s",
                 pPath, OSS_PRIMITIVE_FILE_SEP, SDB_OSS_MEMDUMPNAME ) ;

   trapFile.Open ( fileName ) ;

   if ( trapFile.isValid () )
   {
      trapFile.seekToEnd () ;
      trapFile.Write ( " -------- Memory Allocation Information --------\n" ) ;
      std::set<void*>::iterator it ;
      for ( it = gMemTrackCB._memTrackMap.begin() ;
            it != gMemTrackCB._memTrackMap.end() ;
            ++it )
      {
         void *p = *it ;
         totalSize += ossMemTraceDump ( p, trapFile ) ;
      }
      ossMemset ( fileName, 0, sizeof ( fileName ) ) ;
      ossSnprintf ( fileName, sizeof(fileName),
                    " -------- Totally Allocated %lld Bytes --------\n",
                    totalSize ) ;
      trapFile.Write ( fileName ) ;
   }
done :
   trapFile.Close () ;
   gMemTrackCB._memTrackMutex.release() ;
   return ;
error :
   goto done ;
}
#endif
