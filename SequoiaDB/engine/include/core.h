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

   Source File Name = core.h

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CORE_H_
#define CORE_H_

#if defined(__GNUC__) || defined(__xlC__)
    #define SDB_INLINE static __OSS_INLINE__
    #define SDB_EXPORT
#else
    #define SDB_INLINE static
    #ifdef SDB_STATIC_BUILD
        #define SDB_EXPORT
    #elif defined(SDB_DLL_BUILD)
        #define SDB_EXPORT __declspec(dllexport)
    #else
        #define SDB_EXPORT __declspec(dllimport)
    #endif
#endif

#ifdef __cplusplus
#define SDB_EXTERN_C_START extern "C" {
#define SDB_EXTERN_C_END }
#else
#define SDB_EXTERN_C_START
#define SDB_EXTERN_C_END
#endif

#include "ossTypes.h"
#include "ossErr.h"

#endif /* CORE_HPP_ */
