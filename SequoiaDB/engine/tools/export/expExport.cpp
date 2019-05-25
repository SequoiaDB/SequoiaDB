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

   Source File Name = expExport.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/

#include "expExport.hpp"
#include "ossMem.hpp"
#include "ossIO.hpp"
#include "pd.hpp"
#include "jstobs.h"
#include <iostream>

namespace exprt
{

   #define EXP_LOG_OUTPUT_SIZE  ( 1024 * 1024 * 1024 ) // 1G

   INT32 expCLExporter::_query( sdbCollectionHandle &hCL, 
                                sdbCursorHandle &hCusor )
   {
      INT32 rc = SDB_OK ;
      string clFullName ;
      bson select ;
      bson condition ;
      bson sort ;
      bson_init ( &select ) ;
      bson_init ( &condition ) ;
      bson_init ( &sort ) ;

      clFullName = _cl.fullName() ;

      if ( _cl.select.empty() )
      {
         bson_empty( &select ) ;
      }
      else
      {
         if( !json2bson( _cl.select.c_str(), NULL,
                         CJSON_RIGOROUS_PARSE, FALSE, &select ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid format of select : %s", 
                   _cl.select.c_str() ) ;
            goto error ;
         }
      }
      
      if ( _cl.filter.empty() )
      {
         bson_empty( &condition ) ;
      }
      else
      {
         if( !json2bson( _cl.filter.c_str(), NULL,
                         CJSON_RIGOROUS_PARSE, FALSE, &condition ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid format of filter : %s", 
                   _cl.filter.c_str() ) ;
            goto error ;
         }
      }
      
      if ( _cl.sort.empty() )
      {
         bson_empty( &sort ) ;
      }
      else
      {
         if( !json2bson( _cl.sort.c_str(), NULL,
                         CJSON_RIGOROUS_PARSE, FALSE, &sort ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid format of sort : %s", 
                   _cl.sort.c_str() ) ;
            goto error ;
         }
      }

      rc = sdbGetCollection( _hConn, clFullName.c_str(), &hCL ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         PD_LOG( PDERROR, "Collection %s does not exist", clFullName.c_str() ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection %s, rc = %d", 
                 clFullName.c_str(), rc ) ;
         goto error ;
      }
      rc = sdbQuery ( hCL, &condition, &select, &sort, NULL,
                      _cl.skip, _cl.limit, &hCusor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query the first record of %s, rc = %d",
                 clFullName.c_str(), rc ) ;
         goto error ;
      }
   done :
      bson_destroy( &select ) ;
      bson_destroy( &condition ) ;
      bson_destroy( &sort ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 expCLExporter::_exportRecords( sdbCursorHandle hCusor, 
                                        UINT64 &exportedCount, 
                                        UINT64 &failCount )
   {
      INT32 rc = SDB_OK ;
      bson record ;
      const CHAR *buf = NULL ;
      UINT32 size = 0 ;
      UINT64 accumulatedSize = 0 ;
      UINT64 _exportedCount = 0 ;
      UINT64 _failCount = 0 ;
      bson_init ( &record ) ;

      while ( TRUE )
      {
         rc = sdbNext( hCusor, &record ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get the next record, rc = %d", rc ) ;
            goto error ;
         }

         rc = _convertor.convert( record, buf, size ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to convert the record, rc = %d",  rc ) ;
            ++failCount ;
            goto error ;
         }

         rc = _out.output( buf, size ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to output the record, rc = %d", rc ) ;
            ++_failCount ;
            goto error ;
         }
         ++_exportedCount ;

         accumulatedSize += size ;
         if ( accumulatedSize > EXP_LOG_OUTPUT_SIZE )
         {
            PD_LOG( PDINFO, "Successed to exported %llu records, "
                            "failed to to exported %llu records",
                    _exportedCount, _failCount ) ;
            accumulatedSize = 0 ;
         }

         buf = _options.delRecord().c_str() ;
         size = (UINT32)_options.delRecord().size() ;
         accumulatedSize += size ;
         rc = _out.output( buf, size ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to output record-delimiter after"
                             " the record , rc = %d", rc ) ;
            goto error ;
         }
      }
      
   done :
      exportedCount += _exportedCount ;
      failCount += _failCount ;
      bson_destroy( &record ) ;
      return rc ;
   error :
      goto done ;
   }
   
   INT32 expCLExporter::run( UINT64 &exportedCount, UINT64 &failCount )
   {
      INT32 rc = SDB_OK ;
      sdbCursorHandle hCusor = SDB_INVALID_HANDLE ;
      sdbCollectionHandle hCL = SDB_INVALID_HANDLE ;
      const CHAR *buf = NULL ;
      UINT32 size = 0 ;

      rc = _query( hCL, hCusor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to query collection %s.%s, rc = %d", 
                  _cl.csName.c_str(), _cl.clName.c_str(), rc );
         goto error ;
      }

      rc = _out.open() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open output, rc = %d", rc ) ;
         goto error ;
      }

      rc = _convertor.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init convertor, rc = %d", rc ) ;
         goto error ;
      }

      rc = _convertor.head( buf, size ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get head of convertor, rc = %d", rc ) ;
         goto error ;
      }
      if ( size > 0 )
      {
         rc = _out.output( buf, size );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to output the head, rc = %d", rc ) ;
            goto error ;
         }
         buf = _options.delRecord().c_str() ;
         size = (UINT32)_options.delRecord().size() ;
         rc = _out.output( buf, size );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to output the record delimiter, rc = %d",
                    rc ) ;
            goto error ;
         }
      }

      rc = _exportRecords( hCusor, exportedCount, failCount ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to export records, rc = %d", rc ) ;
         goto error ;
      }

      rc = _convertor.tail( buf, size ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get tail of convertor, rc = %d", rc ) ;
         goto error ;
      }
      if ( size > 0 )
      {
         rc = _out.output( buf, size );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to output the tail, rc = %d", rc ) ;
            goto error ;
         }
      }
      
   done :
      _out.close() ;
      
      if ( SDB_INVALID_HANDLE != hCL )
      {
         sdbReleaseCollection(hCL) ;
      }
      if ( SDB_INVALID_HANDLE != hCusor )
      {
         sdbCloseCursor(hCusor) ;
         sdbReleaseCursor(hCusor) ;
      }
      return rc ;
   error :
      goto done ;
   }

   void expRoutine::_getCLFileName( const expCL &cl, string &fileName )
   {
      if ( !_options.file().empty() )
      {
         fileName = _options.file() ;
      }
      else
      {
         fileName = _options.dir() ;
         fileName += cl.csName ;
         fileName += "." ;
         fileName += cl.clName ;
         fileName += "." ;
         fileName += _options.typeName() ;
      }
   }

   INT32 expRoutine::_exportOne( sdbConnectionHandle hConn, const expCL &cl ) 
   {
      INT32 rc = SDB_OK ;
      UINT64 count = 0 ;
      UINT64 failCount = 0 ;
      string fileName ;
      expCLOutput  *pOutput = NULL ;
      expConvertor *pConvertor = NULL ;
      
      _getCLFileName( cl, fileName ) ;
      pOutput = SDB_OSS_NEW expCLFile( _options, fileName ) ;
      if ( !pOutput )
      {
         PD_LOG( PDERROR, "Failed to alloc the pOutput" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      
      if ( FORMAT_CSV == _options.type() )
      {
         pConvertor = SDB_OSS_NEW expCSVConvertor( _options, cl ) ;
      }
      else if ( FORMAT_JSON == _options.type() )
      {
         pConvertor = SDB_OSS_NEW expJsonConvertor( _options, cl ) ;
      }
      if ( !pConvertor )
      {
         PD_LOG( PDERROR, "Failed to alloc the pConvertor" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      {
         expCLExporter exporter( _options, hConn, cl, *pOutput, *pConvertor ) ;
         rc = exporter.run( count, failCount ) ;
      }

      _exportedRecordCount += count ;
      _failRecordCount += failCount ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, 
                 "Failed to export collection %s.%s :"
                 " %llu successful, %llu failed records", 
                 cl.csName.c_str(), cl.clName.c_str(),
                 count, failCount ) ;
         goto error ;
      }
      
      PD_LOG( PDINFO, 
              "Exported collection %s.%s: %llu successful, %llu failed records", 
              cl.csName.c_str(), cl.clName.c_str(),
              count, failCount ) ;
         
   done :
      if ( pOutput )    { SDB_OSS_DEL pOutput; }
      if ( pConvertor ) { SDB_OSS_DEL pConvertor; }
      return rc ;
   error :
      goto done ;
   }

   INT32 expRoutine::_export( sdbConnectionHandle hConn ) 
   {
      INT32 rc = SDB_OK ;

      for ( expCLSet::const_iterator it = _clSet.begin();
            _clSet.end() != it; ++it )
      {
         rc = _exportOne( hConn, *it ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDINFO, "Failed to export collection %s.%s, rc = %d", 
                    it->csName.c_str(), it->clName.c_str(), rc ) ;
            ++_failCLCount ;
            goto error ;
         }
         ++_exportedCLCount ;
      }
      
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 expRoutine::_connectDB( sdbConnectionHandle &hConn )
   {
      INT32 rc = SDB_OK ;
      const expOptions &options = _options ;
      
   #ifdef SDB_SSL
      if ( options.useSSL() )
      {
         rc = sdbSecureConnect( options.hostName().c_str(), 
                                options.svcName().c_str(),
                                options.user().c_str(),
                                options.password().c_str(),
                                &hConn ) ;
      }
      else
   #endif
      {
         rc = sdbConnect( options.hostName().c_str(), options.svcName().c_str(),
                          options.user().c_str(), options.password().c_str(),
                          &hConn ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to connect database %s:%s, rc = %d",
                  options.hostName().c_str(), options.svcName().c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 expRoutine::run()
   {
      INT32 rc = SDB_OK ;
      sdbConnectionHandle hConn = SDB_INVALID_HANDLE ;

      rc = _connectDB(hConn) ;
      if ( SDB_OK != rc )
      {
         cerr << "failed to connect to " << _options.hostName() << ":"
              << _options.svcName() << endl ;
         goto error ;
      }
      PD_LOG ( PDINFO, "Connect to %s:%s", 
               _options.hostName().c_str(), _options.svcName().c_str() );

      rc = _clSet.parse(hConn) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to parse collection set, rc = %d", rc ) ;
         goto error ;
      }

      if ( _options.hasGenConf() )
      {
         rc = _options.writeToConf( _clSet ) ;
         if ( SDB_OK != rc )
         {
            cerr << "Failed to write configure-file" << endl ;
            PD_LOG ( PDERROR, "Failed to write configure-file, rc = %d", rc ) ;
            goto error ;
         }
         goto done ;
      }

      rc = _export( hConn ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to export collections, rc = %d", rc ) ;
         goto error ;
      }

   done :
      if ( SDB_INVALID_HANDLE != hConn )
      {
         sdbDisconnect (hConn) ;
         sdbReleaseConnection(hConn) ;
      }
      return rc ;
   error :
      goto done ;
   }

   void expRoutine::printStatistics()
   {
      if ( !_options.hasGenConf() )
      {
         if ( 0 == _failCLCount && 0 == _failRecordCount )
         {
            PD_LOG( PDINFO, "Exported successfully with"
                            " %u successful collections, "
                            " %llu successful records",
                    _exportedCLCount, _exportedRecordCount ) ;
            cout << "Exported successfully with " << _exportedCLCount 
                 << " successful collections, " << _exportedRecordCount
                 << " successful records" << endl ;
         }
         else if( _exportedRecordCount > 0 &&
                  ( _failCLCount > 0 || _failRecordCount > 0 ) )
         {
            PD_LOG( PDINFO, "Exported partial-successfully with"
                            " %u successful collections, "
                            " %llu successful records",
                    _exportedCLCount, _exportedRecordCount ) ;
            cout << "Exported partial-successfully with " << _exportedCLCount 
                 << " successful collections, " << _exportedRecordCount
                 << " successful records" << endl ;
         }
         else 
         {
            PD_LOG( PDINFO, "Failed to export!" ) ;
            cerr << "Failed to export!" << endl ;
         }
      }
   }
}
