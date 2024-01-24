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

   Source File Name = sptSPInfo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_PRIVATE_DATA_HPP_
#define SPT_PRIVATE_DATA_HPP_
#include "sptScope.hpp"
#include "oss.hpp"
#include <iostream>

namespace engine
{
   class _sptPrivateData:public SDBObject
   {
   public:
      _sptPrivateData() ;

      _sptPrivateData( sptScope *pScope ) ;

      virtual ~_sptPrivateData() ;

      sptScope *getScope() ;

      const CHAR* getErrFileName() ;

      UINT32 getErrLineno() ;

      void SetErrInfo( const std::string &filename, const UINT32 lineno ) ;

      BOOLEAN isSetErrInfo() ;

      void clearErrInfo() ;
   private:
      // _scope can't be modified after init
      sptScope *_scope ;
      std::string _errFileName ;
      UINT32 _errLineno ;
      BOOLEAN _isSetErrInfo ;
   } ;

   typedef class _sptPrivateData sptPrivateData ;
}
#endif
