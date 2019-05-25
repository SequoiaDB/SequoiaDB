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

      std::string getErrFileName() ;

      UINT32 getErrLineno() ;

      void SetErrInfo( const std::string &filename, const UINT32 lineno ) ;

      BOOLEAN isSetErrInfo() ;

      void clearErrInfo() ;
   private:
      sptScope *_scope ;
      std::string _errFileName ;
      UINT32 _errLineno ;
      BOOLEAN _isSetErrInfo ;
   } ;

   typedef class _sptPrivateData sptPrivateData ;
}
#endif
