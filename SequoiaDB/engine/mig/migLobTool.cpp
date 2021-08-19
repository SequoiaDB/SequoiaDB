/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = migLobTool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "migLobTool.hpp"
#include "msgDef.hpp"

using namespace bson ;
using namespace sdbclient ;

namespace lobtool 
{
const UINT32 BUF_SIZE = 2 * 1024 * 1024 ;

   _migLobTool::_migLobTool()
   :_buf( NULL ),
    _bufSize( 0 ),
    _written( 0 ),
    _db( NULL )
   {
      _bufSize = BUF_SIZE ;
      _buf = ( CHAR * )ossAlignedAlloc( OSS_FILE_DIRECT_IO_ALIGNMENT,
                                        BUF_SIZE ) ;
   }

   _migLobTool::~_migLobTool()
   {
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }

      if ( NULL != _buf )
      {
         SDB_OSS_ORIGINAL_FREE( _buf ) ;
      }
   }

   INT32 _migLobTool::exec( const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      migOptions ops ;
      BSONElement ele ;

      if ( NULL == _buf )
      {
         PD_LOG( PDERROR, "failed to allocate memory" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ele = options.getField( MIG_OP ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_OP, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( 0 == ossStrcmp( ele.valuestr(), MIG_OP_IMPRT ) )
      {
         ops.type = MIG_OP_TYPE_IMPRT ;
      }
      else if ( 0 == ossStrcmp( ele.valuestr(), MIG_OP_EXPRT ) )
      {
         ops.type = MIG_OP_TYPE_EXPRT ;
      }
      else if ( 0 == ossStrcmp( ele.valuestr(), MIG_OP_MIGRATION ) )
      {
         ops.type = MIG_OP_TYPE_MIGRATION ;
      }
      else
      {
         PD_LOG( PDERROR, "unknown operation type:%s", ele.valuestr() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ele = options.getField( MIG_HOSTNAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_HOSTNAME, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ops.hostname = ele.valuestr() ;

      ele = options.getField( MIG_SERVICE ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_SERVICE, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ops.service = ele.valuestr() ;

      if ( MIG_OP_TYPE_MIGRATION != ops.type )
      {
         ele = options.getField( MIG_FILE ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid type of %s, options:%s",
                    MIG_FILE, options.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         ops.file = ele.valuestr() ;
      }

      ele = options.getField( MIG_USRNAME ) ;
      if ( ele.eoo() )
      {
        ops.usrname = "" ;
      }
      else if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_USRNAME, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         ops.usrname = ele.valuestr() ;
      }

      ele = options.getField( MIG_PASSWD ) ;
      if ( ele.eoo() )
      {
         ops.passwd = "" ;
      }
      else if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_PASSWD, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         ops.passwd = ele.valuestr() ;
      }

      ele = options.getField( MIG_CL ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_CL, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ops.collection = ele.valuestr() ;

      ele = options.getField( MIG_IGNOREFE ) ;
      if ( ele.eoo() )
      {
         ops.ignorefe = FALSE ;
      }
      else if ( Bool != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_IGNOREFE, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         ops.ignorefe = ele.Bool() ;
      }

      if ( MIG_OP_TYPE_MIGRATION == ops.type )
      {
         ele = options.getField( MIG_DST_HOST ) ;
         if ( String == ele.type() )
         {
            ops.dsthost = ele.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid type of %s, options:%s",
                    MIG_DST_HOST, options.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ele = options.getField( MIG_DST_SERVICE ) ;
         if ( String == ele.type() )
         {
            ops.dstservice = ele.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid type of %s, options:%s",
                    MIG_DST_SERVICE, options.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;         
         }

         ele = options.getField( MIG_DST_USRNAME ) ;
         if ( String == ele.type() )
         {
            ops.dstusrname = ele.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid type of %s, options:%s",
                    MIG_DST_USRNAME, options.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ele = options.getField( MIG_DST_PASSWD ) ;
         if ( String == ele.type() )
         {
            ops.dstpasswd = ele.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid type of %s, options:%s",
                    MIG_DST_PASSWD, options.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ele = options.getField( MIG_DST_CL ) ;
         if ( String == ele.type() )
         {
            ops.dstcl = ele.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid type of %s, options:%s",
                    MIG_DST_CL, options.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      ele = options.getField( FIELD_NAME_PREFERED_INSTANCE ) ;
      if ( String == ele.type() )
      {
         ops.prefer = ele.valuestr() ;
      }
      else if ( ele.isNumber() )
      {
         ops.preferNum = ele.Number() ;
      }

#ifdef SDB_SSL
      ele = options.getField( MIG_SSL ) ;
      if ( ele.eoo() )
      {
         ops.useSSL = FALSE ;
      }
      else if ( Bool != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of %s, options:%s",
                 MIG_SSL, options.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         ops.useSSL = ele.Bool() ;
      }
#endif

      rc = _initDB( ops ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init db:%s, rc:%d",
                 options.toString( FALSE, TRUE ).c_str(), rc ) ;
         goto error ;
      }

      if ( MIG_OP_TYPE_EXPRT == ops.type )
      {
         rc = _exportLob( ops ) ;
      }
      else if ( MIG_OP_TYPE_IMPRT == ops.type )
      {
         rc = _importLob( ops ) ;
      }
      else if ( MIG_OP_TYPE_MIGRATION == ops.type )
      {
         rc = _migrate( ops ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to complete operation, rc:%d", rc ) ;
         goto error ;
      }

   done:
      _closeDB() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_migrate( const migOptions &ops )
   {
      INT32 rc = SDB_OK ;
      bson::BSONObj obj ;
      sdbclient::sdbCursor cursor ;
      sdbclient::sdb dstDB ;
      sdbclient::sdbCollection dstCL ;
      UINT64 totalNum = 0 ;

      PD_LOG( PDEVENT, "begin to migrate lob" ) ;
      rc = dstDB.connect( ops.dsthost, ops.dstservice,
                          ops.dstusrname, ops.dstpasswd ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to connect dst db:%d", rc ) ;
         goto error ;
      }

      rc = dstDB.getCollection( ops.dstcl, dstCL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get dst collection:%s, rc:%d",
                 ops.dstcl, rc ) ;
         goto error ;
      }

      rc = _cl.listLobs( cursor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to list lobs in collection:%s, rc:%d",
                 ops.collection, rc ) ;
         goto error ;
      }

      do
      {
         rc = cursor.next( obj ) ;
         if ( SDB_OK == rc )
         {
            BSONElement oid ;
            BSONElement available ;

            oid = obj.getField( FIELD_NAME_LOB_OID ) ;
            if ( jstOID != oid.type() )
            {
               PD_LOG( PDERROR, "invalid object:%s",
                       obj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            available = obj.getField( FIELD_NAME_LOB_AVAILABLE ) ;
            if ( Bool != available.type() )
            {
               PD_LOG( PDERROR, "invalid object:%s",
                       obj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( !available.Bool() )
            {
               continue ;
            }

            rc = _migrateLob2Dst( oid.OID(), dstCL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to migrate lob:%s, rc:%d",
                       obj.toString( FALSE, TRUE ).c_str(), rc ) ;
               if ( SDB_FE == rc && ops.ignorefe )
               {
                  rc = SDB_OK ;
                  continue ;
               }
               else
               {
                  goto error ;
               }
            }

            ++totalNum ;
         }
         else if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to get next lob:%d", rc ) ;
            goto error ;
         }
      } while ( TRUE ) ;

      PD_LOG( PDEVENT, "lob migration has been done, total num:%lld", totalNum ) ;
      cout << "lob migration has been done, total num:" << totalNum << endl ;
   done:
      dstDB.disconnect() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_migrateLob2Dst( const bson::OID &oid,
                                       sdbclient::sdbCollection &cl )
   {
      INT32 rc = SDB_OK ;
      sdbLob src ;
      sdbLob dst ;

      rc = cl.createLob( dst, &oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create lob:%s, rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      rc = _cl.openLob( src, oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob[%s] in src collection, rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      do
      {
         UINT32 read = 0 ;
         rc = src.read( _bufSize,
                        _buf,
                        &read ) ;
         if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read lob[%s], rc:%d",
                    oid.str().c_str(), rc ) ;
            goto error ;
         }
         _written += read ;

         rc = dst.write( _buf, _written ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob[%s], rc:%d",
                    oid.str().c_str(), rc ) ;
            goto error ;
         }
         _written = 0 ;
      } while ( TRUE ) ;

      rc = dst.close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%s, rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }
   done:
      src.close() ;
      return rc ;
   error:
      {
      BOOLEAN closed = TRUE ;
      dst.isClosed( closed ) ;
      if ( !closed )
      {
         dst.close() ;
      }

      if ( SDB_FE != rc )
      {
         cl.removeLob( oid ) ;
      }
      }
      goto done ;
   }                      

   INT32 _migLobTool::_exportLob( const migOptions &ops )
   {
      INT32 rc = SDB_OK ;
      bson::BSONObj obj ;
      sdbclient::sdbCursor cursor ;
      UINT64 totalNum = 0 ;

      PD_LOG( PDEVENT, "begin to export lob" ) ;
      
      rc = _createFile( ops.file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create file:%s, rc:%d",
                 ops.file, rc ) ;
         goto error ;
      }
      
      rc = _cl.listLobs( cursor ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to list lobs in collection:%s, rc:%d",
                 ops.collection, rc ) ;
         goto error ;
      }

      do
      {
         rc = cursor.next( obj ) ;
         if ( SDB_OK == rc )
         {
            BSONElement available = obj.getField( FIELD_NAME_LOB_AVAILABLE ) ;
            if ( Bool != available.type() )
            {
               PD_LOG( PDERROR, "invalid object:%s",
                       obj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( !available.Bool() )
            {
               continue ;
            }

            rc = _append2File( obj ) ;
            if ( SDB_FNE == rc ||
                 SDB_LOB_IS_NOT_AVAILABLE == rc ||
                 SDB_LOB_SEQUENCE_NOT_EXIST == rc )
            {
               PD_LOG( PDWARNING, "lob[%s] may be removed when export it, rc:%d",
                       obj.toString( FALSE, TRUE ).c_str(), rc ) ;
               rc = SDB_OK ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append lob[%s] to file, rc:%d",
                       obj.toString( FALSE, TRUE ).c_str(), rc ) ;
               goto error ;
            }
            else
            {
               ++totalNum ;
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to fetch next record:%d", rc ) ;
            goto error ;
         }

      } while ( TRUE ) ;

      rc = _refreshHeader( totalNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to refresh header:%d", rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "lob exporting has been done, total num:%lld", totalNum ) ;
      cout << "lob exporting has been done, total num:" << totalNum << endl ;
   done:
      cursor.close() ;
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
      return rc ;
   error:
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
         ossDelete( ops.file ) ;
      }
      goto done ;
   }

   INT32 _migLobTool::_importLob( const migOptions &ops )
   {
      INT32 rc = SDB_OK ;
      const migFileHeader *header = NULL ;
      UINT64 totalNum = 0 ;
      imprtIterator itr ;
      SINT64 totalImport = 0 ;
      BOOLEAN skip = FALSE ;
      PD_LOG( PDEVENT, "begin to import lob" ) ;

      rc = _openFile( ops.file, header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s", ops.file ) ;
         goto error ;
      }

      /// header will be invalid.
      totalNum = header->totalNum ;
      for ( UINT64 i = 0; i < totalNum; ++i )
      {
         skip = FALSE ;
         rc = _sendLobFromFile( ops.ignorefe, itr, skip ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to send lob:%d", rc ) ;
            goto error ;
         }
         else if ( !skip )
         {
            ++totalImport ;
         }
      }

      PD_LOG( PDEVENT, "lob importing has been done, total num:%lld", totalImport ) ;
      cout << "lob importing has been done, total num:" << totalImport << endl ;
   done:
      return rc ;
   error:
      goto done ;
   }

   enum IMPRT_STATUS
   {
      IMPRT_STATUS_META = 1,
      IMPRT_STATUS_BODY,
      IMPRT_STATUS_SEEK,
   } ;

   INT32 _migLobTool::_sendLobFromFile( BOOLEAN ignorefe,
                                        imprtIterator &itr,
                                        BOOLEAN &skip )
   {
      INT32 rc = SDB_OK ;
      sdbLob lob ;
      SINT64 lobLen = 0 ;
      bson::OID oid ;
      IMPRT_STATUS status  = IMPRT_STATUS_META ;
      SINT64 seekSize = 0 ;

      do
      {
         if ( itr.empty() )
         {
            SINT64 read = 0 ;
            rc = ossReadN( &_file, _bufSize, _buf, read ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to read file:%d", rc ) ;
               goto error ;
            }
            itr.loadSize = read ;
            itr.start = 0 ;
            continue ;
         }

         if ( IMPRT_STATUS_META == status )
         {
            bson::OID oid ;
            SINT32 objLen = 0 ;
            SINT64 aligned = 0 ;

            rc = _getLobMeta( itr, oid, lobLen, objLen ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            aligned = ossRoundUpToMultipleX( lobLen + objLen,
                                             OSS_FILE_DIRECT_IO_ALIGNMENT ) ;

            rc = _cl.createLob( lob, &oid ) ;
            if ( SDB_FE == rc && ignorefe )
            {
               status = IMPRT_STATUS_SEEK ;
               seekSize = aligned - objLen ;
               rc = SDB_OK ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to open lob[%s], rc:%d",
                       oid.str().c_str(), rc ) ;
               goto error ;
            }
            else
            {
               status = IMPRT_STATUS_BODY ;
               seekSize = aligned - lobLen - objLen ;
            }

            itr.start += objLen ;
         }
         else if ( IMPRT_STATUS_BODY == status )
         {
            SINT64 needWrite = itr.keepSize() <= lobLen ?
                               itr.keepSize() : lobLen ;
            rc = lob.write( _buf + itr.start,
                            needWrite ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write lob, rc:%d", rc ) ;
               goto error ;
            }
            lobLen -= needWrite ;
            itr.start += needWrite ;
            if ( 0 == lobLen )
            {
               break ;
            }
         }
         else
         {
            UINT32 seek = itr.keepSize() <= seekSize ?
                          itr.keepSize() : seekSize ;
            itr.start += seek ;
            seekSize -= seek ;
            if ( 0 == seekSize )
            {
               break ;
            }
         }
      } while ( TRUE ) ;

      if ( IMPRT_STATUS_SEEK != status )
      {
         rc = lob.close() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
            goto error ;
         }

         SDB_ASSERT( seekSize <= itr.keepSize(), "impossible" ) ;
         itr.start += seekSize ;
         skip = FALSE ;
      }
      else
      {
         skip = TRUE ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_getLobMeta( const imprtIterator &itr,
                                   bson::OID &oid, SINT64 &len,
                                   SINT32 &objLen )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement ele ;
      const UINT32 *bsonLen = ( const UINT32 * )( _buf + itr.start ) ;
      if ( itr.keepSize() < *bsonLen )
      {
         PD_LOG( PDERROR, "bson len:%d, keep in memory:%d."
                 " this tool's version may not match the file's version" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      obj = BSONObj( _buf + itr.start ) ;
      ele = obj.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "invalid bson object in file:%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      oid = ele.OID() ;

      ele = obj.getField( FIELD_NAME_LOB_SIZE ) ;
      if ( NumberInt != ele.type() &&
           NumberLong != ele.type() )
      {
         PD_LOG( PDERROR, "invalid bson object in file:%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      len = ele.Long() ;

      objLen = obj.objsize() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_refreshHeader( UINT64 totalNum )
   {
      INT32 rc = SDB_OK ;
      migFileHeader *header = NULL ;
      ossTimestamp t ;
      ossGetCurrentTime( t ) ;

      _initFileHeader( ( migFileHeader * )_buf ) ;
      header = ( migFileHeader * )_buf ;
      header->totalNum = totalNum ;
      header->crtTime = t.time * 1000 + t.microtm / 1000 ;
      rc = ossSeek( &_file, 0, OSS_SEEK_SET ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek to the beginning of file:%d", rc ) ;
         goto error ;
      }

      rc = ossWriteN( &_file, _buf, sizeof( migFileHeader ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write file, rc:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_append2File( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      sdbLob lob ;
      bson::OID oid ;
      UINT64 totalWrite = 0 ;
      INT64 truncateSize = 0 ;
      BSONElement ele ;

      rc = ossGetFileSize( &_file, &truncateSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get file size:%d", rc ) ;
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "invalid lob obj:%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      oid = ele.OID() ;

      rc = _write( obj.objdata(), obj.objsize() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write file:%d", rc ) ;
         goto error ;
      }

      totalWrite += obj.objsize() ;

      rc = _cl.openLob( lob, oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob[%s], rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      do
      {
         UINT32 read = 0 ;
         rc = lob.read( _bufSize - _written,
                        _buf + _written,
                        &read ) ;
         if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read lob[%s], rc:%d",
                    oid.str().c_str(), rc ) ;
            goto error ;
         }

         _written += read ;
         if ( _bufSize == _written )
         {
            rc = ossWriteN( &_file, _buf, _bufSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write file:%d", rc ) ;
               goto error ;
            }
            _written = 0 ;
         }
      } while ( TRUE ) ;

      if ( 0 < _written )
      {
         UINT32 aligned = ossRoundUpToMultipleX( _written,
                                                 OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
         ossMemset( _buf + _written, 0, aligned - _written ) ;
         SDB_ASSERT( aligned <= _bufSize, "impossible" ) ;
         rc = ossWriteN( &_file, _buf, aligned ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write file:%d", rc ) ;
            goto error ;
         }
         _written = 0 ;
      }

   done:
      lob.close() ;
      return rc ;
   error:
      if ( 0 < truncateSize )
      {
         INT32 rcTmp = _truncate( truncateSize ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to truncate file:%d", rcTmp ) ;
         }
         _written = 0 ;
      }
      goto done ;
   }

   INT32 _migLobTool::_truncate( INT64 len )
   {
      INT32 rc = SDB_OK ;
      rc = ossTruncateFile( &_file, len ) ;
      if ( SDB_OK  != rc )
      {
         PD_LOG( PDERROR, "failed to truncate file:%d", rc ) ;
         goto error ;
      }

      rc = ossSeek( &_file, 0, OSS_SEEK_END ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek to the end of file:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_write( const CHAR *data, UINT32 size )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pos = data ;
      UINT32 totalLen = size ;

      do
      {
         UINT32 cpLen = totalLen <= _bufSize - _written ?
                        totalLen : _bufSize - _written ;
         ossMemcpy( _buf + _written, pos, cpLen ) ;
         _written += cpLen ;

         if ( _bufSize == _written )
         {
            rc = ossWriteN( &_file, _buf, _bufSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write file:%d", rc ) ;
               goto error ;
            }
            _written = 0 ;
         }

         pos += cpLen ;
         totalLen -= cpLen ;
      } while ( 0 < totalLen ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _migLobTool::_initDB( const migOptions &ops )
   {
      INT32 rc = SDB_OK ;

      _db = new(std::nothrow) sdb ( ops.useSSL ) ;
      if ( NULL == _db )
      {
         PD_LOG( PDERROR, "failed to alloc sdb") ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _db->connect( ops.hostname,
                        ops.service,
                        ops.usrname,
                        ops.passwd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to connect to specified db[%s:%s], rc:%d",
                 ops.hostname, ops.service, rc ) ;
         goto error ;
      }

      rc = _db->getCollection( ops.collection, _cl ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get collection[%s], rc:%d",
                  ops.collection, rc ) ;
         goto error ;
      }

      if ( NULL != ops.prefer )
      {
         rc = _db->setSessionAttr(
                  BSON( FIELD_NAME_PREFERED_INSTANCE << ops.prefer ) ) ;
      }
      else if ( 0 != ops.preferNum )
      {
         rc = _db->setSessionAttr(
                  BSON( FIELD_NAME_PREFERED_INSTANCE << ops.preferNum ) ) ;
      }
      else
      {
         goto done ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set session's attribute:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _migLobTool::_closeDB()
   {
      if ( NULL != _db )
      {
         _db->disconnect() ;
         delete _db ;
         _db = NULL ;
      }
      return ;
   }

   INT32 _migLobTool::_openFile( const CHAR *fullPath,
                                 const migFileHeader *&header )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullPath, "can not be null" ) ;
      SINT64 read = 0 ;
      UINT32 mode = OSS_READONLY |
                    OSS_SHAREREAD |
                    OSS_DIRECTIO ;
      rc = ossOpen( fullPath, mode, OSS_RU|OSS_WU|OSS_RG, _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc:%d",
                 fullPath, rc ) ;
         goto error ;
      }

      rc = ossReadN( &_file, sizeof( migFileHeader ),
                     _buf, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read file:%s, rc:%d",
                 fullPath, rc ) ;
         goto error ;
      }

      header = ( const migFileHeader * )_buf ;
      if ( 0 != ossStrcmp( header->eyeCatcher,
                           MIG_FILE_EYE ) )
      {
         PD_LOG( PDERROR, "invalid file header" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( MIG_LOB_TOOL_VERSION != header->version )
      {
         PD_LOG( PDERROR, "invalid file version:%d", header->version ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      cout << header->toString() << endl ;      
   done:
      return rc ;
   error:
      header = NULL ;
      goto done ;
   }

   INT32 _migLobTool::_createFile( const CHAR *fullPath )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != fullPath, "can not be null" ) ;
      UINT32 mode = OSS_READWRITE |
                    OSS_SHAREREAD |
                    OSS_CREATEONLY |
                    OSS_DIRECTIO ;

      rc = ossOpen( fullPath, mode, OSS_RU|OSS_WU|OSS_RG, _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc:%d",
                 fullPath, rc ) ;
         goto error ;
      }

      _initFileHeader( ( migFileHeader * )_buf ) ;
      rc = ossWriteN( &_file, _buf, sizeof( migFileHeader ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write file[%s], rc:%d",
                 fullPath, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
      goto done ;
   }

   void _migLobTool::_initFileHeader( migFileHeader *header )
   {
      migFileHeader h ;
      ossMemcpy( header, &h, sizeof( h ) ) ;
   }
}

