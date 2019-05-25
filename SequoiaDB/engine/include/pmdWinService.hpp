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

   Source File Name = pmdDaemon.hpp

   Descriptive Name = pmdDaemon

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/09/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDWINSERVICE_HPP__
#define PMDWINSERVICE_HPP__
#include "ossFeat.h"

#if defined (_WINDOWS)

#include "ossTypes.h"

typedef  INT32 (*PMD_WINSERVICE_FUNC)( INT32 argc, CHAR **argv ) ;

#define PMD_WINSVC_SVCNAME_MAX_LEN     255

INT32 WINAPI pmdWinstartService( const CHAR *pServiceName,
                        PMD_WINSERVICE_FUNC svcFun );

#endif //#if defined (_WINDOWS)

#endif //#define PMDWINSERVICE_HPP__