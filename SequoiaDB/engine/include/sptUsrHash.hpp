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

   Source File Name = sptUsrSystem.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRHASH_HPP_
#define SPT_USRHASH_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptApi.hpp"

#define SPT_USR_HASH_DIGEST          "Digest"

namespace engine
{
   /*
      _sptUsrHash define
   */
   class _sptUsrHash : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrHash )

   public:
      _sptUsrHash() ;
      virtual ~_sptUsrHash() ;

   public:
      static INT32 md5( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      static INT32 fileMD5( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   } ;
   typedef class _sptUsrHash sptUsrHash ;
}

#endif // SPT_USRHASH_HPP_

