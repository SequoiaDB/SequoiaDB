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

   Source File Name = utilDecodeRawbson.hpp

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
#ifndef UTIL_DECODE_BSON_HPP__
#define UTIL_DECODE_BSON_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "pd.hpp"
#include <vector>

struct fieldResolve : public SDBObject
{
   CHAR *pField ;
   fieldResolve *pSubField ;
   fieldResolve() : pField(NULL),
                    pSubField(NULL)
   {
   }
} ;

class utilDecodeBson : public SDBObject
{
private:
   std::string _delChar ;
   std::string _delField ;
   BOOLEAN _includeBinary ;
   BOOLEAN _includeRegex ;
   BOOLEAN _kickNull ;
   BOOLEAN _isStrict ;
   
public:
   std::vector<fieldResolve *> _vFields ;

private:
   CHAR *_trimLeft( CHAR *pCursor, INT32 &size ) ;
   CHAR *_trimRight( CHAR *pCursor, INT32 &size ) ;
   CHAR *_trim( CHAR *pCursor, INT32 &size ) ;
   void  _freeFieldList( fieldResolve *pFieldRe ) ;
   INT32 _filterString( CHAR **pField, INT32 &size ) ;
   INT32 _parseSubField( CHAR *pField, fieldResolve *pParent ) ;
   INT32 _appendBsonElement( void *pObj, fieldResolve *pFieldRe,
                             const CHAR *pData ) ;
   INT32 _checkFormat( const CHAR *pFloatFmt ) ;

public:
   utilDecodeBson() ;
   ~utilDecodeBson() ;
   INT32 init( std::string delChar, std::string delField,
               BOOLEAN includeBinary,
               BOOLEAN includeRegex,
               BOOLEAN kickNull,
               BOOLEAN isStrict,
               const CHAR *pFloatFmt ) ;
   INT32 parseFields( CHAR *pFields, INT32 size ) ;
   INT32 parseCSVSize( CHAR *pbson, INT32 *pCSVSize ) ;
   INT32 parseJSONSize( CHAR *pbson, INT32 *pJSONSize ) ;
   INT32 bsonCovertCSV( CHAR *pbson, CHAR **ppBuffer, INT32 *pCSVSize ) ;
   INT32 bsonCovertJson( CHAR *pbson, CHAR **ppBuffer, INT32 *pJSONSize ) ;
} ;

#endif