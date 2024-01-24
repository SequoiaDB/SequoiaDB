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

   Source File Name = coordAuthOperator.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_AUTH_OPERATOR_HPP__
#define COORD_AUTH_OPERATOR_HPP__

#include "coordAuthBase.hpp"

namespace engine
{
   /*
      _coordAuthOperator define
   */
   class _coordAuthOperator : public _coordAuthBase
   {
      public:
         _coordAuthOperator() ;
         virtual ~_coordAuthOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

      private:

         virtual void         _onSucReply( const MsgOpReply *pReply ) ;

   } ;
   typedef _coordAuthOperator coordAuthOperator ;

}

#endif // COORD_AUTH_OPERATOR_HPP__

