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
   INT32 expJsonConvertor::init()
   {
      INT32 rc = SDB_OK ;
      INT32 fieldsBufSize = 0 ;

      rc = _decodeBson.init( &_bufferBuilder,
                             _options.delChar(), 
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
      // _fieldsBuf is needed because _decodeBson.parseFields requires a
      // not-const buf, while _cl.fields.c_str() is const
      _fieldsBuf = (CHAR*)SDB_OSS_MALLOC( fieldsBufSize + 1 ) ;
      if ( !_fieldsBuf )
      {
         PD_LOG ( PDERROR, "Failed to alloc buf sized %d ", fieldsBufSize ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossStrcpy( _fieldsBuf, _cl.fields.c_str() ) ;

      // _fieldsBuf will be modified inside while parsing
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
      CHAR *jsonBuf  = NULL ;

      rc = _decodeBson.bsonCovertJson( fromRecord.data,
                                       &jsonBuf, &jsonSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to convert bson to json, rc=%d", rc ) ;
         goto error ;
      }

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
         toBuf = _bufferBuilder.getBuff( toSize + 1 ) ;
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

      rc = _decodeBson.bsonCovertCSV( fromRecord.data,
                                      &csvBuf, &csvSize ) ;
      if ( SDB_OK !=  rc )
      {
         PD_LOG ( PDERROR, "Failed to convert bson to csv, rc=%d", rc ) ;
         goto error ;
      }

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

      SDB_ASSERT( !_opened, "Cannot open again" ) ;

      if ( FALSE == _options.replace() &&
           SDB_OK == ossAccess( _fileName.c_str() ) )
      {
         rc = SDB_FE ;
         PD_LOG ( PDERROR, "File %s already existed", _fileName.c_str() ) ;
         cerr << "File " << _fileName <<" already existed" << endl ;
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

   // one export-file should not be too large 
   // so open one more file to write data when current file is too large
   // the adding file keeps the suffix like ".1",".2"
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
