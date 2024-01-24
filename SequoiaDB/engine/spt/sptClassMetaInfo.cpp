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

   Source File Name = sptClassMetaInfo.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptClassMetaInfo.hpp"
#include "ossIO.hpp"
#include "ossPath.hpp"
#include "ossFile.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "../util/fromjson.hpp"
#include "utilStr.hpp"

#include <iostream>
#include <sstream>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>

using std::cout ;
using std::endl ;
namespace fs = boost::filesystem ;

#if defined ( _WINDOWS )
#define SPT_SDB_SHELL_TROFF_DIR   "..\\doc\\manual\\"
#define DIRECTORY_DELIMITER       "\\"
#else
#define SPT_SDB_SHELL_TROFF_DIR   "../doc/manual/"
#define DIRECTORY_DELIMITER       "/"
#endif

#define SPT_TROFF_SUFFIX_EN       "_en.troff"
#define SPT_TROFF_SUFFIX_CN       "_cn.troff"

#define SPT_TROFF_NAME_EN         ".SH NAME"
#define SPT_TROFF_NAME_CN         ".SH 名称"
#define SPT_TROFF_SYNOPSIS_EN     ".SH SYNOPSIS"
#define SPT_TROFF_SYNOPSIS_CN     ".SH 语法"
#define SPT_TROFF_CATEGORY_EN     ".SH CATEGORY"
#define SPT_TROFF_CATEGORY_CN     ".SH 类别"

#define PRINT_ERROR               cout << ss.str().c_str() << endl ;
#define ERROR_END                 "(" << __FILE__ << ":" << __LINE__ << ")"

static const CHAR* _uselessMarks[] = { SPT_TROFF_SYNOPSIS_EN,
                                       SPT_TROFF_SYNOPSIS_CN,
                                       SPT_TROFF_CATEGORY_EN,
                                       SPT_TROFF_CATEGORY_CN,
                                       "\r", ".PP", "\\f[B]", "\\f[I]", 
                                       "\\f[]", "\\f[]" } ;

static const CHAR* _uselessMarks2[] = { "\r", "\n" } ;

namespace engine
{

   INT32 _checkBuffer ( CHAR **ppBuff, INT64 *pBuffLen, INT64 length )
   {
      INT32 rc = SDB_OK ;
      if ( length > *pBuffLen )
      {
         CHAR *pOld = *ppBuff ;
         INT32 newSize = length + 1 ;
         *ppBuff = (CHAR*)SDB_OSS_REALLOC ( *ppBuff, sizeof(CHAR)*(newSize)) ;
         if ( !*ppBuff )
         {
            rc = SDB_OOM ;
            *ppBuff = pOld ;
            goto error ;
         }
         *pBuffLen = newSize ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _readFile( const CHAR *pFilePath, CHAR **ppBuff, INT64 *pBuffLen )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasOpen = FALSE ;
      INT64 fileSize = 0 ;
      SINT64 readSize = 0 ;
      OSSFILE file ;
      stringstream ss ;

      if ( !ppBuff )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = ossAccess( pFilePath ) ;
      if ( rc )
      {
         ss << "Failed to access file[" << pFilePath << "], rc = "
            << rc << ERROR_END ;
         goto error ;
      }
      // get contents from config file
      rc = ossOpen( pFilePath, OSS_READONLY | OSS_SHAREREAD, 0, file ) ;
      if ( rc )
      {
         ss << "Failed to open file[" << pFilePath << "], rc = "
            << rc << ERROR_END ;
         goto error ;
      }
      hasOpen = TRUE ;
      rc = ossGetFileSize( &file, &fileSize ) ;
      if ( rc )
      {
         ss << "Failed to get the size of file[" << pFilePath
            << "], rc = " << rc << ERROR_END ;
         goto error ;
      }
      rc = _checkBuffer( ppBuff, pBuffLen, fileSize ) ;
      if ( rc )
      {
         ss << "Failed to check the size of buffer when read[" 
            << pFilePath << "], rc = " << rc << ERROR_END ;
         goto error ;
      }
      rc = ossReadN( &file, fileSize, *ppBuff, readSize ) ;
      if ( rc )
      {
         ss << "Failed to read content from file[" 
            << pFilePath << "], rc = " << rc << ERROR_END ;
      }
      
   done:
      if ( hasOpen )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      cout << ss.str().c_str() << endl ;
      goto done ;
   }

   void _filterMarks( CHAR *pBuff, const CHAR **marks, INT32 num )
   {
      INT32 i = 0 ;
      INT32 left = 0 ;
      CHAR *pos = NULL ;
      CHAR *end = NULL ;

      for ( ; i < num; i++ )
      {
         while( TRUE )
         {
            pos = ossStrstr( pBuff, marks[i] ) ;
            if ( NULL == pos )
            {
               break ;
            }
            end = pos + ossStrlen( marks[i] ) ;
            left = ossStrlen( end ) ;
            ossMemmove( pos, end, left ) ;
            pos[left] = '\0' ;
         }
      }
   }

   void _replaceCharsWithSpace( CHAR *pBuff, const CHAR *c )
   {
      CHAR *pos = NULL ;

      while( TRUE )
      {
         pos = ossStrstr( pBuff, c ) ;
         if ( NULL == pos )
         {
            break ;
         }
         ossMemset( pos, ' ', ossStrlen(c) ) ;
      }
   }

   _sptClassMetaInfo::_sptClassMetaInfo() 
   {
      _lang = SPT_LANG_EN ;
      _initOK = FALSE ;
   }

   _sptClassMetaInfo::_sptClassMetaInfo( const string &lang ) 
   {
      _lang = lang == "cn" ? SPT_LANG_CN : SPT_LANG_EN ;
      _initOK = SDB_OK == _init() ? TRUE : FALSE ;
   }

   // fuzzyFuncName maybe "xxx::yyy" or "xxx" or "yyy"
   // when fuzzyFuncName is "xxx" or "yyy", use matcher to determine
   // which class the function is belong to.
   INT32 _sptClassMetaInfo::queryFuncInfo( const string &fuzzyFuncName,
                                           const string &matcher,
                                           BOOLEAN isInstance,
                                           vector<string> &output )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      string lfuzzyFuncName = fuzzyFuncName ;
      string lmatcher = matcher ;
      vector<string> vec ;
      vector<string>::iterator it ;
      boost::to_lower( lfuzzyFuncName ) ;
      boost::to_lower( lmatcher ) ;
      
      if ( !_initOK )
      {
         rc = SDB_SYS ;
         ss << "Failed to init info for help, rc = " << rc << ERROR_END ;
         goto error ;
      }
     
      // in case fuzzyFuncName is "xxx" or "yyy"
      if ( std::string::npos == lfuzzyFuncName.find( SPT_CLASS_SEPARATOR ) )
      {
         for ( it = _functions.begin(); it != _functions.end(); it++ )
         {
            string lfullName = *it ;
            boost::to_lower( lfullName ) ;
            boost::split( vec, lfullName, 
               boost::is_any_of( SPT_CLASS_SEPARATOR ), 
               boost::token_compress_on ) ;
            if ( vec.size() < 2 )
            {
               rc = SDB_SYS ;
               ss << "Invalid function name[" << *it << "], rc = " 
                  << rc << ERROR_END ;
               goto error ;
            }
            if ( ( lmatcher.empty() || vec[0].compare( lmatcher ) ==0 ) &&
                 std::string::npos != vec[1].find( lfuzzyFuncName ) )
            {
               output.push_back( *it ) ;
            }
         }
      }
      else // in case fuzzyFuncName is "xxx::yyy"
      {
         string lfuzzyClsName1 ;
         string lfuzzyClsName2 ;
         string lfuzzyFuncName1 ;
         string lfuzzyFuncName2 ;
         boost::split( vec, lfuzzyFuncName,
            boost::is_any_of( SPT_CLASS_SEPARATOR ), 
            boost::token_compress_on ) ;
         lfuzzyClsName1 = vec[0] ;
         lfuzzyFuncName1 = vec[1] ;
         for ( it = _functions.begin(); it != _functions.end(); it++ )
         {
            vec.clear() ;
            boost::split( vec, *it,
               boost::is_any_of( SPT_CLASS_SEPARATOR ), boost::token_compress_on ) ;
            if ( vec.size() < 2 )
            {
               rc = SDB_SYS ;
               ss << "Invalid function name[" << *it << "], rc = " 
                  << rc << ERROR_END ;
               goto error ;
            }
            lfuzzyClsName2 = vec[0] ;
            lfuzzyFuncName2 = vec[1] ;
            boost::to_lower( lfuzzyClsName2 ) ;
            boost::to_lower( lfuzzyFuncName2 ) ;
            if ( std::string::npos != lfuzzyClsName2.find( lfuzzyClsName1 ) &&
                 std::string::npos != lfuzzyFuncName2.find( lfuzzyFuncName1 ) )
            {
               output.push_back( *it ) ;
            }
         }
      }
      
   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   // funcName must be "xxxx::yyyy", e.g. "Oma::createCoord"
   INT32 _sptClassMetaInfo::getTroffFile( const string &fullName, string &path )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      MAP_FUNC_META_INFO_IT map_meta_it ;
      string lname ;
      string lfullName = fullName ;
      boost::to_lower( lfullName ) ;

      if ( !_initOK )
      {
         rc = SDB_SYS ;
         ss << "Failed to init info for help, rc = " << rc << ERROR_END ;
         goto error ;
      }
 
      for ( map_meta_it = _map_func_meta_info.begin(); 
            map_meta_it != _map_func_meta_info.end(); map_meta_it++ )
      {
         string lclassName = map_meta_it->first ;
         const vector<sptFuncMetaInfo> &vec_func = map_meta_it->second ;
         vector<sptFuncMetaInfo>::const_iterator it = vec_func.begin() ;

         boost::to_lower( lclassName ) ;
         if ( std::string::npos == lfullName.find( lclassName ) )
         {
            continue ;
         }
         for ( ; it != vec_func.end(); it++ )
         {
            lname = lclassName + SPT_CLASS_SEPARATOR + it->funcName ;
            boost::to_lower( lname ) ;
            if ( lfullName == lname )
            {
               path = it->path ;
               goto done ;
            }
         }
      }

      rc = SDB_SYS ;
      ss << "No troff file for function[" << fullName 
         << "], rc = " << rc << ERROR_END ;
      goto error ;
      
   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }
   
   INT32 _sptClassMetaInfo::getMetaInfo( const string &className, 
                                         INT32 type,
                                         vector<sptFuncMetaInfo> &output )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      MAP_FUNC_META_INFO_IT map_meta_it = _map_func_meta_info.find( className ) ;

      if ( !_initOK )
      {
         rc = SDB_SYS ;
         ss << "Failed to init info for help, rc = " << rc << ERROR_END ;
         goto error ;
      }

      if ( _map_func_meta_info.end() == map_meta_it )
      {
         rc = SDB_SYS ;
         ss << "No function info for class["<< className 
            << "], rc = " << rc << ERROR_END ;
         goto error ;
      }
      else
      {
         const vector<sptFuncMetaInfo> &vec = map_meta_it->second ;
         vector<sptFuncMetaInfo>::const_iterator it = vec.begin() ;
         INT32 t = 0 ;
         for ( ; it != vec.end(); it++ )
         {
            t = it->funcType ;
            if ( t & type )
            {
               output.push_back( *it ) ;          
            }
         }
      }
      

   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   INT32 _sptClassMetaInfo::_init()
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;

      _map_func_def_info = sptFuncDef::getInstance().getFuncDefInfo() ;
      if ( _map_func_def_info.size() <= 0 )
      {
         rc = SDB_SYS ;
         ss << "No function definition information, rc = " 
            << rc << ERROR_END ;
         goto error ;
      }

      rc = _loadTroffFile() ;
      if ( rc )
      {
         ss << "Failed to load contents from troff file, rc = " 
            << rc << ERROR_END ;
         goto error ;
      }

      rc = _extractTroffInfo() ;
      if ( rc )
      {
         ss << "Failed to extract info from troff file, rc = " 
            << rc << ERROR_END ;
         goto error ;
      }

      _mergeMetaInfo() ;

   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   INT32 _sptClassMetaInfo::_loadTroffFile()
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      CHAR filePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      vector<string> vec ;
      multimap< string, string > mapFiles ;
      multimap< string, string >::iterator it ;
      string filter = string("*") + 
         (_lang == SPT_LANG_EN ? SPT_TROFF_SUFFIX_EN : SPT_TROFF_SUFFIX_CN) ;

      // load file
      rc = ossGetEWD( filePath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ss << "Failed to current work directory, rc = " << rc << ERROR_END ;
         goto error ;
      }
      rc = utilCatPath( filePath, OSS_MAX_PATHSIZE, SPT_SDB_SHELL_TROFF_DIR ) ;
      if ( rc )
      {
         ss << "Failed to cat path[" << SPT_SDB_SHELL_TROFF_DIR
            << "], rc = " << rc << ERROR_END ;
         goto error ;
      }
      rc = ossAccess( filePath ) ;
      if ( rc )
      {
         ss << "Failed to access file[" << filePath << "], rc = " 
            << rc << ERROR_END ;
         goto error ;
      }
      // get files from the specified directory
      rc = ossEnumFiles( filePath, mapFiles, filter.c_str(), 2 ) ;
      if ( rc )
      {
         ss << "Failed to enum files from[" << filePath << "], "
            << "rc = " << rc << ERROR_END ;
         goto error ;
      }
      for ( it = mapFiles.begin(); it != mapFiles.end(); it++ )
      {
         vec.clear() ;
         boost::split( vec, it->second, 
            boost::is_any_of( DIRECTORY_DELIMITER ) ) ;
         _mapFiles.insert( pair<string, string>( 
            vec[vec.size() - 2], it->second ) ) ;
      }
   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   INT32 _sptClassMetaInfo::_extractTroffInfo()
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      INT64 fileSize = 4096 ;
      CHAR *pFileBuff = NULL ;
      multimap<string, string>::iterator it ;
      MAP_FUNC_META_INFO_IT it2 ;
      
      // alloc buffer, on sucess, free it in "done"
      pFileBuff = (CHAR *)SDB_OSS_MALLOC( fileSize ) ;
      if ( !pFileBuff )
      {
         rc = SDB_OOM ;
         ss << "Failed to alloc memory, rc = " << rc << ERROR_END ;
         goto error ;
      }

      for( it = _mapFiles.begin(); it != _mapFiles.end(); it++ )
      {
         sptFuncMetaInfo metaInfo ;
         string className = it->first ;
         string filePath = it->second ;
         string funcName ;
         string fullName ;
         vector<string> syntax ;
         string desc ;

         ossMemset( pFileBuff, 0, fileSize ) ;
         rc = _readFile( filePath.c_str(), &pFileBuff, &fileSize ) ;
         if ( rc )
         {
            ss << "Failed to read troff's contents from file [" 
               << filePath.c_str() << ", rc = " << rc << ERROR_END ;
            goto error ;
         }
         // extract info
         rc = _getFuncName( filePath, funcName ) ;
         if ( rc )
         {
            ss << "Failed to get the name of function, rc = "
               << rc << ERROR_END ;
            goto error ;
         }
         rc = _getFuncSynopsis( pFileBuff, fileSize, syntax ) ;
         if ( rc )
         {
            ss << "Failed to get the syntax of function in file[" << filePath
               << "], rc = " << rc << ERROR_END ;
            goto error ;
         }
         rc = _getFuncDesc( pFileBuff, fileSize, desc ) ;
         if ( rc )
         {
            ss << "Failed to get the description of function in file[" 
               << filePath << "], rc = " << rc << ERROR_END ;
            goto error ;
         }
         // keep the meta info to map
         metaInfo.funcName = funcName ;
         metaInfo.syntax = syntax ;
         metaInfo.desc = desc ;
         metaInfo.funcType = SPT_FUNC_INSTANCE ;
         metaInfo.path = filePath ;
         it2 = _map_func_meta_info.find( className ) ;
         if ( it2 != _map_func_meta_info.end() )
         {
            it2->second.push_back( metaInfo ) ;
         }
         else
         {
            vector<sptFuncMetaInfo> vec ;
            vec.push_back( metaInfo ) ;
            _map_func_meta_info.insert( PAIR_FUNC_META_INFO( className, vec ) ) ;
         }
         // keep the meta info to vector for fuzzy searching
         fullName = className + SPT_CLASS_SEPARATOR + funcName ;
         _functions.push_back( fullName ) ;
      }

   done:
      if ( pFileBuff )
      {
         SAFE_OSS_FREE( pFileBuff ) ;
      }
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   void _sptClassMetaInfo::_mergeMetaInfo()
   {
      string fullName ;
      MAP_FUNC_META_INFO_IT map_meta_it ;
      MAP_FUNC_DEF_INFO_IT map_def_it = _map_func_def_info.begin() ;
 
      for ( ; map_def_it != _map_func_def_info.end(); map_def_it++ )
      {
         string className = map_def_it->first ;
         map_meta_it = _map_func_meta_info.find( className ) ;
         if ( map_meta_it == _map_func_meta_info.end() )
         {
            continue ;
         }
         vector<sptFuncDefInfo> &vec_def = map_def_it->second ;
         vector<sptFuncMetaInfo> &vec_meta = map_meta_it->second ;
         vector<sptFuncDefInfo>::iterator vec_def_it = vec_def.begin() ;
         vector<sptFuncMetaInfo>::iterator vec_meta_it ;
         for ( ; vec_def_it != vec_def.end(); vec_def_it++ )
         {
            string funcName = vec_def_it->funcName ;
            vec_meta_it = vec_meta.begin() ;
            for ( ; vec_meta_it != vec_meta.end(); vec_meta_it++ )
            {
               if ( funcName == vec_meta_it->funcName )
               {
                  vec_meta_it->funcType = vec_def_it->funcType ;
                  break ;
               }
               else
               {
                  continue ;
               }
            }
         }
      }
   }

   INT32 _sptClassMetaInfo::_getFuncName( string &filePath, string &funcName )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      fs::path path( filePath ) ;
      string fileName = path.leaf().string() ;
      std::size_t found = 0 ;

      found = (SPT_LANG_CN == _lang) ? fileName.rfind( SPT_TROFF_SUFFIX_CN ) :
                                       fileName.rfind( SPT_TROFF_SUFFIX_EN ) ;
      if ( found == string::npos )
      {
         rc = SDB_INVALIDARG ;
         ss << "Invalid troff file name[" << fileName << "], rc = "
            << rc << ERROR_END ;
         goto error ;
      }
      funcName = fileName.substr( 0, found ) ;
      
   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   INT32 _sptClassMetaInfo::_getContents( const CHAR *pFileBuff,
                                          const CHAR *pMark1,
                                          const CHAR*pMark2,
                                          CHAR **ppBuff,
                                          INT32 *pBuffSize )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      INT32 buffSize = 0 ;
      CHAR *pBuff = NULL ;
      CHAR *begin = NULL ;
      CHAR *end = NULL ;

      begin = ossStrstr( (CHAR *)pFileBuff, (CHAR *)pMark1 ) ;
      end = ossStrstr( (CHAR *)pFileBuff, (CHAR *)pMark2 ) ;
      if ( NULL == begin || NULL == end )
      {
         rc = SDB_INVALIDARG ;
         ss << "Failed to get [" << pMark1 << "], or [" 
            << pMark2 << "], rc = " << rc << ERROR_END ;
         goto error ;
      }
      // alloc memory, free in done
      buffSize = end - begin ;
      pBuff = (CHAR *)SDB_OSS_MALLOC( buffSize + 1 ) ;
      if ( NULL == pBuff )
      {
         rc = SDB_OOM ;
         ss << "Failed to alloc [" << buffSize << "] bytes, rc = " 
            << rc << ERROR_END ;
         goto error ;
      }
      ossMemset( pBuff, 0, buffSize ) ;
      // get contents
      ossMemcpy( pBuff, begin, buffSize ) ;
      pBuff[buffSize] = '\0' ;
      // output
      *ppBuff = pBuff ;
      *pBuffSize = buffSize + 1 ;
      
   done:
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   INT32 _sptClassMetaInfo::_getFuncSynopsis( const CHAR *pFileBuff, 
                                              INT32 fileSize, 
                                              vector<string> &output )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      INT32 buffSize = 0 ;
      CHAR *pBuff = NULL ;
      vector<string> vec ;
      vector<string>::iterator it ;
      const CHAR *beg_mark = 
         SPT_LANG_EN == _lang ? SPT_TROFF_SYNOPSIS_EN : SPT_TROFF_SYNOPSIS_CN ;
      const CHAR *end_mark = 
         SPT_LANG_EN == _lang ? SPT_TROFF_CATEGORY_EN : SPT_TROFF_CATEGORY_CN ;

      // pBuff 
      rc = _getContents( pFileBuff, beg_mark, end_mark, 
                         &pBuff, &buffSize ) ;
      if ( SDB_OK != rc )
      {
         ss << "Failed to contents for extracting synopsis, rc = " 
            << rc << endl ;
         goto error ;
      }
      // filter marks
      _filterMarks( pBuff, _uselessMarks, 
                    sizeof( _uselessMarks ) / sizeof( const CHAR * ) ) ;
      // split synopsis
      boost::split( vec, pBuff, boost::is_any_of("\n") ) ;
      for ( it = vec.begin(); it != vec.end(); it++ )
      {
         if ( !it->empty() )
         {
            std::size_t found = it->find( "(" ) ;
            if ( std::string::npos == found )
            {
               output.push_back( *it ) ;
            }
            else
            {
               string sub = it->substr( 0, found ) ;
               found = sub.find_last_of( "." ) ;
               if ( std::string::npos == found )
               {
                  output.push_back( *it ) ;
               }
               else
               {
                  output.push_back( it->substr( found + 1 ) ) ;
               }
            }
         }
      }
      
   done:
      if ( pBuff )
      {
         SAFE_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

   INT32 _sptClassMetaInfo::_getFuncDesc( const CHAR *pFileBuff, 
                                          INT32 fileSize, 
                                          string &desc )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      INT32 buffSize = 0 ;
      CHAR *pBuff = NULL ;
      const CHAR *pos = NULL ;
      const CHAR *beg_mark = 
         SPT_LANG_EN == _lang ? SPT_TROFF_NAME_EN : SPT_TROFF_NAME_CN ;
      const CHAR *end_mark = 
         SPT_LANG_EN == _lang ? SPT_TROFF_SYNOPSIS_EN : SPT_TROFF_SYNOPSIS_CN ;

      // pBuff 
      rc = _getContents( pFileBuff, beg_mark, end_mark, 
                         &pBuff, &buffSize ) ;
      if ( SDB_OK != rc )
      {
         ss << "Failed to contents for extracting synopsis, rc = " 
            << rc << endl ;
         goto error ;
      }
      _replaceCharsWithSpace( pBuff, "\n" ) ;
      _filterMarks( pBuff, _uselessMarks2, 
                    sizeof( _uselessMarks2 ) / sizeof( const CHAR * ) ) ;
      // get description
      pos = ossStrstr( pBuff, "-" ) ;
      if ( NULL == pos )
      {
         rc = SDB_INVALIDARG ;
         ss << "Failed to get the description of function, rc = " 
            << rc << ERROR_END ;
         goto error ;
      }
      // we are going to return a desc in the format of "- xxxx"
      pos++ ;
      while( '\r' != *pos && '\n' != *pos )
      {
         if ( ' ' == *pos )
         {
            pos++ ;
         }
         else
         {
            break ;
         }
      }
      if ( '\r' != *pos && '\n' != *pos )
      {
         desc += string( "- " ) + pos ;
         boost::trim( desc ) ;
      }

   done:
      if ( pBuff )
      {
         SAFE_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      PRINT_ERROR ;
      goto done ;
   }

} // namespace
