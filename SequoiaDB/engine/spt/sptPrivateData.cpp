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

   string _sptPrivateData::getErrFileName()
   {
      return _errFileName ;
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
