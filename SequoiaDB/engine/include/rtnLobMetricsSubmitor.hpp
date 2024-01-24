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

   Source File Name = rtnLobMetricsSubmitor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/29/2022  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBMETRICSSUBMITOR_HPP_
#define RTN_LOBMETRICSSUBMITOR_HPP_

#include "ossTypes.h"
#include "monInterface.hpp"
#include "monCB.hpp"

namespace engine
{

   class _pmdEDUCB ;

   class _rtnLobMetricsSubmitor: public SDBObject
   {
   public:
      _rtnLobMetricsSubmitor( _pmdEDUCB *cb, IMonSubmitEvent *pEvent = NULL ) ;
      ~_rtnLobMetricsSubmitor() ;
      void   submit() ;
      void   discard() ;

   private:
      _pmdEDUCB*          _eduCB ;
      IMonSubmitEvent*    _pEvent ;
      monAppCB            _monAppBegin ;
      BOOLEAN             _hasSubmit ;
      BOOLEAN             _hasDiscard ;
   } ;

   typedef _rtnLobMetricsSubmitor rtnLobMetricsSubmitor ;

}

#endif

