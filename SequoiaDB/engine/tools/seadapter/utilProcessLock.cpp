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

   Source File Name = utilProcessLock.cpp

   Descriptive Name = Util Processor Lock.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/25/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilProcessLock.hpp"
#include "ossUtil.hpp"

namespace seadapter
{
   _utilProcFlockMutex::_utilProcFlockMutex()
   : _fileOpened( FALSE ),
     _fileLocked( FALSE )
   {
   }

   _utilProcFlockMutex::~_utilProcFlockMutex()
   {
      if ( _fileOpened )
      {
         destroy() ;
      }
   }

   INT32 _utilProcFlockMutex::init( const CHAR *processName, void *arg )
   {
      INT32 rc = SDB_OK ;
      UINT32 mode = OSS_READWRITE | OSS_CREATE ;
      const CHAR *fileName = (const CHAR *)arg ;

      SDB_UNUSED( processName ) ;

      if ( !fileName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Invalid argument to initialize process flock mutex" ) ;
         goto error ;
      }

      if ( _fileOpened )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Process flock mutex initialized already" ) ;
         goto error ;
      }

      // Open the lock file. Create it if it's not there.
      rc = ossOpen( fileName, mode, OSS_RU|OSS_WU|OSS_RG, _file ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open lock file failed[ %d ]", rc ) ;
         goto error ;
      }

      _lockFileName = std::string( fileName ) ;
      _fileOpened = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilProcFlockMutex::lock()
   {
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 _utilProcFlockMutex::tryLock()
   {
      INT32 rc = SDB_OK ;
      if ( !_fileOpened )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Process flock mutex is not initialized" ) ;
         goto error ;
      }

      rc = ossLockFile( &_file, OSS_LOCK_EX ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Locking lock file failed[ %d ]", rc ) ;
         goto error ;
      }

      _fileLocked = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilProcFlockMutex::unlock()
   {
      if ( _fileLocked )
      {
         ossLockFile( &_file, OSS_LOCK_UN ) ;
         _fileLocked = FALSE ;
      }
   }

   void _utilProcFlockMutex::destroy()
   {
      BOOLEAN lockedByMe = _fileLocked ;

      if ( _fileLocked )
      {
         unlock() ;
      }

      if ( _fileOpened )
      {
         ossClose( _file ) ;
         if ( lockedByMe )
         {
            ossDelete( _lockFileName.c_str() ) ;
         }
         _fileOpened = FALSE ;
      }
   }
}

