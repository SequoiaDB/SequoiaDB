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
         if( !json2bson( _cl.select.c_str(), NULL, CJSON_RIGOROUS_PARSE,
                         FALSE, TRUE, 0, &select ) )
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
         if( !json2bson( _cl.filter.c_str(), NULL, CJSON_RIGOROUS_PARSE,
                         FALSE, TRUE, 0, &condition ) )
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
         if( !json2bson( _cl.sort.c_str(), NULL, CJSON_RIGOROUS_PARSE,
                         FALSE, TRUE, 0, &sort ) )
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
            // TODO: may continue to export when option 'errorstop' is false,
            //       not handle yet
            goto error ;
         }

         rc = _out.output( buf, size ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to output the record, rc = %d", rc ) ;
            ++_failCount ;
            // TODO: may continue to export when option 'errorstop' is false,
            //       not handle yet
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

      // 1. query and get cursor
      rc = _query( hCL, hCusor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to query collection %s.%s, rc = %d", 
                  _cl.csName.c_str(), _cl.clName.c_str(), rc );
         goto error ;
      }

      // 2. open to output
      rc = _out.open() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open output, rc = %d", rc ) ;
         goto error ;
      }

      // 3. init the covertor
      rc = _convertor.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init convertor, rc = %d", rc ) ;
         goto error ;
      }

      // 4. output the head-line
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

      // 5. iterate to export each record
      rc = _exportRecords( hCusor, exportedCount, failCount ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to export records, rc = %d", rc ) ;
         goto error ;
      }

      // 6. output the tail, do nothing normally
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
            // TODO: may continue to export when option 'errorstop' is false,
            //       not handle yet
            goto error ;
         }
         ++_exportedCLCount ;
      }
      
   done :
      //if ( _exportedCLCount > 0 || _exportedRecordCount > 0 )
      //{
      //   rc = SDB_OK ;
      //}
      return rc ;
   error :
      goto done ;
   }

   INT32 expRoutine::_connectDB( sdbConnectionHandle &hConn )
   {
      INT32 rc = SDB_OK ;
      const expOptions &options = _options ;
      const vector<Host> &hosts = _options.hosts() ;
      vector<Host>::const_iterator it ;

      for ( it = hosts.begin(); it != hosts.end(); ++it )
      {
         const Host& host = *it ;

      #ifdef SDB_SSL
         if ( options.useSSL() )
         {
            rc = sdbSecureConnect( host.hostname.c_str(), host.svcname.c_str(),
                                   options.user().c_str(),
                                   options.password().c_str(),
                                   &hConn ) ;
         }
         else
      #endif
         {
            rc = sdbConnect( host.hostname.c_str(), host.svcname.c_str(),
                             options.user().c_str(), options.password().c_str(),
                             &hConn ) ;
         }

         if( rc )
         {
            continue ;
         }

         break ;
      }

      if ( rc )
      {
         for ( it = hosts.begin(); it != hosts.end(); ++it )
         {
            const Host& host = *it;

            PD_LOG ( PDERROR, "Failed to connect database %s:%s, rc = %d",
                     host.hostname.c_str(), host.svcname.c_str(), rc ) ;
         }

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

      // 1. connect to db
      rc = _connectDB(hConn) ;
      if ( SDB_OK != rc )
      {
         cerr << "Failed to connect to " << _options.hostName() << ":"
              << _options.svcName() << ", rc = " << rc << endl;
         goto error ;
      }
      PD_LOG ( PDINFO, "Connect to %s:%s", 
               _options.hostName().c_str(), _options.svcName().c_str() );

      // 2. parse the options to get collection-list to will be exported
      rc = _clSet.parse(hConn) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to parse collection set, rc = %d", rc ) ;
         goto error ;
      }

      // 3.1. only writes options to conf-file
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

      // 3.2. do the exporting work
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

   void expRoutine::printStatistics( INT32 rc )
   {
      // TODO: may print the failed count of collections and records
      //       when option 'errorstop' is supported
      if ( !_options.hasGenConf() )
      {
         if( SDB_OK == rc )
         {
            PD_LOG( PDINFO, "Exported successfully!" ) ;
            cout << "Exported successfully!" << endl ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to export!" ) ;
            cerr << "Failed to export!" << endl ;

            if( _exportedRecordCount == 0 )
            {
               return ;
            }
         }

         PD_LOG( PDINFO, "Exported records: %u "
                         "\nCompletely exported collections: %llu ",
                 _exportedRecordCount, _exportedCLCount ) ;
         cout << "Exported records: " << _exportedRecordCount << endl ;
         cout << "Completely exported collections: " << _exportedCLCount << endl ;
      }
   }
}
