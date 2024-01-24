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

   Source File Name = rplOptions.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_OPTIONS_HPP_
#define REPLAY_OPTIONS_HPP_

#include "ossTypes.hpp"
#include "ossIO.hpp"
#include "utilOptions.hpp"
#include "../bson/bsonobj.h"

using namespace std;
using namespace bson;

namespace replay
{
   class Options: public engine::utilOptions
   {
   public:
      Options();
      ~Options();
      INT32 parse(INT32 argc, CHAR* argv[]);
      void  printHelp();
      void  printHelpfull();
      BOOLEAN hasHelp();
      BOOLEAN hasVersion();
      BOOLEAN hasHelpfull();
      string  buildPrintableCmd(INT32 argc, CHAR* argv[]);
      string  buildBackgroundCmd(INT32 argc, CHAR* argv[]);

   public:
      OSS_INLINE const string& outputConf() const { return _outputConf; }
      OSS_INLINE const string& hostName() const { return _hostName; }
      OSS_INLINE const string& serviceName() const { return _serviceName; }
      OSS_INLINE const string& user() const { return _user; }
      OSS_INLINE const string& password() const { return _password; }
      OSS_INLINE const string& cipherfile() const { return _cipherfile; }
      OSS_INLINE const string& token() const { return _token; }
      OSS_INLINE const string& status() const { return _status; }
      OSS_INLINE const string& path() const { return _path; }
      OSS_INLINE SDB_OSS_FILETYPE pathType() const { return _pathType; }
      OSS_INLINE UINT32 intervalNum() const { return _intervalNum; }
      OSS_INLINE BOOLEAN useSSL() const { return _useSSL; }
      OSS_INLINE const BSONObj& filter() const { return _filter; }
      OSS_INLINE BOOLEAN dump() const { return _dump; }
      OSS_INLINE BOOLEAN dumpHeader() const { return _dumpHeader; }
      OSS_INLINE BOOLEAN remove() const { return _delete; }
      OSS_INLINE BOOLEAN watch() const { return _watch; }
      OSS_INLINE BOOLEAN daemon() const { return _daemon; }
      OSS_INLINE BOOLEAN debug() const { return _debug; }
      OSS_INLINE BOOLEAN deflate() const { return _deflate; }
      OSS_INLINE BOOLEAN inflate() const { return _inflate; }
      OSS_INLINE BOOLEAN isReplicaFile() const { return _isReplicaFile; }
      OSS_INLINE BOOLEAN updateWithShardingKey() const { return _updateWithShardingKey; }
      OSS_INLINE BOOLEAN isKeepShardingKey() const { return _isKeeyShardingKey; }

   private:
      INT32 setOptions( INT32 argc );

   private:
      string            _hostName;
      string            _serviceName;
      string            _user;
      string            _password;
      string            _cipherfile;
      string            _token;
      string            _status;
      string            _path;
      string            _outputConf;
      SDB_OSS_FILETYPE  _pathType;
      BOOLEAN           _useSSL;
      BSONObj           _filter;
      BOOLEAN           _dump;
      BOOLEAN           _dumpHeader;
      BOOLEAN           _delete;
      BOOLEAN           _watch;
      BOOLEAN           _daemon;
      BOOLEAN           _debug;
      BOOLEAN           _deflate;
      BOOLEAN           _inflate;
      BOOLEAN           _isReplicaFile;
      BOOLEAN           _updateWithShardingKey;
      BOOLEAN           _isKeeyShardingKey;
      UINT32            _intervalNum;
   };
}

#endif /* REPLAY_OPTIONS_HPP_ */
