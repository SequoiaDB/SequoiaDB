/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = ossTimeZone.hpp

   Descriptive Name = Operating System Services Utility Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains wrappers for utilities like
   memcpy, strcmp, etc...

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/31/2022  Yang QinCheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSTIMEZONE_HPP_
#define OSSTIMEZONE_HPP_

#include "core.h"
#include "ossTypes.h"

#define OSS_TIMEZONE_MAX_LEN 256

/*
 * Get time zone information from system file, like /etc/timezone, /etc/localtime.
 *
 * [in] len  The length of the time zone information, if you are unsure its len, you can use the
 * OSS_TIMEZONE_MAX_LEN macro.
 * [out] pTimeZone  The time zone information of current system. eg: "Asia/Shanghai"
 */
INT32 ossGetSysTimeZone( CHAR *pTimeZone, INT32 len ) ;

/*
 * Get the TZ environment variable.
 *
 * [in] len  The length of the TZ environment variable, if you are unsure its len, you can use the
 * OSS_TIMEZONE_MAX_LEN macro.
 * [out] pTimeZone  The value of the TZ environment variable. eg: "Asia/Shanghai"
 */
INT32 ossGetTZEnv ( CHAR *pTimeZone, INT32 len ) ;

/*
 * Initialize the TZ environment variable.
 */
INT32 ossInitTZEnv () ;

#endif  // OSSTIMEZONE_HPP_
