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

   Source File Name = dmsReadUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_READ_UNIT_HPP_
#define SDB_DMS_READ_UNIT_HPP_

#include "dmsDef.hpp"
#include "interface/IReadUnit.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;

   /*
      _dmsReadUnit define
    */
   class _dmsReadUnit : public IReadUnit
   {
   public:
      _dmsReadUnit( IStorageSession *session )
      : _session( session )
      {
      }

      virtual ~_dmsReadUnit() = default ;

      IStorageSession *getSession()
      {
         return _session ;
      }

   protected:
      IStorageSession *_session ;
   } ;

   typedef class _dmsReadUnit dmsReadUnit ;

   /*
      _dmsReadUnitScope define
    */
   class _dmsReadUnitScope
   {
   public:
      _dmsReadUnitScope( IStorageSession *session, _pmdEDUCB *cb ) ;
      ~_dmsReadUnitScope() ;

   protected:
      _pmdEDUCB *_cb = nullptr ;
      dmsReadUnit _currentReadUnit ;
      IReadUnit *_lastReadUnit = nullptr ;
      pmdDummySession _dummySession ;
      BOOLEAN _attached = FALSE ;
   } ;
   typedef class _dmsReadUnitScope dmsReadUnitScope ;

}

#endif // SDB_DMS_READ_UNIT_HPP_