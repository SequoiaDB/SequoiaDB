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

   Source File Name = utilCommObjBuff.cpp

   Descriptive Name = Common BSONObj buffer

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/01/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifdef _DEBUG
#include <iostream>
#endif /* _DEBUG */

#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "utilCommObjBuff.hpp"

namespace seadapter
{
   _utilCommObjBuff::_utilCommObjBuff( BOOLEAN autoExtent, UINT32 sizeLimit )
   {
      _init = FALSE ;
      _buff = NULL ;
      _buffSize = 0 ;
      _objNum = 0 ;
      _objSize = 0 ;
      _autoExtend = autoExtent ;
      _extendSize = 0 ;
      _sizeLimit = sizeLimit ;
      _writePos = 0 ;
      _readPos = 0 ;
   }

   _utilCommObjBuff::~_utilCommObjBuff()
   {
      if ( _buff )
      {
         SDB_OSS_FREE( _buff ) ;
      }
   }

   INT32 _utilCommObjBuff::init( UINT32 size, UINT32 extendSize )
   {
      INT32 rc = SDB_OK ;
      if ( _buff )
      {
         SDB_OSS_FREE( _buff ) ;
         _buff = NULL ;
         _init = FALSE ;
      }

      _buff = (CHAR *)SDB_OSS_MALLOC( size ) ;
      if ( !_buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate memory, size: %u, rc: %d",
                 size, rc ) ;
         goto error ;
      }

      _buffSize = size ;
      _extendSize = extendSize ;
      _init = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _utilCommObjBuff::valid() const
   {
      return _init ;
   }

   INT32 _utilCommObjBuff::reset()
   {
      INT32 rc = SDB_OK ;
      if ( !_init )
      {
         rc = init() ;
         PD_RC_CHECK( rc, PDERROR, "Initialize buffer failed[ %d ]" ) ;
      }
      _objNum = 0 ;
      _objSize = 0 ;
      _writePos = 0 ;
      _readPos = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCommObjBuff::appendObj( const BSONObj &obj)
   {
      INT32 rc = SDB_OK ;
      CHAR *writePos = NULL ;

      if ( !_init )
      {
         rc =  SDB_SYS ;
         PD_LOG( PDERROR, "Object buffer is not initialized" ) ;
         goto error ;
      }

      try
      {
         // The object may be very bit, in which case we may extend the buffer
         // multiple times.
         while ( !_enough( ossAlign4( (UINT32)obj.objsize() ) ) )
         {
            if ( _buffSize >= _sizeLimit )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Buffer is not enough, rc: %d", rc ) ;
               goto error ;
            }

            rc = _extendBuff() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to extend object buffer, rc: %d",
                         rc ) ;
         }

         writePos = _buff + _writePos ;
         ossMemcpy( writePos, obj.objdata(), obj.objsize() ) ;
         _objNum++ ;
         _writePos += ossAlign4( (UINT32)obj.objsize() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCommObjBuff::appendObj( const BSONObj *obj)
   {
      INT32 rc = SDB_OK ;
      CHAR *writePos = NULL ;

      if ( !_init )
      {
         rc =  SDB_SYS ;
         PD_LOG( PDERROR, "Object buffer is not initialized" ) ;
         goto error ;
      }

      try
      {
         // The object may be very big, in which case we may extend the buffer
         // multiple times.
         while ( !_enough( ossAlign4( (UINT32)obj->objsize() ) ) )
         {
            if ( _buffSize >= _sizeLimit )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Buffer is not enough, rc: %d", rc ) ;
               goto error ;
            }

            rc = _extendBuff() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to extend object buffer, rc: %d",
                         rc ) ;
         }

         writePos = _buff + _writePos ;
         ossMemcpy( writePos, obj->objdata(), obj->objsize() ) ;
         _objNum++ ;
         _writePos += ossAlign4( (UINT32)obj->objsize() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _utilCommObjBuff::_extendBuff()
   {
      INT32 rc = SDB_OK ;
      UINT32 targetSize = 0 ;
      UINT32 extendSize = 0 ;

      extendSize = ( 0 == _extendSize ) ? _buffSize : _extendSize ;
      targetSize = _buffSize + extendSize ;
      if ( targetSize > _sizeLimit )
      {
         targetSize = _sizeLimit ;
      }

      rc = _realloc( targetSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to realloc object buffer, rc: %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCommObjBuff::_realloc( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      CHAR *newBuff = NULL ;
      newBuff = (CHAR *)SDB_OSS_REALLOC( _buff, size ) ;
      if ( !newBuff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate memory, size: %u, rc: %d",
                 size, rc ) ;
         goto error ;
      }
      _buff = newBuff ;
      _buffSize = size ;

   done:
      return rc ;
   error:
      goto done ;
   }

#ifdef _DEBUG
   void _utilCommObjBuff::viewAll()
   {
      UINT32 readPos = 0 ;
      std::cout << "All objects in buffer: " << endl ;
      while ( readPos < _writePos )
      {
         BSONObj objTemp( &_buff[readPos] ) ;
         std::cout << objTemp.toString() << endl ;
         readPos += ossAlign4( (UINT32)objTemp.objsize() ) ;
      }
   }
#endif /* _DEBUG */
}

