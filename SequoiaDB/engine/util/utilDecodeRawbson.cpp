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

   Source File Name = utilDecodeRawbson.cpp

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

#include "utilDecodeRawbson.hpp"
#include "rawbson2csv.h"
#include "../client/jstobs.h"
#include "../client/bson/bson.h"

#define UTIL_DE_STR_TABLE   '\t'
#define UTIL_DE_STR_CR      '\r'
#define UTIL_DE_STR_LF      '\n'
#define UTIL_DE_STR_COMMA   ','
#define UTIL_DE_STR_SPACE   32
#define UTIL_DE_STR_QUOTES  '"'
#define UTIL_DE_STR_SLASH   '\\'

CHAR *utilDecodeBson::_trimLeft( CHAR *pCursor, INT32 &size )
{
   INT32 tempSize = size ;
   for ( INT32 i = 0; i < size; ++i )
   {
      switch( *pCursor )
      {
      case UTIL_DE_STR_TABLE:
      case UTIL_DE_STR_SPACE:
         ++pCursor ;
         --tempSize ;
         break ;
      case 0:
      default:
         size = tempSize ;
         return pCursor ;
      }
   }
   size = tempSize ;
   return pCursor ;
}

CHAR *utilDecodeBson::_trimRight ( CHAR *pCursor, INT32 &size )
{
   INT32 tempSize = size ;
   for ( INT32 i = 1; i <= size; ++i )
   {
      switch( *( pCursor + ( size - i ) ) )
      {
      case UTIL_DE_STR_TABLE:
      case UTIL_DE_STR_SPACE:
         --tempSize ;
         break ;
      case 0:
      default:
         size = tempSize ;
         return pCursor ;
      }
   }
   size = tempSize ;
   return pCursor ;
}

CHAR *utilDecodeBson::_trim ( CHAR *pCursor, INT32 &size )
{
   pCursor = _trimLeft( pCursor, size ) ;
   pCursor = _trimRight( pCursor, size ) ;
   return pCursor ;
}

INT32 utilDecodeBson::_filterString( CHAR **pField, INT32 &size )
{
   INT32 rc = SDB_OK ;
   CHAR *pBuffer = *pField ;
   if ( pBuffer[0] == UTIL_DE_STR_QUOTES &&
        pBuffer[size-1] == UTIL_DE_STR_QUOTES )
   {
      ++pBuffer ;
      size -= 2 ;
   }
   *pField = pBuffer ;
   return rc ;
}

void _utilPrintLog( const CHAR *pFunc,
                    const CHAR *pFile,
                    UINT32 line,
                    const CHAR *pFmt,
                    ... )
{
   va_list ap;
   CHAR userInfo[ PD_LOG_STRINGMAX + 1 ] = { 0 } ;
   va_start(ap, pFmt);
   vsnprintf(userInfo, PD_LOG_STRINGMAX, pFmt, ap);
   va_end(ap);
   pdLog( PDERROR, pFunc, pFile, line, userInfo ) ;
}

INT32 utilDecodeBson::_checkFormat( const CHAR *pFloatFmt )
{
   INT32 rc = SDB_OK ;

   if( pFloatFmt == NULL )
   {
      goto done ;
   }

   if( pFloatFmt[0] != '%' )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "floatfmt is invalid, format is %[+][.Precision]Type" ) ;
      goto error ;
   }

   ++pFloatFmt ;

   if( pFloatFmt[0] == '+' )
   {
      ++pFloatFmt ;
   }

   if( pFloatFmt[0] == '.' )
   {
      ++pFloatFmt ;
      for( ; pFloatFmt[0] >= '0' && pFloatFmt[0] <= '9'; ++pFloatFmt )
      {
      }
   }

   if( pFloatFmt[0] != 'f' && pFloatFmt[0] != 'e' && pFloatFmt[0] != 'E' &&
       pFloatFmt[0] != 'g' && pFloatFmt[0] != 'G' )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "floatfmt is invalid, type is f|e|E|g|G" ) ;
      goto error ;
   }

   ++pFloatFmt ;

   if( pFloatFmt[0] != 0 )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "floatfmt is invalid, format is %[+][.Precision]Type" ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::init( std::string delChar, std::string delField,
                            BOOLEAN includeBinary,
                            BOOLEAN includeRegex,
                            BOOLEAN kickNull,
                            BOOLEAN isStrict,
                            const CHAR *pFloatFmt )
{
   INT32 rc = SDB_OK ;

   if ( _delChar.size() > 0 && std::string::npos != delField.find( delChar ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "delfield can not contain delchar" ) ;
      goto error ;
   }

   if( pFloatFmt != NULL )
   {
      if( ossStrncmp( pFloatFmt, "db2", 3 ) == 0 )
      {
         setCsvPrecision( "%+.14E" ) ;
         setJsonPrecision( "%+.14E" ) ;
      }
      else
      {
         rc = _checkFormat( pFloatFmt ) ;
         if( rc )
         {
            PD_LOG ( PDERROR, "failed to parse format" ) ;
            goto error ;
         }
         setCsvPrecision( pFloatFmt ) ;
         setJsonPrecision( pFloatFmt ) ;
      }
   }

   _delChar = delChar ;
   _delField = delField ;
   _includeBinary = includeBinary ;
   _includeRegex = includeRegex ;
   _kickNull = kickNull ;
   _isStrict = isStrict ;
   setPrintfLog( _utilPrintLog ) ;
done:
   return rc ;
error:
   goto done ;
}

utilDecodeBson::utilDecodeBson() : _delChar(),
                                   _delField(),
                                   _includeBinary(FALSE),
                                   _includeRegex(FALSE),
                                   _kickNull(FALSE),
                                   _isStrict(FALSE)
{
}

utilDecodeBson::~utilDecodeBson()
{
   _freeFieldList( NULL ) ;
}

void utilDecodeBson::_freeFieldList( fieldResolve *pFieldRe )
{
   if ( NULL == pFieldRe )
   {
      fieldResolve *pTemp = NULL ;
      INT32 fieldsNum = _vFields.size() ;
      for ( INT32 i = 0; i < fieldsNum; ++i )
      {
         pTemp = _vFields.at( i ) ;
         _freeFieldList( pTemp ) ;
      }
   }
   else
   {
      if ( pFieldRe->pSubField )
      {
         _freeFieldList( pFieldRe->pSubField ) ;
         SAFE_OSS_DELETE( pFieldRe ) ;
      }
      else
      {
         SAFE_OSS_DELETE( pFieldRe ) ;
      }
   }
}

INT32 utilDecodeBson::_parseSubField( CHAR *pField, fieldResolve *pParent )
{
   INT32 rc = SDB_OK ;
   CHAR *pSubField = NULL ;
   fieldResolve *pFieldRe = NULL ;
   pFieldRe = SDB_OSS_NEW fieldResolve() ;
   if ( !pFieldRe )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to malloc memory", rc ) ;
      goto error ;
   }

   if ( pParent )
   {
      pParent->pSubField = pFieldRe ;
   }
   else
   {
      _vFields.push_back( pFieldRe ) ;
   }

   pSubField = ossStrchr( pField, '.' ) ;
   if ( pSubField )
   {
      rc = SDB_INVALIDARG ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Field does not has the\".\" symbol", rc ) ;
         goto error ;
      }
      *pSubField = 0 ;
      ++pSubField ;
      pFieldRe->pField = pField ;
      rc = _parseSubField( pSubField, pFieldRe ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to call _parseSubField", rc ) ;
         goto error ;
      }
   }
   else
   {
      pFieldRe->pField = pField ;
      pFieldRe->pSubField = NULL ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::parseFields( CHAR *pFields, INT32 size )
{
   INT32   rc         = SDB_OK ;
   INT32   tempRc     = SDB_OK ;
   INT32   fieldSize  = 0 ;
   BOOLEAN isString   = FALSE;
   CHAR   *pCursor    = pFields ;
   CHAR   *leftField  = pFields ;

   do
   {
      if ( 0 == size )
      {
         if ( !isString )
         {
            fieldSize = pCursor - leftField ;
            leftField = _trim( leftField, fieldSize ) ;
            if ( fieldSize == 0 )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else
            {
               rc = _filterString( &leftField, fieldSize ) ;
               if ( rc )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               leftField[ fieldSize ] = 0 ;
               rc = _parseSubField( leftField, NULL ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to call _parseSubField", rc ) ;
                  goto error ;
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "field format error, only one side of \
the field appears \", rc = %d", rc ) ;
            goto error ;
         }
         break ;
      }

      if ( UTIL_DE_STR_QUOTES == *pCursor )
      {
         --size ;
         ++pCursor ;
         isString = !isString ;
      }
      else if ( !isString &&
                ( UTIL_DE_STR_COMMA == *pCursor || UTIL_DE_STR_LF == *pCursor ) )
      {
         fieldSize = pCursor - leftField ;
         leftField = _trim( leftField, fieldSize ) ;
         if ( UTIL_DE_STR_LF == *pCursor )
         {
            tempRc = SDB_UTIL_CSV_FIELD_END ;
         }
         if ( fieldSize == 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            rc = _filterString( &leftField, fieldSize ) ;
            if ( rc )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            leftField[ fieldSize ] = 0 ;
            rc = _parseSubField( leftField, NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to call _parseSubField", rc ) ;
               goto error ;
            }
         }

         if ( tempRc == SDB_UTIL_CSV_FIELD_END )
         {
            break ;
         }
         else
         {
            --size ;
            ++pCursor ;
            leftField = pCursor ;
         }
      }
      else
      {
         --size ;
         ++pCursor ;
      }
   }while ( TRUE ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::parseCSVSize( CHAR *pbson, INT32 *pCSVSize )
{
   INT32 rc = SDB_OK ;
   rc = getCSVSize( _delChar.c_str(), _delField.c_str(), _delField.size(),
                    pbson, pCSVSize, _includeBinary, _includeRegex, _kickNull ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get csv size, rc = %d", rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::parseJSONSize( CHAR *pbson, INT32 *pJSONSize )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   if ( bson_init_finished_data( &obj, pbson ) )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to init bson, rc = %d", rc ) ;
      goto error ;
   }
   *pJSONSize = bson_sprint_length ( &obj ) ;
   if ( *pJSONSize == 0 )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to get json size, rc = %d", rc ) ;
      goto error ;
   }
   *pJSONSize = (*pJSONSize) * 2 ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::_appendBsonElement( void *pObj,
                                          fieldResolve *pFieldRe,
                                          const CHAR *pData )
{
   INT32 rc = SDB_OK ;
   bson *obj = (bson *)pObj ;
   bson subObj ;
   bson_iterator it ;
   bson_type fieldType ;
   bson_init( &subObj ) ;
   bson_init_finished_data( &subObj, pData ) ;

   fieldType = bson_find( &it, &subObj, pFieldRe->pField ) ;
   if ( BSON_EOO == fieldType || BSON_UNDEFINED == fieldType )
   {
      if ( bson_append_undefined( obj, pFieldRe->pField ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to call bson_append_undefined, rc = %d", rc ) ;
         goto error ;
      }
      goto done ;
   }

   if ( pFieldRe->pSubField )
   {
      if ( BSON_OBJECT == fieldType || BSON_ARRAY == fieldType )
      {
         rc = _appendBsonElement( obj, pFieldRe->pSubField, bson_iterator_value( &it ) ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element, rc = %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( bson_append_undefined( obj, pFieldRe->pField ) )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to call bson_append_undefined, rc = %d", rc ) ;
            goto error ;
         }
         goto done ;
      }
   }
   else
   {
      if ( bson_append_element( obj, pFieldRe->pField, &it ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to call bson_append_element, rc = %d", rc ) ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::bsonCovertCSV( CHAR *pbson,
                                     CHAR **ppBuffer,
                                     INT32 *pCSVSize )
{
   INT32 rc = SDB_OK ;
   INT32 fieldsNum = 0 ;
   fieldResolve *pFieldRc = NULL ;
   bson obj ;
   bson_init( &obj ) ;

   fieldsNum = _vFields.size() ;
   for ( INT32 i = 0; i < fieldsNum; ++i )
   {
      pFieldRc = _vFields.at( i ) ;
      rc = _appendBsonElement( &obj, pFieldRc, pbson ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to append bson element, rc = %d", rc ) ;
         goto error ;
      }
   }
   bson_finish ( &obj ) ;
   rc = bson2csv( _delChar.c_str(), _delField.c_str(), _delField.size(),
                  obj.data, ppBuffer, pCSVSize,
                  _includeBinary, _includeRegex, _kickNull ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to bson convert csv, rc = %d", rc ) ;
      goto error ;
   }
done:
   bson_destroy ( &obj ) ;
   return rc ;
error:
   goto done ;
}

INT32 utilDecodeBson::bsonCovertJson( CHAR *pbson,
                                      CHAR **ppBuffer,
                                      INT32 *pJSONSize )
{
   INT32 rc = SDB_OK ;
   INT32 fieldsNum = 0 ;
   fieldResolve *pFieldRc = NULL ;
   bson obj ;
   bson_init( &obj ) ;

   fieldsNum = _vFields.size() ;
   if ( fieldsNum > 0 )
   {
      for ( INT32 i = 0; i < fieldsNum; ++i )
      {
         pFieldRc = _vFields.at( i ) ;
         rc = _appendBsonElement( &obj, pFieldRc, pbson ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to append bson element, rc = %d", rc ) ;
            goto error ;
         }
      }
   }
   else
   {
      if ( bson_init_data( &obj, pbson ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to call bson_init_finished_data, rc = %d", rc ) ;
         goto error ;
      }
   }
   bson_finish ( &obj ) ;
   if ( !bsonToJson2 ( *ppBuffer, *pJSONSize, &obj, _isStrict ) )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to convert bson to json, rc=%d", rc ) ;
      goto error ;
   }
done:
   bson_destroy ( &obj ) ;
   return rc ;
error:
   goto done ;
}