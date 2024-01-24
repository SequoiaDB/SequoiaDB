/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = revertOptions.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REVERT_OPTIONS_HPP_
#define REVERT_OPTIONS_HPP_

#include "ossTypes.hpp"
#include "../bson/bson.hpp"
#include "dpsDef.hpp"
#include "utilOptions.hpp"
#include <vector>
#include <string>
#include <ctime>

using namespace std ; 

namespace sdbrevert
{
   enum SDB_REVERT_DATA_TYPE
   {
      SDB_REVERT_DOC = 0,
      SDB_REVERT_LOB,
      SDB_REVERT_ALL
   } ;
   
   class revertOptions: public engine::utilOptions {
      public:
         revertOptions() ;
         ~revertOptions() ;
         INT32 parse( INT32 argc, CHAR* argv[] ) ;
         void  printHelpInfo() ;
         BOOLEAN hasHelp() ;
         BOOLEAN hasVersion() ;
   
      public:
         OSS_INLINE const vector<string>& hostList() const { return _hostList ; }
         OSS_INLINE const string& userName() const { return _userName ; }
         OSS_INLINE const string& password() const { return _password ; }
         OSS_INLINE const string& cipherfile() const { return _cipherfile ; }
         OSS_INLINE const string& token() const { return _token ; }
         OSS_INLINE BOOLEAN useSSL() const { return _useSSL ; }
         OSS_INLINE const string& targetClName() const { return _targetClName ; }
         OSS_INLINE const vector<string>& logPathList() const { return _logPathList ; }
         OSS_INLINE SDB_REVERT_DATA_TYPE dataType() const { return _dataType ; }
         OSS_INLINE const bson::BSONObj& matcher() const { return _matcher ; }
         OSS_INLINE const time_t& startTime() const { return _startTime ; }
         OSS_INLINE const time_t& endTime() const { return _endTime ; }
         OSS_INLINE DPS_LSN_OFFSET startLSN() const { return _startLSN ; }
         OSS_INLINE DPS_LSN_OFFSET endLSN() const { return _endLSN ; }
         OSS_INLINE const string& outputClName() const { return _outputClName ; }
         OSS_INLINE const string& label() const { return _label ; }
         OSS_INLINE INT32 jobs() const { return _jobs ; }
         OSS_INLINE BOOLEAN stop() const { return _stop ; }
         OSS_INLINE BOOLEAN fillLobHole() const { return _fillLobHole ; }
         OSS_INLINE const string& tempPath() const { return _tmpPath ; }

      private:
         INT32 _setOptions( INT32 argc ) ;
         INT32 _check() ;
   
      private:
         vector<string>       _hostList ;
         string               _userName ;
         string               _password ;
         string               _cipherfile ;
         string               _token ;
         BOOLEAN              _useSSL ;
         string               _targetClName ;
         vector<string>       _logPathList ;
         SDB_REVERT_DATA_TYPE _dataType ;
         bson::BSONObj        _matcher ;
         time_t               _startTime ;
         time_t               _endTime ;
         DPS_LSN_OFFSET       _startLSN ;
         DPS_LSN_OFFSET       _endLSN ;
         string               _outputClName ;
         string               _label ;
         INT32                _jobs ;
         BOOLEAN              _stop ;
         BOOLEAN              _fillLobHole ;
         string               _tmpPath ;
   } ;
}

#endif /* REVERT_OPTIONS_HPP_ */
