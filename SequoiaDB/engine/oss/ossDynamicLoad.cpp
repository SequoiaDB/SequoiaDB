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

   Source File Name = ossDynamicLoad.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of OSS component. This file contains declare of oss dynamic loading
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/26/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossDynamicLoad.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#if defined (_LINUX)
#include <dlfcn.h>
#endif

_ossModuleHandle::_ossModuleHandle ( const CHAR *pModuleName,
                                     const CHAR *pLibraryPath,
                                     UINT32 dlOpenMode ) :
_isInitialized(FALSE),
_moduleHandle(OSS_MINVALIDHANDLE),
_flags(dlOpenMode)
{
   ossStrncpy ( _moduleName, pModuleName, sizeof(_moduleName) ) ;
   if ( pLibraryPath )
      ossStrncpy ( _libPath, pLibraryPath, sizeof(_libPath) ) ;
   else
      ossMemset ( _libPath, 0, sizeof(_libPath) ) ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSMODULEHANDLE_INIT, "_ossModuleHandle::init" ) ;
// load module
INT32 _ossModuleHandle::init ()
{
   INT32 rc = SDB_OK ;
   CHAR  strPath [ 2*OSS_MAX_PATHSIZE + 1 ] = {0} ;
   CHAR  strModule [ OSS_MAX_PATHSIZE + 1 ] = {0} ;
   OSS_MHANDLE handle = 0 ;
   CHAR *p = NULL ;
#if defined (_WINDOWS)
   UINT32 errorMode ;
#endif
   PD_TRACE_ENTRY ( SDB_OSSMODULEHANDLE_INIT ) ;
   if ( _moduleName[0] == '\0' )
   {
      PD_LOG ( PDERROR, "Module name can't be empty" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PD_TRACE3 ( SDB_OSSMODULEHANDLE_INIT, PD_PACK_STRING(_moduleName),
               PD_PACK_STRING(_libPath), PD_PACK_UINT(_flags) ) ;
   // copy module name to local and remove everything after (
   ossStrncpy ( strModule, _moduleName, sizeof(strModule) ) ;
   p = ossStrchr ( strModule, '(' ) ;
   if ( p )
   {
      *p = '\0' ;
   }
   rc = patchModuleName( strModule, _moduleName, sizeof(_moduleName) );
   PD_RC_CHECK ( rc, PDERROR, "Failed to patch module name, rc = %d", rc ) ;
   // if the path is provided, let's make sure it exists
   if ( _libPath[0] )
   {
      INT32 pathLen = 0 ;
      rc = ossAccess ( _libPath,
#if defined (_LINUX)
                       F_OK
#elif defined (_WINDOWS)
                       0
#endif
      ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to access path %s, rc = %d",
                    _libPath, rc ) ;
      ossStrncat ( strPath, _libPath, sizeof(strPath) ) ;
      pathLen = ossStrlen ( strPath ) ;
      // to add / at end of path
      if ( strPath[pathLen-1] != OSS_FILE_SEP_CHAR )
      {
         // make sure we have sufficient size
         if ( pathLen >= OSS_MAX_PATHSIZE )
         {
            PD_LOG ( PDERROR, "library path is too long: %s",
                     _libPath ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // append path with / and set next char to 0
         strPath[pathLen-1] = OSS_FILE_SEP_CHAR ;
         strPath[pathLen]   = '\0' ;
      }
   }
   // append module name
   if ( ossStrlen ( strPath ) + ossStrlen ( _moduleName ) >= sizeof(strPath) )
   {
      PD_LOG ( PDERROR, "path + module name is too long: %s:%s",
               strPath, _moduleName ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   ossStrncat ( strPath, _moduleName, OSS_MAX_PATHSIZE ) ;
#if defined (_LINUX)
   // try to open and load
   handle = dlopen ( strPath, _flags | RTLD_NOW ) ;
   if ( !handle )
   {
      PD_LOG ( PDERROR, "Failed to load module %s, error = %s",
               strPath, dlerror() ) ;
      rc = SDB_SYS ;
      goto error ;
   }
   // when we successfully opened library, let's initialize members
   _isInitialized = TRUE ;
   _moduleHandle = handle ;
   // clear errors
   dlerror() ;
#elif defined (_WINDOWS)
   // avoid popup a window if the dll was not found
   errorMode = SetErrorMode ( SEM_NOOPENFILEERRORBOX |
                              SEM_FAILCRITICALERRORS ) ;
   _moduleHandle = LoadLibrary ( (LPCTSTR)strPath ) ;
   SetErrorMode ( errorMode ) ;
   if ( NULL == _moduleHandle )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to load module %s, error = %d",
               _moduleName, rc ) ;
      OSSMODULEHANDLE_ERR(rc) ;
      goto error ;
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSMODULEHANDLE_INIT, rc ) ;
   return rc ;
error :
   _isInitialized = FALSE ;
   _moduleHandle = OSS_MINVALIDHANDLE ;
   goto done ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSMODULEHANDLE_UNLOAD, "_ossModuleHandle::unload" ) ;
INT32 _ossModuleHandle::unload ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSMODULEHANDLE_UNLOAD ) ;
   if ( !_moduleHandle )
   {
      goto error ;
   }
   
#if defined (_LINUX)
   rc = dlclose ( _moduleHandle ) ;
   if ( 0 != rc )
   {
      PD_LOG ( PDERROR, "Failed to close library: %s",
               dlerror() ) ;
      rc = SDB_SYS ;
      goto error ;
   }
#elif defined (_WINDOWS)
   if ( !FreeLibrary ( _moduleHandle ) )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to close library: %d",
               rc ) ;
      OSSMODULEHANDLE_ERR(rc) ;
      goto error ;
   }
#endif
   _isInitialized = FALSE ;
   _moduleHandle = OSS_MINVALIDHANDLE ;
done :
   PD_TRACE_EXITRC ( SDB_OSSMODULEHANDLE_UNLOAD, rc ) ;
   return rc ;
error :
   goto done ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSMODULEHANDLE_RESOLVEADDRESS, "_ossModuleHandle::resolveAddress" )
INT32 _ossModuleHandle::resolveAddress (
                          const CHAR *pFunctionName,
                          OSS_MODULE_PFUNCTION *pFunctionAddress )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSMODULEHANDLE_RESOLVEADDRESS ) ;
   if ( !pFunctionName || !pFunctionAddress )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PD_TRACE2 ( SDB_OSSMODULEHANDLE_RESOLVEADDRESS,
               PD_PACK_STRING ( _moduleName ),
               PD_PACK_STRING ( pFunctionName )) ;
   SDB_ASSERT ( _isInitialized,
                "handle must be initialized before resolve address" ) ;
#if defined (_LINUX)
   *pFunctionAddress = (OSS_MODULE_PFUNCTION)dlsym ( _moduleHandle,
                                                     (CHAR*)pFunctionName ) ;
   if ( !*pFunctionAddress )
   {
      PD_LOG ( PDERROR, "Failed to find function %s: %s",
               pFunctionName, dlerror () ) ;
      rc = SDB_SYS ;
      goto error ;
   }
#elif defined (_WINDOWS)
   *pFunctionAddress = (OSS_MODULE_PFUNCTION)GetProcAddress (
         _moduleHandle,
         (LPCSTR)pFunctionName ) ;
   if ( !*pFunctionAddress )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to find function %s: %d",
               pFunctionName, rc ) ;
      OSSMODULEHANDLE_ERR(rc) ;
      goto error ;
   }
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSMODULEHANDLE_RESOLVEADDRESS, rc ) ;
   return rc ;
error :
   goto done ;
}

INT32 _ossModuleHandle::patchModuleName( const CHAR* name,
                                         CHAR *patchedName,
                                         UINT32 size )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( name, "Module name can not be NULL" ) ;
   SDB_ASSERT( patchedName, "Patched buffer can not be NULL" ) ;

   const CHAR *ptr = NULL ;
   INT32 patchedNameLen = 0 ;
   INT32 tailLen = ossStrlen(LIB_END_STR) ;

   ossMemset( patchedName, 0, size ) ;
   // patch begin
#ifdef _LINUX
   ptr = ossStrstr( name, LIB_START_STR ) ;
   if ( ptr != name )
   {
      /// not exist, add prefix
      ossStrncpy( patchedName, LIB_START_STR, size - 1 ) ;
   }
#endif //_LINUX

   patchedNameLen = ossStrlen( patchedName ) ;
   ptr = ossStrrchr( name, '.' ) ;
   if ( ptr != NULL )
   {
      // make sure module name end with ".so" or ".dll"
#ifdef _WINDOWS
      if ( 'd' != *(ptr + 1) || 'l' != *(ptr + 2) && 'l' != *(ptr + 3) )
      {
         if ( patchedNameLen + ossStrlen( name ) + tailLen >= size )
         {
            rc = SDB_INVALIDSIZE ;
            goto error ;
         }
         ossStrncat( patchedName, name, ossStrlen( name ) ) ;
         ossStrncat( patchedName, LIB_END_STR, tailLen ) ;
      }
#else
      if ( 's' != *(ptr + 1) && 'o' != *(ptr + 2) )
      {
         if ( patchedNameLen + ossStrlen( name ) + tailLen >= size )
         {
            rc = SDB_INVALIDSIZE ;
            goto error ;
         }
         ossStrncat( patchedName, name, ossStrlen( name ) ) ;
         ossStrncat( patchedName, LIB_END_STR, tailLen ) ;
      }
#endif
      else
      {
         if ( patchedNameLen + ossStrlen( name ) + tailLen >= size )
         {
            rc = SDB_INVALIDSIZE ;
            goto error ;
         }
         ossStrncat( patchedName, name, strlen( name ) ) ;
      }
   }
   else
   {
      if ( patchedNameLen + ossStrlen( name ) + tailLen >= size )
      {
         rc = SDB_INVALIDSIZE ;
         goto error ;
      }
      ossStrncat( patchedName, name, ptr - name ) ;
      ossStrncat( patchedName, LIB_END_STR, tailLen ) ;
   }

done:
   return rc ;
error:
   goto done ;
}
