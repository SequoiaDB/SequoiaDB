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

   Source File Name = dmsReadUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsReadUnit.hpp"
#include "dmsDef.hpp"
#include "ossErr.h"
#include "ossMem.hpp"
#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsReadUnitScope implement
    */
   _dmsReadUnitScope::_dmsReadUnitScope( IStorageSession *session, _pmdEDUCB *cb )
   : _cb( cb ),
     _currentReadUnit( session )
   {
      if ( NULL == _cb->getSession() )
      {
         _dummySession.attachCB( _cb ) ;
         _attached = TRUE ;
      }
      SDB_ASSERT( _cb->getSession()->getOperationContext(),
                  "operation context should be valid" ) ;
      _lastReadUnit = _cb->getSession()->getOperationContext()->getReadUnit() ;
      _cb->getSession()->getOperationContext()->setReadUnit( &_currentReadUnit ) ;
   }

   _dmsReadUnitScope::~_dmsReadUnitScope()
   {
      _cb->getSession()->getOperationContext()->setReadUnit( _lastReadUnit ) ;
      if ( _attached )
      {
         _dummySession.detachCB() ;
      }
   }

}
