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

   Source File Name = coordAuthBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_AUTHBASE_HPP__
#define COORD_AUTHBASE_HPP__

#include "coordOperator.hpp"

namespace engine
{

   /*
      _coordAuthBase define
   */
   class _coordAuthBase : public _coordOperator
   {
      public:
         _coordAuthBase() ;
         virtual ~_coordAuthBase() ;

      protected:
         INT32 forward( MsgHeader *pMsg,
                        pmdEDUCB *cb,
                        BOOLEAN sWhenNoPrimary,
                        INT64 &contextID,
                        const CHAR **ppUserName = NULL,
                        const CHAR **ppPass = NULL ) ;

   } ;
   typedef _coordAuthBase coordAuthBase ;

}

#endif // COORD_AUTHBASE_HPP__

