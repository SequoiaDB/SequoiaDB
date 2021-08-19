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

   Source File Name = omToolOptions.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMTOOL_OPTIONS_HPP_
#define OMTOOL_OPTIONS_HPP_

#include "utilOptions.hpp"

namespace omTool
{
   class omToolOptions: public engine::utilOptions
   {
   public:
      omToolOptions() ;
      ~omToolOptions() ;

      BOOLEAN hasHelp();

      BOOLEAN hasVersion();

      INT32 parse( INT32 argc, CHAR* argv[] ) ;

      /* General */
      inline const string& mode() const { return _mode ; }

      /* HostFiles */
      inline const string& hostname() const { return _hostname ; }
      inline const string& ip() const { return _ip ; }

      /* Directories */
      inline const string& path() const { return _path ; }
      inline const string& user() const { return _user ; }

   private:
      INT32 _checkOptions();

   private:
      string _mode ;

      /* HostFiles */
      string _hostname ;
      string _ip ;

      /* Directories */
      string _path ;
      string _user ;
   } ;
}

#endif /* OMTOOL_OPTIONS_HPP_ */