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
#ifndef OSSDYNAMICLOAD_HPP__
#define OSSDYNAMICLOAD_HPP__

#include "core.hpp"
#include "oss.hpp"
typedef INT32 ( OSS_MODULE_FUNCTION ) () ;
typedef OSS_MODULE_FUNCTION *OSS_MODULE_PFUNCTION ;
#if defined (_WINDOWS)
typedef HMODULE OSS_MHANDLE ;
#else
typedef void* OSS_MHANDLE ;
#endif
#define OSS_MINVALIDHANDLE NULL

#if defined (_WINDOWS)
#define OSSMODULEHANDLE_ERR(rc)                               \
switch(rc)                                                    \
{                                                             \
case ERROR_INVALID_ACCESS:                                    \
case ERROR_ACCESS_DENIED:                                     \
   rc = SDB_PERM ;                                            \
   break ;                                                    \
case ERROR_GEN_FAILURE :                                      \
case ERROR_MOD_NOT_FOUND :                                    \
case ERROR_FILE_NOT_FOUND :                                   \
case ERROR_INVALID_NAME :                                     \
case ERROR_PATH_NOT_FOUND :                                   \
   rc = SDB_FNE ;                                             \
   break ;                                                    \
case ERROR_NOT_ENOUGH_MEMORY :                                \
   rc = SDB_OOM ;                                             \
   break ;                                                    \
default :                                                     \
   rc = SDB_SYS ;                                             \
   break ;                                                    \
}                                                             \


#endif

#ifdef _LINUX
#define LIB_START_STR "lib"
#endif

#ifdef _LINUX
#define LIB_END_STR ".so"
#else
#define LIB_END_STR ".dll"
#endif

class _ossModuleHandle : public SDBObject
{
public :
   BOOLEAN           _isInitialized ;
   CHAR              _moduleName [ OSS_MAX_PATHSIZE + 1 ] ;
   OSS_MHANDLE       _moduleHandle ;
   CHAR              _libPath [ OSS_MAX_PATHSIZE + 1 ] ;
   UINT32            _flags ;
   _ossModuleHandle ( const CHAR *pModuleName,
                      const CHAR *pLibraryPath,
                      UINT32 dlOpenMode = 0 ) ;
   ~_ossModuleHandle ()
   {
      unload () ;
   }
   INT32 init () ;
   INT32 unload () ;
   INT32 resolveAddress ( const CHAR *pFunctionName,
                          OSS_MODULE_PFUNCTION *pFunctionAddress ) ;
private:
   INT32 patchModuleName( const CHAR* name, CHAR *patchedName, UINT32 size ) ;
} ;
typedef class _ossModuleHandle ossModuleHandle ;

INT32 ossLoadModule ( ossModuleHandle *mHandle, const CHAR *pModuleName,
                      const CHAR *pLibraryPath,
                      UINT32 dlOpenMode, UINT32 options = 0 ) ;

INT32 ossUnloadModule ( ossModuleHandle *mHandle ) ;

INT32 ossResolveAddress ( ossModuleHandle *mHandle,
                          const CHAR *pFunctionName,
                          OSS_MODULE_PFUNCTION *pFunctionAddress ) ;
#endif
