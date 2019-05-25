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

   Source File Name = dmsLobDirectInBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LOBDIRECTINBUFFER_HPP_
#define DMS_LOBDIRECTINBUFFER_HPP_

#include "dmsLobDirectBuffer.hpp"

namespace engine
{
   /*
      _dmsLobDirectInBuffer define
   */
   class _dmsLobDirectInBuffer : public _dmsLobDirectBuffer
   {
   public:
      _dmsLobDirectInBuffer( CHAR *usrBuf,
                             UINT32 size,
                             UINT32 offset,
                             BOOLEAN needAligned,
                             IExecutor *cb ) ;
      virtual ~_dmsLobDirectInBuffer() ;

   public:
      virtual  INT32 doit( const tuple **pTuple ) ;
      virtual  void  done() ;

   private:

   } ;
   typedef class _dmsLobDirectInBuffer dmsLobDirectInBuffer ;
}

#endif //DMS_LOBDIRECTINBUFFER_HPP_

