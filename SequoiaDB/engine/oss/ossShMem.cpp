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

   Source File Name = ossShMem.cpp

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
#include "ossShMem.hpp"
#include "pd.hpp"
#include <errno.h>

CHAR *ossSHMAlloc( ossSHMKey shmKey, UINT32 bufSize, INT32 shmFlag, ossSHMMid &shmMid )
{
   CHAR *pBuf = NULL;
#if defined (_LINUX)
   shmMid = shmget( shmKey,bufSize, shmFlag | 0666 );
   if ( shmMid < 0 )
   {
      perror("shmget");
      goto error;
   }
   pBuf = (CHAR *)shmat( shmMid, (const void*)0, 0 );
   if ( (CHAR *)-1 == pBuf )
   {
      shmctl( shmMid, IPC_RMID, 0 );
      pBuf = NULL;
   }
#elif defined (_WINDOWS)
   wchar_t *pKey = NULL;
   UINT32 keySize = MultiByteToWideChar(CP_ACP, 0, shmKey, -1, NULL, 0 );
   if ( keySize <= 0 )
   {
      goto error;
   }
   pKey = new wchar_t[keySize];
   if ( NULL == pKey )
   {
      goto error;
   }
   MultiByteToWideChar(CP_ACP, 0, shmKey, -1, pKey, keySize );
   shmMid = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, pKey );
   if ( NULL == shmMid )
   {
      if ( shmFlag | OSS_SHM_CREATE )
      {
         shmMid = CreateFileMapping( INVALID_HANDLE_VALUE,
                                    NULL,
                                    PAGE_READWRITE,
                                    0,
                                    bufSize,
                                    pKey );
      }
   }
   else
   {
      if ( ( shmFlag | OSS_SHM_CREATE )
            && ( shmFlag | OSS_SHM_EXCL ) )
      {
         CloseHandle( shmMid );
         goto error;
      }
   }
   if ( NULL == shmMid )
   {
      goto error;
   }
   pBuf = (char *)MapViewOfFile( shmMid,
                                 FILE_MAP_ALL_ACCESS,
                                 0,
                                 0,
                                 bufSize );
   if ( NULL == pBuf )
   {
      CloseHandle( shmMid );
      shmMid = NULL;
   }
#endif
done:
#if defined (_WINDOWS)
   if ( pKey != NULL )
   {
      delete[] pKey;
   }
#endif
   return pBuf;
error:
   goto done;
}

CHAR *ossSHMAttach( ossSHMKey shmKey, UINT32 bufSize, ossSHMMid &shmMid )
{
   CHAR *pBuf = NULL;
#if defined (_LINUX)
   shmMid = shmget( shmKey,bufSize, 0 | 0666 );
   if ( shmMid < 0 )
   {
      perror("shmget");
      goto error;
   }
   pBuf = (CHAR *)shmat( shmMid, (const void*)0, 0 );
   if ( (CHAR *)-1 == pBuf )
   {
      shmctl( shmMid, IPC_RMID, 0 );
      pBuf = NULL;
   }
#elif defined (_WINDOWS)
   wchar_t *pKey = NULL;
   UINT32 keySize = MultiByteToWideChar(CP_ACP, 0, shmKey, -1, NULL, 0 );
   if ( keySize <= 0 )
   {
      goto error;
   }
   pKey = new wchar_t[keySize];
   if ( NULL == pKey )
   {
      goto error;
   }
   MultiByteToWideChar(CP_ACP, 0, shmKey, -1, pKey, keySize );
   shmMid = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, pKey );
   if ( NULL == shmMid )
   {
      goto error;
   }
   pBuf = (char *)MapViewOfFile( shmMid,
                                 FILE_MAP_ALL_ACCESS,
                                 0,
                                 0,
                                 bufSize );
   if ( NULL == pBuf )
   {
      CloseHandle( shmMid );
      shmMid = NULL;
   }
#endif
done:
#if defined (_WINDOWS)
   if ( pKey != NULL )
   {
      delete[] pKey;
   }
#endif
   return pBuf;
error:
   goto done;
}

void ossSHMFree( ossSHMMid & shmMid, CHAR **ppBuf )
{
   SDB_ASSERT( ppBuf != NULL, "ppBuf can't be NULL!" ) ;
   if ( NULL == ppBuf )
   {
      return;
   }
#if defined (_LINUX)
   if ( shmMid >= 0 )
   {
      shmctl( shmMid, IPC_RMID, 0 );
      shmMid = -1;
   }
   *ppBuf = NULL;
#elif defined (_WINDOWS)
   if ( *ppBuf != NULL )
   {
      UnmapViewOfFile( *ppBuf );
      *ppBuf = NULL;
   }
   if ( shmMid != NULL )
   {
      CloseHandle( shmMid );
      shmMid = NULL;
   }
#endif
}

void ossSHMDetach( ossSHMMid & shmMid, CHAR **ppBuf )
{
   SDB_ASSERT( ppBuf != NULL, "ppBuf can't be NULL!" ) ;
#if defined (_LINUX)
   if ( NULL != ppBuf )
   {
      shmdt( *ppBuf );
      *ppBuf = NULL;
   }
   shmMid = -1;
#elif defined (_WINDOWS)
   if ( *ppBuf != NULL )
   {
      UnmapViewOfFile( *ppBuf );
      *ppBuf = NULL;
   }
   if ( shmMid != NULL )
   {
      CloseHandle( shmMid );
      shmMid = NULL;
   }
#endif
}