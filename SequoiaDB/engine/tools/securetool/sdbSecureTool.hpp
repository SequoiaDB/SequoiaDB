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

   Source File Name = sdbPasswd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/19/2022  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_SECURE_TOOL_HPP_
#define SDB_SECURE_TOOL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMemPool.hpp"

using namespace std ;

class _sdbSecureTool
{
public:
   _sdbSecureTool() ;
   ~_sdbSecureTool() {}

   INT32 init( INT32 argc, CHAR* argv[] ) ;
   INT32 process() ;

private:
   INT32 _processData() ;
   INT32 _processFiles() ;
   INT32 _processOneFile( const ossPoolString& sourceFullName,
                          const ossPoolString& outputFullOrPath ) ;
   INT32 _checkSourceFilePath( ossPoolVector<ossPoolString>& sourceFiles ) ;
   INT32 _checkOutputFilePath( ossPoolString& outputFileOrPath ) ;
   INT32 _buildOutputFullFile( const ossPoolString& sourceFullName,
                               const ossPoolString& outputPath,
                               ossPoolString& outputFullFile ) ;
   void _decryptContent( const CHAR* str,
                         INT32 len,
                         ossPoolString& output ) ;

private:
   ossPoolString _decryptData ;
   BOOLEAN       _hasDecrypt ;

   ossPoolString _sourceFileOrPath ;
   BOOLEAN       _sourceIsPath ; // _sourceFileOrPath is path or file

   ossPoolString _outputFileOrPath ;
   BOOLEAN       _hasOutputPath ;
   BOOLEAN       _outputIsPath ; // _outputFileOrPath is path or file
} ;
typedef _sdbSecureTool sdbSecureTool ;

#endif
