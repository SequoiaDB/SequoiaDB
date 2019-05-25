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

   Source File Name = pmdProc.hpp

   Descriptive Name = pmdProc

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/26/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDPROC_HPP_
#define PMDPROC_HPP_

#include "ossTypes.h"
#include "oss.hpp"

namespace engine
{
   /*
      iPmdProc define
   */
   class _iPmdProc : public SDBObject
   {
   public:
      _iPmdProc() ;
      virtual ~_iPmdProc() ;
      virtual INT32 regSignalHandler() ;

   public:
      static BOOLEAN isRunning() ;
      static void stop( INT32 sigNum ) ;

   private:
      static BOOLEAN                _isRunning ;

   } ;
   typedef _iPmdProc iPmdProc ;

}

#endif // PMDPROC_HPP_

