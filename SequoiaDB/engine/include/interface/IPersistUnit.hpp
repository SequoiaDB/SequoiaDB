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

   Source File Name = IPersistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_PERSIST_UNIT_HPP_
#define SDB_I_PERSIST_UNIT_HPP_

#include "sdbInterface.hpp"
#include "interface/IStorageSession.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      IStatPersistUnit define
    */
   class IStatPersistUnit : public _utilPooledObject
   {
   public:
      IStatPersistUnit() = default ;
      virtual ~IStatPersistUnit() = default ;
      IStatPersistUnit( const IStatPersistUnit &o ) = delete ;
      IStatPersistUnit &operator =( const IStatPersistUnit& ) = delete ;

   public:
      virtual INT32 commitUnit( IExecutor *executor ) = 0 ;
      virtual INT32 abortUnit( IExecutor *executor ) = 0 ;
   } ;

   /*
      IPersistUnit define
    */
   class IPersistUnit : public _utilPooledObject
   {
   public:
      IPersistUnit() = default ;
      virtual ~IPersistUnit() = default ;
      IPersistUnit( const IPersistUnit &o ) = delete ;
      IPersistUnit &operator =( const IPersistUnit& ) = delete ;

   public:
      virtual INT32 beginUnit( IExecutor *executor,
                               BOOLEAN isTrans ) = 0 ;
      virtual INT32 prepareUnit( IExecutor *executor,
                                 BOOLEAN isTrans ) = 0 ;
      virtual INT32 commitUnit( IExecutor *executor,
                                BOOLEAN isTrans ) = 0 ;
      virtual INT32 abortUnit( IExecutor *executor,
                               BOOLEAN isTrans,
                               BOOLEAN isForced ) = 0 ;

      virtual INT32 registerStatUnit( utilThreadLocalPtr<IStatPersistUnit> &statUnitPtr ) = 0 ;
   } ;

}

#endif // SDB_I_PERSIST_UNIT_HPP_