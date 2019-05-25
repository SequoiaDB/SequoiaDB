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

   Source File Name = sptConvertor.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPTCONVERTOR_HPP_
#define SPTCONVERTOR_HPP_

#include "core.hpp"
#include "jsapi.h"
#include "../client/bson/bson.h"
#include <string>
using namespace std ;

typedef enum _SPT_CONVERT_MODE
{
   SPT_CONVERT_NORMAL = 0,
   SPT_CONVERT_MATCHER,
   SPT_CONVERT_AGGREGATE
} SPT_CONVERT_MODE ;

/*
   sptConvertor
*/
class sptConvertor
{
public:
   sptConvertor( JSContext *cx, SPT_CONVERT_MODE mode = SPT_CONVERT_NORMAL )
   : _hasSetErrMsg( FALSE ),
     _inMatcher( FALSE ),
     _mode( mode ),
     _cx( cx )
   {
      if ( SPT_CONVERT_MATCHER == _mode )
      {
         _inMatcher = TRUE ;
      }
   }

   ~sptConvertor()
   {
      _cx = NULL ;
   }

public:
   INT32 toBson( JSObject *obj , bson **bs ) ;

   INT32 toBson( JSObject *obj, bson *bs ) ;

   const CHAR* getErrorMsg() ;

   static INT32 toString( JSContext *cx,
                          const jsval &val,
                          std::string &str ) ;

private:
   INT32 _traverse( JSObject *obj , bson *bs ) ;

   INT32 _appendToBson( const std::string &name,
                        const jsval &val,
                        bson *bs ) ;

   INT32 _getDecimalPrecision( const CHAR *precisionStr, 
                               INT32 *precision, INT32 *scale ) ;
   INT32 _addSpecialObj( JSObject *obj,
                         const CHAR *key,
                         bson *bs ) ;

   INT32 _addJsonTypes( JSObject *obj,
                        const CHAR *key,
                        bson *bs ) ;

   BOOLEAN _getProperty( JSObject *obj,
                         const CHAR *fieldName,
                         JSType type,
                         jsval &val ) ;

   INT32 _toString( const jsval &val, std::string &str ) ;

   INT32 _toInt( const jsval &val, INT32 &iN ) ;

   INT32 _toDouble( const jsval &val, FLOAT64 &fV ) ;

   INT32 _toBoolean( const jsval &val, BOOLEAN &bL ) ;

   INT32 _addObjectId( JSObject *obj,
                       const CHAR *key,
                       bson *bs ) ;

   INT32 _addBinData( JSObject *obj,
                      const CHAR *key,
                      bson *bs ) ;

   INT32 _addTimestamp( JSObject *obj,
                        const CHAR *key,
                        bson *bs ) ;

   INT32 _addRegex( JSObject *obj,
                    const CHAR *key,
                    bson *bs ) ;

   INT32 _addMinKey( JSObject *obj,
                     const CHAR *key,
                     bson *bs ) ;

   INT32 _addMaxKey( JSObject *obj,
                     const CHAR *key,
                     bson *bs ) ;

   INT32 _getNumberLongValue( JSObject *obj,
                              INT64 &value ) ;

   INT32 _addNumberLong( JSObject *obj,
                         const CHAR *key,
                         bson *bs) ;

   INT32 _addSdbDate( JSObject *obj,
                      const CHAR *key,
                      bson *bs) ;
   void _setErrorMsg( const CHAR *pErrMsg, BOOLEAN isReplace ) ;
private:
   BOOLEAN _isValidNumberLong( const CHAR *value ) ;

private:
   BOOLEAN _hasSetErrMsg ;
   BOOLEAN _inMatcher ;
   SPT_CONVERT_MODE _mode ;
   JSContext *_cx ;
   string _errorMsg ;
} ;


#endif

