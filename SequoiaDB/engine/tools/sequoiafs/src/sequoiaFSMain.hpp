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

   Source File Name = sequoiaFSMain.hpp

   Descriptive Name = sequoiafs main entry.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSMAIN_H__
#define __SEQUOIAFSMAIN_H__

#include "sequoiaFS.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/xattr.h>
#include <sys/syscall.h>
#include "fuse.h"
#include "fuse_lowlevel.h"
#include "utilPidFile.hpp"
#include "ossFile.hpp"
#include "ossCmdRunner.hpp"

#ifndef SEQUOIAFS_SUPPORT_FUSE_VERSION
/*SequoiaFS support libfuse version, 0x0286 means 2.8.6*/
#define SEQUOIAFS_SUPPORT_FUSE_VERSION 0x0286
#endif

#endif

