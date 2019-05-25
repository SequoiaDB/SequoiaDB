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

   Source File Name = fromjson.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

/** \file fromjson.hpp
    \brief Convert from json to BSONObj
*/
#ifndef FROMJSON_HPP__
#define FROMJSON_HPP__
#include "core.hpp"
#include "ossFeat.hpp"
#if defined (SDB_ENGINE) || defined (SDB_CLIENT)
#include "../bson/util/builder.h"
#include "../bson/util/optime.h"
#include "../bson/bsontypes.h"
#include "../bson/oid.h"
#include "../bson/bsonelement.h"
#include "../bson/bsonobj.h"
#include "../bson/bsonmisc.h"
#include "../bson/bsonobjbuilder.h"
#include "../bson/bsonobjiterator.h"
#include "../bson/bson-inl.h"
#include "../bson/ordering.h"
#include "../bson/stringdata.h"
#include "../bson/bson_db.h"
#else
#include "bson/bson.hpp"
#endif

/** \namespace bson
    \brief Include files for C++ BSON module
*/
namespace bson
{
/** \fn INT32 fromjson ( const string &str, BSONObj &out ) ;
    \brief Convert from json to BSONObj.
    \param [in] str The json string to be converted
    \param [out] out The CPP BSONObj object of first json in "str"
    \param [in] isBatch Ignore the unnecessary things behind the first json or not
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
   SDB_EXPORT INT32 fromjson ( const string &str, BSONObj &out, 
                               BOOLEAN isBatch = TRUE ) ;

/** \fn INT32 fromjson ( const CHAR *pStr, BSONObj &out ) ;
    \brief Convert from json to BSONObj.
    \param [in] pStr The C-style json charactor string to be converted
    \param [out] out The CPP BSONObj object of first json in "str"
    \param [in] isBatch Ignore the unnecessary things behind the first json or not 
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
   SDB_EXPORT INT32 fromjson ( const CHAR *pStr, BSONObj &out,
                               BOOLEAN isBatch = TRUE ) ;

}
#endif
