/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = omContextTransfer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/09/2015  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_CONTEXTTRANSFER_HPP__
#define OM_CONTEXTTRANSFER_HPP__

#include "rtnContext.hpp"
#include "pmdRemoteSession.hpp"
#include "pmdEDU.hpp"
#include "omSdbConnector.hpp"
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   class _omContextTransfer : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _omContextTransfer )
      public:
         _omContextTransfer( INT64 contextID, UINT64 eduID ) ;
         virtual ~_omContextTransfer() ;

         INT32 open( omSdbConnector *conn, MsgHeader *reply ) ;
      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () ;

      protected:
         virtual INT32     _prepareData( _pmdEDUCB *cb ) ;
         INT32             _appendReply( MsgHeader *reply ) ;
         INT32             _getMoreFromRemote( _pmdEDUCB *cb ) ;

      protected:
         omSdbConnector       *_conn ;
         SINT64               _originalContextID ;
   } ;

   typedef _omContextTransfer omContextTransfer ;
}

#endif /* OM_CONTEXTTRANSFER_HPP__ */




