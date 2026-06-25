/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

class utilBuffBuilderBase : public SDBObject
{
public:
   utilBuffBuilderBase() {}
   virtual ~utilBuffBuilderBase(){}

   virtual CHAR *getBuff( UINT32 size ) = 0 ;
   virtual UINT32 getBuffSize() = 0 ;
} ;

class utilDecodeBson : public SDBObject
{
public:
   utilDecodeBson() ;
   ~utilDecodeBson() ;

   INT32 init( utilBuffBuilderBase *buffBuilder,
               std::string delChar, std::string delField,
               BOOLEAN includeBinary,
               BOOLEAN includeRegex,
               BOOLEAN kickNull,
               BOOLEAN isStrict,
               const CHAR *pFloatFmt,
               BOOLEAN fullPrecision = FALSE ) ;

   INT32 parseFields( CHAR *pFields, INT32 size ) ;

   INT32 bsonCovertCSV( CHAR *pbson,
                        CHAR **ppBuffer, INT32 *pCSVSize ) ;

   INT32 bsonCovertJson( CHAR *pbson,
                         CHAR **ppBuffer, INT32 *pJSONSize ) ;

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

   INT32 _parseCSVSize( CHAR *pbson, INT32 *pCSVSize ) ;
   INT32 _parseJSONSize( CHAR *pbson, INT32 *pJSONSize ) ;

private:
   std::string _delChar ;
   std::string _delField ;
   BOOLEAN _includeBinary ;
   BOOLEAN _includeRegex ;
   BOOLEAN _kickNull ;
   BOOLEAN _isStrict ;
   utilBuffBuilderBase *_buffBuilder ;

public:
   std::vector<fieldResolve *> _vFields ;
} ;


#endif