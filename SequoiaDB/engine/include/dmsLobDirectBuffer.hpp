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

   Source File Name = dmsLobDirectBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LOBDIRECTBUFFER_HPP_
#define DMS_LOBDIRECTBUFFER_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "sdbInterface.hpp"

namespace engine
{

   /*
      _dmsLobDirectBuffer define
   */
   class _dmsLobDirectBuffer : public SDBObject
   {
   public:
       typedef struct _tuple
       {
         CHAR    *buf ;
         UINT32   size ;
         UINT32   offset ;

         _tuple()
         {
            buf      = NULL ;
            size     = 0 ;
            offset   = 0 ;
         }
       } tuple ;

   public:
      _dmsLobDirectBuffer( CHAR *usrBuf,
                           UINT32 size,
                           UINT32 offset,
                           BOOLEAN needAligned,
                           IExecutor *cb ) ;

      virtual ~_dmsLobDirectBuffer() ;

      BOOLEAN  isAligned() const { return _aligned ; }

      INT32    prepare() ;

   public:
      virtual  INT32 doit( const tuple **pTuple ) = 0 ;
      virtual  void  done() = 0 ;

   protected:
      INT32 _extendBuf( UINT32 size ) ;

   protected:
      tuple       _t ;
      BOOLEAN     _aligned ;

      IExecutor   *_cb ;
      CHAR        *_buf ;
      UINT32      _bufSize ;

      CHAR        *_usrBuf ;
      UINT32      _usrSize ;
      UINT32      _usrOffset ;
   } ;
   typedef class _dmsLobDirectBuffer dmsLobDirectBuffer ;
}

#endif //DMS_LOBDIRECTBUFFER_HPP_

