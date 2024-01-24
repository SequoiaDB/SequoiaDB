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

   Source File Name = sptSPInfo.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptPrivateData.hpp"

using namespace std ;
namespace engine
{
   _sptPrivateData::_sptPrivateData():
      _scope( NULL ), _errLineno( 0 ), _isSetErrInfo( FALSE )
   {
   }

   _sptPrivateData::_sptPrivateData( sptScope *pScope ):
      _scope( pScope ), _errLineno( 0 ), _isSetErrInfo( FALSE )
   {
   }

   _sptPrivateData::~_sptPrivateData()
   {
      _scope = NULL ;
   }

   sptScope* _sptPrivateData::getScope()
   {
      return _scope ;
   }

   const CHAR* _sptPrivateData::getErrFileName()
   {
      return _errFileName.c_str() ;
   }

   UINT32 _sptPrivateData::getErrLineno()
   {
      return _errLineno ;
   }

   void _sptPrivateData::SetErrInfo( const std::string &filename,
                                     const UINT32 lineno )
   {
      _errFileName = filename ;
      _errLineno = lineno ;
      _isSetErrInfo = TRUE ;
   }

   BOOLEAN _sptPrivateData::isSetErrInfo()
   {
      return _isSetErrInfo ;
   }

   void _sptPrivateData::clearErrInfo()
   {
      _errFileName.clear() ;
      _errLineno = 0 ;
      _isSetErrInfo = FALSE ;
   }

}
