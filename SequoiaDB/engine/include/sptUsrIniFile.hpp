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

   Source File Name = sptUsrFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2019  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRINIFILE_HPP_
#define SPT_USRINIFILE_HPP_

#include "sptApi.hpp"
#include "utilIniParserEx.hpp"

namespace engine
{
   class _sptUsrIniFile : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrIniFile )

   public:
      _sptUsrIniFile() ;
      virtual ~_sptUsrIniFile() ;

   public:
      INT32 construct( const _sptArguments &arg, _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 setValue( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;
      INT32 getValue( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 setSectionComment( const _sptArguments &arg, _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;
      INT32 getSectionComment( const _sptArguments &arg, _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 setComment( const _sptArguments &arg, _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 getComment( const _sptArguments &arg, _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;


      INT32 setLastComment( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 getLastComment( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 enableItem( const _sptArguments &arg, _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 disableItem( const _sptArguments &arg, _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 disableAllItem( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 toString( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 toObj( const _sptArguments &arg, _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 save( const _sptArguments &arg, _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 getFileName( const _sptArguments &arg, _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 getFlags( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 convertComment( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 comment2String( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

   private:
      INT32 _exist( const string &path, BOOLEAN &isExist ) ;
      void _setError( bson::BSONObj &detail, const CHAR *errMsg ) ;

   private:
      UINT32 _flags ;
      string _fileName ;
      utilIniParserEx _parser ;
   } ;
}

#endif
