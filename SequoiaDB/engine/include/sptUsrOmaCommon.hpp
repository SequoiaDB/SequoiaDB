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

   Source File Name = sptUsrOmaCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/18/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USROMA_COMMON_HPP_
#define SPT_USROMA_COMMON_HPP_
#include "ossTypes.hpp"
#include "../bson/bson.hpp"
namespace engine
{
   class _sptUsrOmaCommon: public SDBObject
   {
   public:
      /*
         static functions
      */
      static INT32 getOmaInstallInfo( bson::BSONObj& retObj, string &err ) ;

      static INT32 getOmaInstallFile( string &retStr, string &err ) ;

      static INT32 getOmaConfigFile( string &retStr, string &err ) ;

      static INT32 getOmaConfigs( const bson::BSONObj &arg,
                                  bson::BSONObj &retObj,
                                  string &err ) ;

      static INT32 getIniConfigs( const bson::BSONObj &arg,
                                  bson::BSONObj &retObj,
                                  string &err ) ;

      static INT32 setOmaConfigs( const bson::BSONObj &arg,
                                  const bson::BSONObj &confObj,
                                  string &err ) ;

      static INT32 setIniConfigs( const bson::BSONObj &arg,
                                  const bson::BSONObj &confObj,
                                  string &err ) ;

      static INT32 getAOmaSvcName( const bson::BSONObj &arg,
                                   string &retStr,
                                   string &err ) ;

      static INT32 addAOmaSvcName( const bson::BSONObj &valueObj,
                                   const bson::BSONObj &optionObj,
                                   const bson::BSONObj &matchObj,
                                   string &err ) ;

      static INT32 delAOmaSvcName( const bson::BSONObj &arg,
                                      string &err ) ;
   private:
      static INT32 _getConfFile( string &confFile ) ;

      /*
      confFile       [in] : Configuration file path
      conf           [out]: Output configuration item
      err            [out]: Description of execution failure
      allowNotExist  [in] : Whether to allow files to not exist
      isSdbConfig    [in] : If it is a sdb configuration file,
                            need to parse the special configuration item.
      enableType     [in] : TRUE:  Automatically identify the type of
                                   configuration item;
                            FALSE: Configuration item types are
                                   converted to strings.
      strDelimiter   [in] : TRUE:  String only supports double quotes;
                            FALSE: String supports double quotes and
                                   single quotes.
      */
      static INT32  _getConfInfo( const string &confFile,
                                  bson::BSONObj &conf,
                                  string &err,
                                  BOOLEAN allowNotExist = FALSE,
                                  BOOLEAN isSdbConfig = TRUE,
                                  BOOLEAN enableType = FALSE,
                                  BOOLEAN strDelimiter = TRUE ) ;

      /*
      isSdbConfig    [in] : TRUE:  Force type enableType and
                                   not use character separators;
                            FALSE: enableType and strDelimiter are valid and
                                   use string separators.
      enableType     [in] : TRUE:  Output according to the type of value;
                            FALSE: Forced value output string.
      strDelimiter   [in] : TRUE:  Output string with double quotes;
                            FALSE: Output string with single quotes.
      */
      static INT32  _confObj2Str( const bson::BSONObj &conf,
                                  string &str,
                                  string &err,
                                  const CHAR* pExcept = NULL,
                                  BOOLEAN isSdbConfig = TRUE,
                                  BOOLEAN enableType = FALSE,
                                  BOOLEAN strDelimiter = TRUE ) ;

      static INT32 _config2Ini( const bson::BSONObj &config,
                                string &out,
                                string &err,
                                BOOLEAN noDelimiter = FALSE,
                                BOOLEAN enableType = FALSE,
                                BOOLEAN strDelimiter = TRUE ) ;
   } ;
}

#endif
