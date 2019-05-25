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

   Source File Name = coordAuthCrtOperator.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_AUTHCRT_OPERATOR_HPP__
#define COORD_AUTHCRT_OPERATOR_HPP__

#include "coordAuthBase.hpp"

namespace engine
{
   /*
      _coordAuthCrtOperator define
   */
   class _coordAuthCrtOperator : public _coordAuthBase
   {
      public:
         _coordAuthCrtOperator() ;
         virtual ~_coordAuthCrtOperator() ;

         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

   } ;
   typedef _coordAuthCrtOperator coordAuthCrtOperator ;

}

#endif // COORD_AUTHCRT_OPERATOR_HPP__

