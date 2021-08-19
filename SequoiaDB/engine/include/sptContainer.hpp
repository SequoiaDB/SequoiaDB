/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = sptContainer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_CONTAINER_HPP_
#define SPT_CONTAINER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptScope.hpp"
#include "ossLatch.hpp"
#include <vector>

namespace engine
{

   typedef std::vector< _sptScope* >            VEC_SCOPE ;
   typedef VEC_SCOPE::iterator                  VEC_SCOPE_IT ;

   /*
      _sptContainer define
   */
   class _sptContainer : public SDBObject
   {
   public:
      _sptContainer() ;
      virtual ~_sptContainer() ;

      INT32    init () ;
      INT32    fini () ;

   public:
      _sptScope   *newScope( SPT_SCOPE_TYPE type = SPT_SCOPE_TYPE_SP,
                             UINT32 loadMask = SPT_OBJ_MASK_ALL ) ;
      void        releaseScope( _sptScope *pScope ) ;

   protected:
      _sptScope* _getFromCache( SPT_SCOPE_TYPE type,
                                UINT32 loadMask ) ;

      _sptScope* _createScope( SPT_SCOPE_TYPE type,
                               UINT32 loadMask ) ;

   private:
      VEC_SCOPE            _vecScopes ;
      ossSpinXLatch        _latch ;

   } ;

   typedef class _sptContainer sptContainer ;
}

#endif // SPT_CONTAINER_HPP_

