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

   Source File Name = sptClassMetaInfo.hpp

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

#ifndef SPTFUNCINFO_HPP__
#define SPTFUNCINFO_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "../spt/sptFuncDef.hpp"

#include <vector>
#include <map>
#include <string>

using namespace bson ;
using std::string ;
using std::vector ;
using std::map ;
using std::multimap ;

namespace engine
{

   #define SPT_CLASS_SEPARATOR       "::"
   #define SPT_GLOBAL_CLASS          "Global"
   
   #define SPT_FUNC_CONSTRUCTOR       0x00000001
   #define SPT_FUNC_INSTANCE          0x00000002
   #define SPT_FUNC_STATIC            0x00000004
   #define SPT_FUNC_INS_STA           (SPT_FUNC_INSTANCE | SPT_FUNC_STATIC) 

   enum SPT_LANG
   {
      SPT_LANG_EN = 1,
      SPT_LANG_CN = 2
   } ;

   struct _sptFuncMetaInfo
   {
      string         funcName ;
      vector<string> syntax ;
      string         desc ;
      INT32          funcType ;
      string         path ;
   } ;
   typedef struct _sptFuncMetaInfo sptFuncMetaInfo ; 

   typedef pair< string, vector<sptFuncMetaInfo> > PAIR_FUNC_META_INFO ;
   typedef map< string, vector<sptFuncMetaInfo> > MAP_FUNC_META_INFO ;
   typedef map< string, vector<sptFuncMetaInfo> >::iterator MAP_FUNC_META_INFO_IT ;

   // extract js class's info and func's info from conf file and troff file
   class _sptClassMetaInfo : public SDBObject
   {
   public:
      _sptClassMetaInfo() ;
      _sptClassMetaInfo( const string &lang ) ;
      ~_sptClassMetaInfo() {}

   public:
      INT32                       queryFuncInfo( const string &fuzzyFuncName, 
                                                 const string &matcher,
                                                 BOOLEAN isInstance,
                                                 vector<string> &output ) ;
      INT32                       getTroffFile( const string &fullName,
                                                string &path) ;
      INT32                       getMetaInfo( const string &className,
                                               INT32 type,
                                               vector<sptFuncMetaInfo> &output ) ;
   private:
      INT32                       _init() ;
      INT32                       _loadTroffFile() ;
      INT32                       _extractTroffInfo() ;
      void                        _mergeMetaInfo() ;
      
   private:
      INT32                       _getFuncName( string &filePath, 
                                                string &funcName ) ;
      INT32                       _getContents( const CHAR *pFileBuff,
                                                const CHAR *pMark1,
                                                const CHAR*pMark2,
                                                CHAR **ppBuff,
                                                INT32 *pBuffSize ) ;
      INT32                       _getFuncSynopsis( const CHAR *pFileBuff, 
                                                    INT32 fileSize, 
                                                    vector<string> &output ) ;
      INT32                       _getFuncDesc( const CHAR *pFileBuff, 
                                                INT32 fileSize, 
                                                string &desc ) ;
   private:
      MAP_FUNC_DEF_INFO           _map_func_def_info ;
      multimap< string, string >  _mapFiles ;
      
   private:
      SPT_LANG                    _lang ;
      BOOLEAN                     _initOK ;
      MAP_FUNC_META_INFO          _map_func_meta_info ;
      vector<string>              _functions ;
   } ;
   typedef class _sptClassMetaInfo sptClassMetaInfo ;

} // namespace
#endif // SPTFUNCINFO_HPP__
