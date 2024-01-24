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

   Source File Name = pmdOperationContext.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_OPERATION_CONTEXT_HPP_
#define PMD_OPERATION_CONTEXT_HPP_

#include "ossUtil.hpp"
#include "sdbInterface.hpp"
#include "interface/IOperationContext.hpp"

namespace engine
{

   /*
      _pmdOperationContext define
    */
   class _pmdOperationContext : public IOperationContext
   {
   public:
      _pmdOperationContext() = default ;
      virtual ~_pmdOperationContext() = default ;
      _pmdOperationContext( const _pmdOperationContext &o ) = delete ;
      _pmdOperationContext &operator =( const _pmdOperationContext &o ) = delete ;

      virtual IPersistUnit *getPersistUnit()
      {
         return _persistUnit.get() ;
      }

      virtual void setPersistUnit( std::unique_ptr<IPersistUnit> persistUnit )
      {
         _persistUnit = std::move( persistUnit ) ;
      }

      virtual IReadUnit *getReadUnit()
      {
         return _readUnit ;
      }

      virtual void setReadUnit( IReadUnit *readUnit )
      {
         _readUnit = readUnit ;
      }

   protected:
      std::unique_ptr<IPersistUnit> _persistUnit ;
      IReadUnit *_readUnit = nullptr ;
   } ;

   typedef class _pmdOperationContext pmdOperationContext ;

}

#endif // PMD_OPERATION_CONTEXT_HPP_