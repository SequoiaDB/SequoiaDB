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

   Source File Name = clsVSSecondary.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSVSSECONDARY_HPP_
#define CLSVSSECONDARY_HPP_

#include "clsVoteStatus.hpp"

namespace engine
{
   class _clsVSSecondary : public _clsVoteStatus
   {
   public:
      _clsVSSecondary( _clsGroupInfo *info,
                       _netRouteAgent *agent ) ;
      virtual ~_clsVSSecondary() ;
   public:
      virtual INT32 handleInput( const MsgHeader *header,
                                 INT32 &next ) ;

      virtual void handleTimeout( const UINT32 &millisec,
                                  INT32 &next ) ;

      virtual void active( INT32 &next ) ;

      virtual const CHAR *name() const { return "Secondary" ;}

   private:
      BOOLEAN           _hasPrint ;
   } ;
}

#endif

