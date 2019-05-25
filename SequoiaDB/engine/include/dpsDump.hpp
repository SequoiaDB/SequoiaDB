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

