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

   Source File Name = clsLocalValidation.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_LOCALVALIDATION_HPP_
#define CLS_LOCALVALIDATION_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"

namespace engine
{
   class _clsDiskDetector : public SDBObject
   {
   public:
      _clsDiskDetector() ;
      ~_clsDiskDetector() ;

      INT32   detect() ;
      INT32   init() ;

   private:
      INT32   _addFilePath( const CHAR* pFilePath ) ;
      INT32   _tryToWriteFile( const CHAR* pFilePath ) ;
      BOOLEAN _isNeedToDetect() ;

   private:
      ossPoolSet<ossPoolString> _filePathsSet ;
      ossPoolSet<ossPoolString>::iterator _it ;
      UINT64 _lastTick ;
      BOOLEAN _isMonitoredRole ;
      BOOLEAN _hasInit ;
   } ;

   class _clsLocalValidation : public SDBObject
   {
   public:
      INT32 run() ;

   private:
      _clsDiskDetector _diskDetector ;
   } ;
}

#endif

