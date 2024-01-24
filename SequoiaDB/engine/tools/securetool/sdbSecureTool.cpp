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
          05/10/2022  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbSecureTool.hpp"
#include <iostream>
#include "ossVer.hpp"
#include "utilParam.hpp"
#include "pdSecure.hpp"
#include "utilCommon.hpp"
#include "ossFile.hpp"
#include "ossPath.hpp"
#include "pd.hpp"

using namespace engine ;

#define SECURE_OPTIONS_HELP         ( "help" )
#define SECURE_OPTIONS_VERSION      ( "version" )
#define SECURE_OPTIONS_SOURCE       ( "source" )
#define SECURE_OPTIONS_OUTPUT       ( "output" )
#define SECURE_OPTIONS_DECRYPT      ( "decrypt" )

#define COMMANDS_ADD_PARAM_OPTIONS_BEGIN(des)  des.add_options()
#define COMMANDS_ADD_PARAM_OPTIONS_END         ;
#define COMMANDS_STRING(a, b)                  (string(a) + string(b)).c_str()

#define SECURE_GENERAL_OPTIONS \
   (COMMANDS_STRING(SECURE_OPTIONS_HELP,    ",h"), "help") \
   (COMMANDS_STRING(SECURE_OPTIONS_VERSION, ",v"), "version") \
   (COMMANDS_STRING(SECURE_OPTIONS_SOURCE,  ",s"), po::value<string>(), "specify source diaglog file or path" ) \
   (COMMANDS_STRING(SECURE_OPTIONS_OUTPUT,  ",o"), po::value<string>(), "specify output file or path, default: source path" ) \
   (COMMANDS_STRING(SECURE_OPTIONS_DECRYPT, ",d"), po::value<string>(), "decrypt data")

#define SECURE_OUTPUT_FILE_SUFFIX   PD_DFT_LOG_DECRYPT_SUFFIX

_sdbSecureTool::_sdbSecureTool()
{
   _hasDecrypt = FALSE ;
   _sourceIsPath = FALSE ;
   _hasOutputPath = FALSE ;
   _outputIsPath = FALSE ;
}

INT32 _sdbSecureTool::init( INT32 argc, CHAR* argv[] )
{
   INT32 rc = SDB_OK ;
   po::variables_map vm ;
   po::options_description desc( "Command options" ) ;

   COMMANDS_ADD_PARAM_OPTIONS_BEGIN ( desc )
        SECURE_GENERAL_OPTIONS
   COMMANDS_ADD_PARAM_OPTIONS_END

   rc = utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( vm.empty() || vm.count( SECURE_OPTIONS_HELP ) )
   {
      // print help information
      cout << desc << endl ;
      cout << "Example:" << endl ;
      cout << "  ./bin/sdbsecuretool -d \"SDBSECURE0000(hfIcCqboSWK2)\"" << endl ;
      cout << endl ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( vm.count( SECURE_OPTIONS_VERSION ) )
   {
      ossPrintVersion( "sdbsecuretool" ) ;
      rc = SDB_SDB_VERSION_ONLY ;
      goto done ;
   }

   if ( vm.count( SECURE_OPTIONS_DECRYPT ) &&
        vm.count( SECURE_OPTIONS_SOURCE ) )
   {
      rc = SDB_INVALIDARG ;
      cerr << "sdbsecuretool can't specify --decrpty and --source "
           << "at the same time" << endl ;
      goto error ;
   }
   if ( !vm.count( SECURE_OPTIONS_DECRYPT ) &&
        !vm.count( SECURE_OPTIONS_SOURCE ) )
   {
      rc = SDB_INVALIDARG ;
      cerr << "sdbsecuretool should specify --decrpty or --source " << endl ;
      goto error ;
   }
   if ( vm.count( SECURE_OPTIONS_DECRYPT ) &&
        vm.count( SECURE_OPTIONS_OUTPUT ) )
   {
      rc = SDB_INVALIDARG ;
      cerr << "sdbsecuretool can't specify --decrpty and --output "
           << "at the same time" << endl ;
      goto error ;
   }

   if ( vm.count( SECURE_OPTIONS_DECRYPT ) )
   {
      _decryptData = vm[SECURE_OPTIONS_DECRYPT].as<string>().c_str() ;
      _hasDecrypt = TRUE ;
   }

   if ( vm.count( SECURE_OPTIONS_SOURCE ) )
   {
      _sourceFileOrPath = vm[SECURE_OPTIONS_SOURCE].as<string>().c_str() ;
   }

   if ( vm.count( SECURE_OPTIONS_OUTPUT ) )
   {
      _outputFileOrPath = vm[SECURE_OPTIONS_OUTPUT].as<string>().c_str() ;
      _hasOutputPath = TRUE ;
   }

done:
   return rc ;
error:
   goto done ;
}

void _sdbSecureTool::_decryptContent( const CHAR* str, INT32 len,
                                      ossPoolString& output )
{
   output.clear() ;

   const CHAR* findStr = str ;
   while ( findStr < ( str + len ) )
   {
      const CHAR* findHeadStr = ossStrstr( findStr, UTIL_SECURE_HEAD ) ;
      if ( findHeadStr == NULL )
      {
         // If we don't find "SDBSECURE", just use original content
         output.append( findStr ) ;
         break ;
      }
      else
      {
         // If we find "SDBSECURE", first use the content before "SDBSECURE"
         output.append( findStr, findHeadStr - findStr ) ;

         // Find end position of cipher text by terminator ')' and new line.
         // The cipher text may hasn't ')', because it is too long and truncated
         const CHAR* findNewLineStr = ossStrstr( findHeadStr, OSS_NEWLINE ) ;
         const CHAR* findEndStr = ossStrstr( findHeadStr,
                                             UTIL_SECURE_END_SYMBOL_STR ) ;
         if ( findEndStr == NULL )
         {
            // If we don't find ')', then treat new line as the ending character
            // of cipher text. If we don't find new line, then treat the last
            // character as the ending character of cipher text.
            if ( findNewLineStr == NULL )
            {
               findEndStr = str + len - 1 ;
            }
            else
            {
               // not include new line
               findEndStr = findNewLineStr - 1 ;
            }
         }
         else if ( findNewLineStr != NULL &&
                   findEndStr > findNewLineStr )
         {
            // If the new line appears first, and then the ')' appears, we
            // treat new line as the ending character of cipher text
            findEndStr = findNewLineStr - 1 ;
         }

         ossPoolString ciphText( findHeadStr, findEndStr - findHeadStr + 1 ) ;
         ossPoolString plainText ;
         INT32 rc = utilSecureDecrypt( ciphText, plainText ) ;
         if ( SDB_OK == rc )
         {
            output += plainText ;
         }
         else if ( SDB_INVALIDARG == rc )
         {
            output += "(ERROR: Invalid data to be decrypted)" ;
         }
         else
         {
            output += "(ERROR: Failed to decrypt, rc: " ;
            output += rc ;
            output += ")" ;
         }

         findStr = findEndStr + 1 ;
      }
   }
}

INT32 _sdbSecureTool::_checkSourceFilePath( ossPoolVector<ossPoolString>& sourceFiles  )
{
   INT32 rc = SDB_OK ;
   SDB_OSS_FILETYPE fileType = SDB_OSS_UNK ;
   CHAR sourceFullPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   // get real path for souce path
   CHAR *p = ossGetRealPath( _sourceFileOrPath.c_str(), sourceFullPath,
                             OSS_MAX_PATHSIZE ) ;
   if ( NULL == p )
   {
      cerr << "Failed to get real path for source: "
           <<  _sourceFileOrPath.c_str() << endl ;
      rc = SDB_INVALIDPATH ;
      goto error;
   }

   // check source path exist or not
   rc = ossAccess( sourceFullPath ) ;
   if ( SDB_FNE == rc )
   {
      cerr << "Source file/path doesn't exist: " << sourceFullPath << endl ;
      goto error;
   }
   else if ( rc )
   {
      cerr << "Failed to check existence of source file/path["
           <<  sourceFullPath << "], rc: " << rc << endl ;
      goto error;
   }

   // check it is path or file
   rc = ossGetPathType( sourceFullPath, &fileType ) ;
   if ( rc )
   {
      cerr << "Failed to get source file/path[" << sourceFullPath
           << "] type, rc: " << rc << endl ;
      goto error;
   }
   if ( SDB_OSS_FIL == fileType )
   {
      sourceFiles.push_back( sourceFullPath ) ;
   }
   else if ( SDB_OSS_DIR == fileType )
   {
      _sourceIsPath = TRUE ;

      // find all diaglog file in the source path
      multimap< string, string > mapFiles ;
      rc = ossEnumFiles( sourceFullPath, mapFiles, PD_DFT_DIAGLOG"*" ) ;
      if ( rc )
      {
         cerr << "Failed to list files in source path[" << sourceFullPath
              << "], rc: " << rc << endl ;
      }

      for ( multimap< string, string >::iterator it = mapFiles.begin() ;
            it != mapFiles.end() ; ++it )
      {
         // NOTE: first is file name, second is full name
         string& fileFullName = it->second ;
         // exclude the decrypted files
         size_t pos = fileFullName.find_last_of( '.' ) ;
         if ( 0 != ossStrcmp( fileFullName.c_str() + pos,
                              SECURE_OUTPUT_FILE_SUFFIX ) )
         {
            sourceFiles.push_back( fileFullName.c_str() ) ;
         }
      }
   }
   else
   {
      cerr << "Source must be file or path: " <<  sourceFullPath << endl ;
      rc = SDB_INVALIDPATH ;
      goto error;
   }


done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbSecureTool::_checkOutputFilePath( ossPoolString& outputFileOrPath )
{
   INT32 rc = SDB_OK ;
   SDB_OSS_FILETYPE fileType = SDB_OSS_UNK ;
   CHAR outputFullPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   // --output default value is source file path
   if ( !_hasOutputPath )
   {
      if ( _sourceIsPath )
      {
         _outputFileOrPath = _sourceFileOrPath ;
      }
      else
      {
         _outputFileOrPath = _sourceFileOrPath + SECURE_OUTPUT_FILE_SUFFIX ;
      }
   }

   // get real path for output path
   CHAR *p = ossGetRealPath( _outputFileOrPath.c_str(), outputFullPath,
                             OSS_MAX_PATHSIZE ) ;
   if ( NULL == p )
   {
      cerr << "Failed to get real path for output: "
           << _outputFileOrPath.c_str() << endl ;
      rc = SDB_INVALIDPATH ;
      goto error ;
   }

   // check output path exist or not
   rc = ossAccess( outputFullPath ) ;
   if ( SDB_FNE == rc )
   {
      // if not exist, treat it as a file
      outputFileOrPath = outputFullPath ;
      _outputIsPath = FALSE ;
      rc = SDB_OK ;
      goto done ;
   }
   else if ( rc )
   {
      cerr << "Failed to check existence of output file/path["
           << outputFullPath << "], rc: " << rc << endl ;
      goto error;
   }

   // it exists, only path can be
   rc = ossGetPathType( outputFullPath, &fileType ) ;
   if ( rc )
   {
      cerr << "Failed to get output file/path[" << outputFullPath
           << "] type, rc: " << rc << endl ;
      goto error ;
   }
   if ( SDB_OSS_FIL == fileType )
   {
      cerr << "Output file already exist: " << outputFullPath << endl ;
      rc = SDB_INVALIDPATH ;
      goto error ;
   }
   else if ( SDB_OSS_DIR == fileType )
   {
      outputFileOrPath = outputFullPath ;
      _outputIsPath = TRUE ;
      goto done ;
   }
   else
   {
      cerr << "Output must be file or path: " <<  outputFullPath << endl ;
      rc = SDB_INVALIDPATH ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbSecureTool::_buildOutputFullFile( const ossPoolString& sourceFullName,
                                            const ossPoolString& outputPath,
                                            ossPoolString& outputFullFile )
{
   INT32 rc = SDB_OK ;

   // source file name + ".decrypt" = output file name
   string sourceFileName = ossFile::getFileName( sourceFullName.c_str() ) ;
   outputFullFile = outputPath ;
   outputFullFile += OSS_FILE_SEP ;
   outputFullFile += sourceFileName.c_str() ;
   outputFullFile += SECURE_OUTPUT_FILE_SUFFIX ;

   return rc ;
}

INT32 _sdbSecureTool::_processOneFile( const ossPoolString& sourceFullName,
                                       const ossPoolString& outputFullOrPath )
{
   INT32 rc = SDB_OK ;
   ossFile sFile ;
   ossFile oFile ;
   INT64 fileSize = 0 ;
   INT64 readSize = 0 ;
   CHAR* readBuf = NULL ;
   ossPoolString outputContent ;
   ossPoolString outputFullFile ;

   // open source file
   rc = sFile.open( sourceFullName.c_str(), OSS_READONLY, OSS_DEFAULTFILE ) ;
   if ( rc )
   {
      cerr << "Failed to open source file["
           << sourceFullName.c_str() << "], rc: " << rc << endl ;
      goto error;
   }

   // check source file size, and malloc memory
   rc = sFile.getFileSize( fileSize ) ;
   if ( rc )
   {
      cerr << "Failed to get size of source file["
           << sourceFullName.c_str() << "], rc: " << rc << endl ;
      goto error;
   }

   if ( fileSize > ( PD_DFT_FILE_SZ * 1024 * 1024 ) )
   {
      rc = SDB_INVALIDPATH ;
      cerr << "Souce file is too large: " << sourceFullName.c_str() << endl ;
      goto error ;
   }

   readBuf = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
   if ( !readBuf )
   {
      rc = SDB_OOM ;
      cerr << "Out of memory" << endl ;
      goto error ;
   }
   ossMemset( readBuf, 0, fileSize + 1 ) ;

   // build output file full name
   if ( !_outputIsPath )
   {
      outputFullFile = outputFullOrPath ;
   }
   else
   {
      rc = _buildOutputFullFile( sourceFullName, outputFullOrPath,
                                 outputFullFile ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   // create output file
   rc = oFile.open( outputFullFile.c_str(),
                    OSS_READWRITE|OSS_EXCLUSIVE|OSS_CREATEONLY,
                    OSS_RU|OSS_WU|OSS_RG|OSS_RO ) ;
   if ( SDB_FE == rc )
   {
      cerr << "Output file already exist: " << outputFullFile.c_str() << endl ;
      goto error ;
   }
   else if ( rc )
   {
      cerr << "Failed to open output File["
           << outputFullFile.c_str() << "], rc: " << rc << endl ;
      goto error ;
   }

   // read and content from source file
   rc = sFile.readN( readBuf, fileSize, readSize ) ;
   if ( rc )
   {
      cerr << "Failed to read source file["
           << sourceFullName.c_str() << "], rc: " << rc << endl ;
      goto error;
   }

   // decrypt content
   _decryptContent( readBuf, fileSize, outputContent ) ;

   // write to output file
   rc = oFile.writeN( outputContent.c_str(), outputContent.size() ) ;
   if ( rc )
   {
      cerr << "Failed to write output file["
           << outputFullFile.c_str() << "], rc: " << rc << endl ;
      goto error;
   }

   cout << "Generate decrypted file: " << outputFullFile.c_str() << endl ;

done:
   if ( readBuf )
   {
      SDB_OSS_FREE( readBuf ) ;
      readBuf = NULL ;
   }
   {
      INT32 rc1 = sFile.close() ;
      if ( rc1 )
      {
         cerr << "Failed to close source file["
              << sourceFullName.c_str() << "], rc: " << rc1 << endl ;
      }
      rc1 = oFile.close() ;
      if ( rc1 )
      {
         cerr << "Failed to close output file["
              << outputFullFile.c_str() << "], rc: " << rc1 << endl ;
      }
   }
   return rc ;
error:
   goto done ;
}

INT32 _sdbSecureTool::_processFiles()
{
   INT32 rc = SDB_OK ;
   ossPoolVector<ossPoolString> sourceFiles ;
   ossPoolString outputFileOrPath ;

   rc = _checkSourceFilePath( sourceFiles ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _checkOutputFilePath( outputFileOrPath ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( sourceFiles.size() > 1 && !_outputIsPath )
   {
      rc = SDB_INVALIDPATH ;
      cerr << "There are multiple source files, --output can't be a file: "
           << _outputFileOrPath.c_str() << endl ;
      goto error ;
   }

   for ( ossPoolVector<ossPoolString>::iterator it = sourceFiles.begin() ;
         it != sourceFiles.end() ; ++it )
   {
      rc = _processOneFile( *it, outputFileOrPath ) ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbSecureTool::_processData()
{
   INT32 rc = SDB_OK ;

   ossPoolString plainText ;
   rc = utilSecureDecrypt( _decryptData, plainText ) ;
   if ( rc )
   {
      if ( SDB_INVALIDARG == rc )
      {
         cerr << "Invalid data format: " << _decryptData << endl ;
      }
      else
      {
         cerr << "Failed to decrypt data, rc: " << rc << endl ;
      }
   }
   else
   {
      cout << plainText.c_str() << endl ;
   }

   return rc ;
}

INT32 _sdbSecureTool::process()
{
   INT32 rc = SDB_OK ;

   if ( _hasDecrypt )
   {
      rc = _processData() ;
   }
   else
   {
      rc = _processFiles() ;
   }

   return rc ;
}

INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   _sdbSecureTool tool ;

   rc = tool.init( argc, argv ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = tool.process() ;
   if ( rc )
   {
      goto error ;
   }

done:
   return engine::utilRC2ShellRC( rc ) ;
error:
   goto done ;
}
