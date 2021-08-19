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

   Source File Name = dpsDump.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/12/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSDUMP_HPP__
#define DPSDUMP_HPP__

#include "oss.hpp"
#include "core.hpp"

namespace engine
{

   /*
      _dpsDump define
   */
   class _dpsDump : public SDBObject
   {
      public:
         _dpsDump () {}
         ~_dpsDump () {}

      public:

         static UINT32 dumpLogFileHead ( CHAR *inBuf, UINT32 inSize,
                                         CHAR *outBuf, UINT32 outSize,
                                         UINT32 options ) ;

   } ;
   typedef _dpsDump dpsDump ;

}

#endif //DPSDUMP_HPP__

