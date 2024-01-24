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

   Source File Name = pmdSignalHandler.hpp

   Descriptive Name = Process MoDel Signal Handler Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains function used for signal
   handler.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/04/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDSIGNALHANDLER_HPP_
#define PMDSIGNALHANDLER_HPP_

#include "core.hpp"

namespace engine
{

   /*
      _signalInfo define
   */
   struct _pmdSignalInfo
   {
      const CHAR        *_name ;
      INT32             _handle ;
   } ;
   typedef _pmdSignalInfo pmdSignalInfo ;

   /*
      Tool functions
   */
   pmdSignalInfo& pmdGetSignalInfo( INT32 signum ) ;

}

#endif // PMDSIGNALHANDLER_HPP_
