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

   Source File Name = rplRecordWriter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_RECORD_WRITER_HPP_
#define REPLAY_RECORD_WRITER_HPP_

#include "rplMonitor.hpp"
#include "ossFile.hpp"
#include "ossPath.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include <sstream>

using namespace std ;
using namespace bson ;
using namespace engine ;

namespace replay
{
   class rplFileWriter : public SDBObject
   {
   public:
      rplFileWriter() ;
      ~rplFileWriter() ;

   public:
      INT32 init( const CHAR *fileName ) ;
      INT32 restore( const CHAR *fileName, INT64 size ) ;
      INT32 writeRecord( const CHAR *record ) ;
      INT32 flushAndClose() ;
      INT32 flush() ;
      const CHAR *getTmpFileName() ;
      const CHAR *getFileName() ;
      UINT64 getWriteSize() ;

   private:
      CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR _tmpFileName[ OSS_MAX_PATHSIZE + 1 ] ;
      ossFile _writer ;
      UINT64 _size ;
   } ;

   class rplRecordWriter : public SDBObject
   {
   public:
      rplRecordWriter( Monitor *monitor, const CHAR *outputDir,
                       const CHAR *prefix, const CHAR *suffix ) ;
      ~rplRecordWriter() ;

   public:
      INT32 init() ;
      INT32 writeRecord( const CHAR *dbName, const CHAR *tableName, UINT64 lsn,
                         const CHAR *record ) ;
      INT32 submit() ;
      INT32 flush() ;
      INT32 getStatus( BSONObj &status ) ;

   private:
      INT32 _restoreFile( const CHAR *tableName, const CHAR *fileName,
                          INT64 size ) ;

   private:
      // clFullName <-> rplFileWriter
      typedef map< string, rplFileWriter* > CL2WriterMap ;
      CL2WriterMap _clWriters ;

      Monitor *_monitor ;
      string _outputDir ;
      string _prefixWithConnector ;
      string _suffixWithConnector ;
   } ;
}

#endif  /* REPLAY_RECORD_WRITER_HPP_ */


