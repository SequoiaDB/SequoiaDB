/*******************************************************************************

   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = expCLFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#include "expOutput.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include <sstream>
#include <iostream>

namespace exprt
{
   CHAR *expConvertor::_getBuf( UINT32 reqSize ) 
   {
      SDB_ASSERT( reqSize > 0, "" ) ;
      if ( !_buf || reqSize > _bufSize )
      {
         _buf = (CHAR *)SDB_OSS_REALLOC( _buf, reqSize ) ;
         _bufSize = reqSize ;
      }

      return _buf ;
   }

   void expConvertor::_freeBuf() 
   {
      if (_buf)
      {
         SDB_OSS_FREE(_buf) ;
         _buf = NULL ;
         _bufSize = 0 ;
      }
   }
   
   INT32 expJsonConvertor::init()
   {
      INT32 rc = SDB_OK ;
      INT32 fieldsBufSize = 0 ;

      rc = _decodeBson.init( _options.delChar(), 
                             _options.delField(), 
                             _options.includeBinary(), 
                             _options.includeRegex(),
                             _options.kickNull(),
                             _options.strict(),
                             _options.floatFmt().c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to int the _decodeBson, rc = %d", rc ) ;
         goto error ;
      }

      if ( _cl.fields.empty() )
      {
         goto done ;
      }

      fieldsBufSize = (INT32)_cl.fields.size() ;
      SDB_ASSERT( NULL == _fieldsBuf, "" ) ;
      _fieldsBuf = (CHAR*)SDB_OSS_MALLOC( fieldsBufSize + 1 ) ;
      if ( !_fieldsBuf )
      {
         PD_LOG ( PDERROR, "Failed to alloc buf sized %d ", fieldsBufSize ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossStrcpy( _fieldsBuf, _cl.fields.c_str() ) ;

      rc = _decodeBson.parseFields( _fieldsBuf, fieldsBufSize ) ;
      if ( SDB_OK !=  rc )
      {
         PD_LOG ( PDERROR, "Failed to parse fields of %s.%s, rc = %d", 
                  _cl.csName.c_str(), _cl.clName.c_str(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      _freeFieldsBuf() ;
      goto done ;
   }

   void expJsonConvertor::_freeFieldsBuf() 
   {
      if (_fieldsBuf)
      {
         SDB_OSS_FREE(_fieldsBuf) ;
         _fieldsBuf = NULL ;
      }
   }

   INT32 expJsonConvertor::convert( bson &fromRecord, 
                                    const CHAR *&toBuf, 
                                    UINT32 &toSize )  
   {
      INT32 rc = SDB_OK ;
      INT32 jsonSize = 0 ;
      CHAR  *jsonBuf = NULL ;

      rc = _decodeBson.parseJSONSize( fromRecord.data, &jsonSize ) ;
      if ( SDB_OK !=  rc )
      {
         PD_LOG ( PDERROR, "Failed to get size of json , rc = %d", rc ) ;
         goto error ;
      }

      jsonBuf = _getBuf( (UINT32)jsonSize ) ;
      if ( !jsonBuf )
      {
         PD_LOG ( PDERROR, "Failed to alloc buf sized %d ", jsonSize ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemset( jsonBuf, 0, jsonSize ) ;
      rc = _decodeBson.bsonCovertJson( fromRecord.data, &jsonBuf, &jsonSize ) ;
      if (rc == SDB_OOM)
      {
         _freeBuf();
         jsonSize *= 3 ; 
         jsonBuf = _getBuf( (UINT32)jsonSize );
         if ( !jsonBuf )
         {
             PD_LOG ( PDERROR, "Failed to alloc buf sized %d ", jsonSize ) ;
             rc = SDB_OOM ;
             goto error ;
         }
         ossMemset( jsonBuf, 0, jsonSize ) ;
         rc = _decodeBson.bsonCovertJson( fromRecord.data, &jsonBuf, &jsonSize ) ;
      }
      if ( SDB_OK !=  rc )
      {
         PD_LOG ( PDERROR, "Failed to convert bson to json, rc=%d", rc ) ;
         goto error ;
      }
      jsonSize = ossStrlen( jsonBuf ) ;

      toBuf = jsonBuf ;
      toSize = (UINT32)jsonSize ;
      
   done :
      return rc ;
   error :
      goto done ;
   }

   static void appendCSVField( fieldResolve *pFieldRe, string &fieldList )
   {
      fieldList += pFieldRe->pField ;
      while ( pFieldRe->pSubField )
      {
         pFieldRe = pFieldRe->pSubField ;
         fieldList += '.' ;
         fieldList += pFieldRe->pField ;
      }
   }

   INT32 expCSVConvertor::head( const CHAR *&toBuf, UINT32 &toSize ) 
   {
      INT32 rc = SDB_OK ;

      if ( _options.headLine() )
      {
         INT32 fieldsNum = 0 ;
         fieldResolve *pFieldRe = NULL ;
         string fieldList ;
         fieldsNum = _decodeBson._vFields.size() ;
         for ( INT32 i = 0; i < fieldsNum; ++i )
         {
            pFieldRe = _decodeBson._vFields[i] ;
            appendCSVField( pFieldRe, fieldList ) ;
            if ( i + 1 < fieldsNum )
            {
               fieldList += _options.delField() ;
            }
         } // end for

         toSize = fieldList.size() ;
         toBuf = _getBuf( toSize + 1 ) ;
         if ( !toBuf )
         {
            PD_LOG ( PDERROR, "Failed to alloc buf sized %u ", toSize ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         ossStrncpy( (CHAR*)toBuf, fieldList.c_str(), toSize + 1 ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 expCSVConvertor::convert( bson &fromRecord, 
                                   const CHAR *&toBuf, 
                                   UINT32 &toSize )
   {
      INT32 rc = SDB_OK ;
      INT32 csvSize = 0 ;
      CHAR  *csvBuf = NULL ;
      INT32 tmpSize = 0 ;
      CHAR *tmpBuf = NULL ;

      rc = _decodeBson.parseCSVSize( fromRecord.data, &csvSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get csv size, rc=%d", rc ) ;
         goto error ;
      }
      csvBuf = _getBuf( (UINT32)csvSize ) ;
      if ( !csvBuf )
      {
         PD_LOG ( PDERROR, "Failed to alloc buf sized %d ", csvSize ) ;
         rc = SDB_OOM ;
      }
      tmpBuf = csvBuf ;
      tmpSize = csvSize ;
      rc = _decodeBson.bsonCovertCSV( fromRecord.data, &tmpBuf, &tmpSize ) ;
      if ( SDB_OK !=  rc )
      {
         PD_LOG ( PDERROR, "Failed to convert bson to csv, rc=%d", rc ) ;
         goto error ;
      }
      csvSize = csvSize - tmpSize ;

      toBuf = csvBuf ;
      toSize= (UINT32)csvSize ;
      
   done :
      return rc ;
   error :
      goto done ;
   }

   expCLFile::expCLFile( const expOptions &options, 
                         const string &fileName ) : 
                         _options(options), 
                         _fileName(fileName),
                         _opened(FALSE),
                         _writedSize(0),
                         _fileSuffix(0)
            
   {
   }

   expCLFile::~expCLFile()
   {
      expCLFile::close() ;
   }

   INT32 expCLFile::open()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( !_opened, "cant open again" ) ;

      if ( FALSE == _options.replace() &&
           SDB_OK == ossAccess( _fileName.c_str() ) )
      {
         rc = SDB_FE ;
         PD_LOG ( PDERROR, "file %s already existed", _fileName.c_str() ) ;
         cerr << "file " << _fileName <<" already existed" << endl ;
         goto error ;
      }

      rc = ossOpen ( _fileName.c_str(),
                     OSS_REPLACE | OSS_WRITEONLY | OSS_EXCLUSIVE, 
                     OSS_RU | OSS_WU | OSS_RG,
                     _file ) ;
      if ( SDB_OK != rc )
      {
          PD_LOG ( PDERROR, "Failed to open file %s, rc = %d",
                   _fileName.c_str(), rc ) ;
          goto error ;
      }
      _opened = TRUE ;
      
   done :
      return rc ;
   error :
      goto done ;
   }

   void expCLFile::close()
   {
      if (_opened)
      {
         ossClose(_file) ;
         _opened = FALSE ;
      }
   }

   INT32 expCLFile::_nextFile() 
   {
      INT32 rc = SDB_OK ;
      ostringstream ss ;
      string fileName = _fileName ;

      ++_fileSuffix ;
      ss << '.' << _fileSuffix ;
      fileName += ss.str() ;

      expCLFile::close() ;
      rc = ossOpen ( fileName.c_str(),
                     OSS_REPLACE | OSS_WRITEONLY | OSS_EXCLUSIVE, 
                     OSS_RU | OSS_WU | OSS_RG,
                     _file ) ;
      if ( SDB_OK != rc )
      {
          PD_LOG ( PDERROR, "Failed to open file %s, rc = %d",
                   fileName.c_str(), rc ) ;
          goto error ;
      }
      _opened = TRUE ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 expCLFile::output( const CHAR *buf, UINT32 sz )
   {
      INT32 rc = SDB_OK ;
      SINT64 writed = 0 ;
      UINT32 allWrited = 0 ;
      UINT32 szSave = sz ;

      if ( _writedSize + sz > _options.fileLimit() )
      {
         rc = _nextFile() ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to open next file, rc = %d", rc ) ;
            goto error ;
         }
         _writedSize = 0 ;
      }

      while ( sz > 0 )
      {
         rc = ossWrite ( &_file, buf + allWrited, sz, &writed ) ;
         if ( SDB_OK != rc && SDB_INTERRUPT != rc )
         {
            PD_LOG ( PDERROR, "Failed to write to file, rc = %d", rc ) ;
            goto error ;
         }
         sz -= ( (UINT32)writed ) ;
         allWrited += ( (UINT32)writed ) ;
         rc = SDB_OK ;
      }

      _writedSize += szSave ;

   done :
      return rc ;
   error :
      goto done ;
   }
   
}
