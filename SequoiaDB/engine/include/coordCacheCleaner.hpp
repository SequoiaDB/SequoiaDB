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

   Source File Name = coordCacheCleaner.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/30/2022  Tangtao Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_CACHECLEANER_HPP__
#define COORD_CACHECLEANER_HPP__

#include "coordResource.hpp"
#include "utilLightJobBase.hpp"


using namespace bson ;

namespace engine
{

   class _coordCacheCleaner : public _utilLightJob
   {
   public:
      _coordCacheCleaner( coordResource* pRes ) ;
      virtual ~_coordCacheCleaner() ;
      virtual INT32 init() ;
      virtual const CHAR* name() const ;
      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   private:
      coordResource       *_pResource ;

   } ;
   typedef _coordCacheCleaner coordCacheCleaner ;

   INT32 coordStartCacheCleanJob( coordResource* pRes ) ;

}

#endif // COORD_CACHECLEANER_HPP__