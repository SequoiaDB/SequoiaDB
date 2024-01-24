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

   Source File Name = sptUsrCmdCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/24/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRCMD_COMMON_HPP_
#define SPT_USRCMD_COMMON_HPP_
#include "ossTypes.hpp"
#include <string>

using namespace std ;
namespace engine
{
   class _sptUsrCmdCommon: public SDBObject
   {
   public:
      _sptUsrCmdCommon() ;
      virtual ~_sptUsrCmdCommon() ;

      INT32 exec( const std::string& command, const std::string& env,
                  const INT32& timeout, const INT32& useShell,
                  std::string &err, std::string &retStr ) ;

      INT32 start( const std::string& command, const std::string& env,
                   const INT32& useShell, const INT32& timeout,
                   std::string &err, INT32 &pid, std::string &retStr ) ;

      INT32 getLastRet( std::string &err, UINT32 &lastRet ) ;

      INT32 getLastOut( std::string &err, string &lastOut ) ;

      INT32 getCommand( std::string &err, string &command ) ;

   private:
      UINT32         _retCode ;
      string         _strOut ;
      string         _command ;
   } ;
}
#endif
