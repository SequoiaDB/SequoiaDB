/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sptUsrFileContent.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/03/2017  wujiaming  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRFILECONTENT_HPP_
#define SPT_USRFILECONTENT_HPP_

#include "sptApi.hpp"

namespace engine
{
   class _sptUsrFileContent : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrFileContent )
   public:
      _sptUsrFileContent() ;

      ~_sptUsrFileContent() ;

      INT32 init( const CHAR* buf, UINT32 len ) ;

      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 getLength( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 toBase64Code( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      INT32 clear( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 memberHelp( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      static INT32 staticHelp( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      inline SINT64 getLength()
      {
         return _length ;
      }

      inline const CHAR* getBuf()
      {
         return _buf ;
      }

      inline void clear()
      {
         if( _buf )
         {
            SDB_OSS_FREE( _buf ) ;
            _buf = NULL ;
            _length = 0 ;
         }
      }

   private:
      SINT64 _length ;
      CHAR *_buf ;
   } ;

   typedef _sptUsrFileContent sptUsrFileContent ;
}
#endif
