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

   Source File Name = coordGroupHandle.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_GROUP_HANDLE_HPP__
#define COORD_GROUP_HANDLE_HPP__

#include "coordRemoteSession.hpp"

namespace engine
{

   /*
      _coordGroupHandler define
   */
   class _coordGroupHandler : public _IGroupSessionHandler
   {
      public:
         _coordGroupHandler() ;
         virtual ~_coordGroupHandler() ;

      public:
         virtual void   prepareForSend( pmdSubSession *pSub,
                                        _coordGroupSel *pSel,
                                        _coordGroupSessionCtrl *pCtrl ) ;

   } ;
   typedef _coordGroupHandler coordGroupHandler ;

}

#endif // COORD_GROUP_HANDLE_HPP__
