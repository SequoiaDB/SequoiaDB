/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilCommObjBuff.hpp

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
#ifndef UTIL_COMMOBJBUFF_HPP_
#define UTIL_COMMOBJBUFF_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "pd.hpp"
#include "../bson/bsonobj.h"

#define UTIL_OBJBUFF_DFT_SIZE        65536
#define UTIL_OBJBUFF_MIN_EXTEND_SIZE ( 1 * 1024 )
#define UTIL_DFT_OBJBUFF_MAX_SIZE    ( 128 * 1024 * 1024 )

using bson::BSONObj ;

namespace seadapter
{
   class _utilCommObjBuff : public SDBObject
   {
      public:
         _utilCommObjBuff( BOOLEAN autoExtent = TRUE,
                          UINT32 sizeLimit = UTIL_DFT_OBJBUFF_MAX_SIZE ) ;
         ~_utilCommObjBuff() ;

         INT32 init( UINT32 size = UTIL_OBJBUFF_DFT_SIZE,
                     UINT32 extendSize = 0 ) ;
         BOOLEAN valid() const ;
         INT32 reset() ;

         INT32 appendObj( const BSONObj &obj) ;
         INT32 appendObj( const BSONObj *obj) ;
         const CHAR *data() { return _buff ; }
         UINT32 capacity() { return _buffSize ; }
         UINT32 dataSize() { return _writePos; }
         UINT32 getObjNum() { return _objNum ; }
         void resetItr() { _readPos = 0 ; }
         OSS_INLINE INT32 nextObj( BSONObj &obj ) ;
         BOOLEAN eof() const { return _readPos >= _writePos ; }

#ifdef _DEBUG
         void viewAll() ;
#endif /* _DEBUG */
      private:
         OSS_INLINE BOOLEAN _enough( INT32 size ) ;
         INT32 _extendBuff() ;
         INT32 _realloc( UINT32 size ) ;

      private:
         BOOLEAN  _init ;
         CHAR *   _buff ;
         UINT32   _buffSize ;
         UINT32   _objNum ;
         UINT32   _objSize ;
         BOOLEAN  _autoExtend ;   // Increase the buffer automatically if it's full.
         UINT32   _extendSize ;
         UINT32   _sizeLimit ;
         UINT32   _writePos ;
         UINT32   _readPos ;
   } ;
   typedef _utilCommObjBuff utilCommObjBuff ;

   OSS_INLINE BOOLEAN _utilCommObjBuff::_enough( INT32 size )
   {
      return (INT32)( _buffSize - _writePos ) - size >= 0 ? TRUE : FALSE ;
   }

   OSS_INLINE INT32 _utilCommObjBuff::nextObj( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      if ( eof() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      try
      {
         BSONObj objTemp( &_buff[_readPos] ) ;
         obj = objTemp ;
         _readPos += ossAlign4( (UINT32)obj.objsize() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to create bson object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
         return SDB_SYS ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

#endif /* RTN_COMMOBJBUFF_HPP_ */

