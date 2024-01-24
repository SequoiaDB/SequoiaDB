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

   Source File Name = sdbConsistencyInspect.cpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data insert/update/delete. This file does NOT include index logic.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2014  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sdbInspect.hpp"
#include "ossUtil.hpp"
#include "utilParam.hpp"
#include "ossVer.hpp"
#include "client.hpp"
#include "utilPasswdTool.hpp"

using namespace engine;


INT32 reallocBuff( CHAR **ppBuf, INT64 &bufSize, INT64 newSize )
{
   INT32 rc = SDB_OK ;
   CHAR *pNewBuf = NULL ;

   if ( bufSize >= newSize )
   {
      goto done ;
   }

   pNewBuf = ( CHAR * )SDB_OSS_REALLOC( *ppBuf, newSize ) ;
   if ( !pNewBuf )
   {
      std::cout << "Error: Failed to allocate buffer. size = "
                << newSize << std::endl ;
      rc = SDB_OOM ;
      goto error ;
   }

   *ppBuf = pNewBuf ;
   bufSize = newSize ;

done:
   return rc ;
error:
   goto done ;
}

/**
** get the index of min bson object
***/
INT32 getMinObjectIndex( ciBson &doc, const INT32 nodeCount )
{
   bson::BSONElement eMin, ee ;

   INT32 idx = 0 ;
   INT32 minIndex = 0 ;
   // find the first valid object
   while ( doc.objs[idx].isEmpty() )
   {
      ++idx ;
      minIndex = idx ;
   }
   // get the id of object
   if ( doc.objs[0].getObjectID( eMin ) )
   {
   }
   // compare to other object
   for ( ; idx < nodeCount ; ++idx )
   {
      if ( !doc.objs[idx].isEmpty() )
      {
         bson::OID id ;
         if ( doc.objs[idx].getObjectID( ee ) )
         {
            if ( ee < eMin )
            {
               eMin = ee ;
               minIndex = idx ;
            }
         }
      }
   }

   return minIndex ;
}

INT32 dumpCiHeader( const ciHeader *header, CHAR *&buffer,
                    INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc  = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      rc = reallocBuff( &buffer, bufferSize, bufferSize + CI_BUFFER_BLOCK ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   len = 0 ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Tool Name    : sdbInspect" OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Tool Version : %d.%d" OSS_NEWLINE,
                       header->_mainVersion, header->_subVersion ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "------------------------------"
                       OSS_NEWLINE "" OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Parameters:" OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Loop        : %d" OSS_NEWLINE,
                       header->_loop ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "action      : %s" OSS_NEWLINE,
                       header->_action ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "coorAddress : %s" OSS_NEWLINE,
                       header->_coordAddr ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "serviceName : %s" OSS_NEWLINE,
                       header->_serviceName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "username    : %s" OSS_NEWLINE,
                       g_username ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "group       : %s" OSS_NEWLINE,
                       header->_groupName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "cs name     : %s" OSS_NEWLINE,
                       header->_csName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "cl name     : %s" OSS_NEWLINE,
                       header->_clName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "file path   : %s" OSS_NEWLINE,
                       header->_filepath ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "output file : %s" OSS_NEWLINE,
                       header->_outfile ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "view        : %s" OSS_NEWLINE,
                       header->_view ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "------------------------------"
                       OSS_NEWLINE "" OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiGroupHeader( const ciGroupHeader *header, CHAR *&buffer,
                         INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc  = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      rc = reallocBuff( &buffer, bufferSize, bufferSize + CI_BUFFER_BLOCK ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   len = 0 ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Replica Group:" OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Group ID     : %d" OSS_NEWLINE,
                       header->_groupID ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Group Name   : %s" OSS_NEWLINE,
                       header->_groupName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Nodes count  : %d" OSS_NEWLINE,
                       header->_nodeCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiNodeSimple( ciLinkList< ciNode > &nodes, CHAR *&buffer,
                        INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc     = SDB_OK ;
   ciNode *node = NULL ;
   INT64 len    = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      rc = reallocBuff( &buffer, bufferSize, bufferSize + CI_BUFFER_BLOCK ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   len = 0 ;
   nodes.resetCurrentNode() ;
   node = nodes.getHead() ;
   while ( NULL != node )
   {
      if ( ciNode::STATE_NORMAL != node->_state )
      {
         len += ossSnprintf( buffer + len,
                             bufferSize - len,
                             "    \"%s\" in node( ServiceName : %s )"
                             OSS_NEWLINE OSS_NEWLINE,
                             ciNode::stateDesc[ node->_state],
                             node->_serviceName ) ;
         CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      }
      node = nodes.next() ;
   }
   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiNode( ciLinkList< ciNode > &link, CHAR *&buffer,
                  INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc     = SDB_OK ;
   ciNode *node = NULL ;
   INT64 len    = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      rc = reallocBuff( &buffer, bufferSize, bufferSize + CI_BUFFER_BLOCK ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   len = 0 ;
   link.resetCurrentNode() ;
   node = link.getHead() ;
   while ( NULL != node )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node index       : %d" OSS_NEWLINE,
                          node->_index) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node ID          : %d" OSS_NEWLINE,
                          node->_nodeID ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node HostName    : %s" OSS_NEWLINE,
                          node->_hostname ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node ServiceName : %s" OSS_NEWLINE,
                          node->_serviceName ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node State       : %s" OSS_NEWLINE,
                          ciNode::stateDesc[ node->_state ] ) ;
      len += ossSnprintf( buffer + len, bufferSize - len, OSS_NEWLINE ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

      node = link.next() ;
   }

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiClHeader( const ciClHeader *header, CHAR *&buffer,
                      INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc  = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      rc = reallocBuff( &buffer, bufferSize, bufferSize + CI_BUFFER_BLOCK ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   len = 0 ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Collection Full Name  : %s" OSS_NEWLINE,
                       header->_fullname ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Main Collection Name  : %s" OSS_NEWLINE,
                       header->_mainClName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   if ( header->_recordCount <= 0 )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    There is no record different" OSS_NEWLINE ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   }
   len += ossSnprintf( buffer + len, bufferSize - len, OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 archiveCiRecord( ciLinkList< ciNode > &nodes,
                       ciLinkList< ciRecord > &link,
                       recordBuffer *rBuffer )
{
   INT32 rc        = SDB_OK ;
   INT32 nodeCount = nodes.count() ;
   ciRecord *rd    = NULL ;
   ciNode   *node  = NULL ;
   const CHAR *pst = NULL ;
   const CHAR *nodeState[] =
   {
      " 1",
      " 0",
      " x"
   } ;

   if ( link.count() > 0 )
   {
      rc = rBuffer->writeBuffer( "  # Node state 1 means node has the record,"
                                 " or 0 means not, and x means node invalid"
                                 OSS_NEWLINE
                                 "  # The order is ascended by node index."
                                 OSS_NEWLINE
                                 "    There is [%d] piece of records that haven't been"
                                 " synchronized."
                                 OSS_NEWLINE "" OSS_NEWLINE, link.count() ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   }
   else
   {
      rc = rBuffer->writeBuffer( "   There is no record different"
                                 OSS_NEWLINE "" OSS_NEWLINE ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   }

   link.resetCurrentNode() ;
   rd = link.getHead() ;
   while ( NULL != rd )
   {
      rc = rBuffer->writeBuffer( "  -record     : %s" OSS_NEWLINE,
                                 rd->_bson.toString().c_str() ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = rBuffer->writeBuffer(  "  -Node State : " ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      nodes.resetCurrentNode() ;
      node = nodes.getHead() ;
      for ( INT32 idx = 0 ; idx < nodeCount ; ++idx )
      {
         ciState st( rd->_state ) ;
         if ( st.hit( idx ) )
         {
            if ( ciNode::STATE_NORMAL != node->_state )
            {
               pst = nodeState[ 2 ] ;
            }
            else
            {
               pst = nodeState[ 0 ] ;
            }
         }
         else
         {
            pst = nodeState[ 1 ] ;
         }
         rc = rBuffer->writeBuffer( pst ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         node = nodes.next() ;
      }
      rc = rBuffer->writeBuffer( OSS_NEWLINE ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      rd = link.next() ;
   }
   rc = rBuffer->writeBuffer( OSS_NEWLINE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = rBuffer->commitBuffer() ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiTail( ciTail &tail, CHAR *&buffer,
                  INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      rc = reallocBuff( &buffer, bufferSize, bufferSize + CI_BUFFER_BLOCK ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Inspect result:" OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total inspected group count       : %d" OSS_NEWLINE,
                       tail._groupCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total inspected collection        : %d" OSS_NEWLINE,
                       tail._clCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total different collections count : %d" OSS_NEWLINE,
                       tail._diffCLCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total different records count     : %d" OSS_NEWLINE,
                       tail._recordCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total time cost                   : %d ms" OSS_NEWLINE,
                       tail._timeCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   if ( 0 == tail._exitCode )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "Reason for exit : exit with no records "
                          "different" OSS_NEWLINE ) ;
   }
   else if ( 1 == tail._exitCode )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "Reason for exit : exit with less than 1%% of "
                          "records not synchronized" OSS_NEWLINE ) ;
   }
   else
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "Reason for exit : loop is limited" OSS_NEWLINE ) ;
   }
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   goto done ;
}

BOOLEAN findCiOffset( ciLinkList< ciOffset > &clOffset, const INT64 offset )
{
   BOOLEAN find = FALSE ;

   clOffset.resetCurrentNode() ;
   ciOffset *off = clOffset.getHead() ;

   while ( NULL != off )
   {
      if ( offset == off->_offset )
      {
         find = TRUE ;
         goto done ;
      }
      off = clOffset.next() ;
   }

done:
   return find ;
}

/**
** read data from file
***/
INT32 readFromFile( OSSFILE &in, INT64 &offset,
                    CHAR *buffer, const INT64 readSize )
{
   INT32 rc       = SDB_OK ;
   ///< read from file
   INT64 restLen  = readSize ;
   INT64 readPos  = 0 ;
   INT64 readLen  = 0 ;

   while( restLen > 0 )
   {
      rc = ossSeekAndRead( &in, offset, buffer + readPos,
                           restLen, &readLen ) ;
      if ( SDB_OK != rc && SDB_INTERRUPT != rc )
      {
         std::cout << "Failed to read data from file" << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
      restLen -= readLen ;
      readPos += readLen ;
   }

   offset += readSize ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write buffer to file
***/
INT32 writeToFile( OSSFILE &out, const CHAR *buffer, const INT64 bufferSize )
{
   INT32 rc        = SDB_OK ;
   ///< write buffer
   INT64 restLen   = bufferSize ;
   INT64 writePos  = 0 ;
   INT64 writeSize = 0 ;
   while( restLen > 0 )
   {
      rc = ossWrite( &out, buffer + writePos, restLen, &writeSize ) ;
      if ( SDB_OK != rc && SDB_INTERRUPT != rc )
      {
         std::cout << "Failed to write data to file" << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
      restLen -= writeSize ;
      writePos += writeSize ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 writeToFileHeader( OSSFILE &out,
                         const CHAR *buffer, const INT64 bufferSize )
{
   INT32 rc        = SDB_OK ;
   ///< write buffer
   INT64 restLen   = bufferSize ;
   INT64 writePos  = 0 ;
   INT64 writeSize = 0 ;
   while( restLen > 0 )
   {
      rc = ossSeekAndWrite( &out, 0, buffer + writePos, restLen, &writeSize ) ;
      if ( SDB_OK != rc && SDB_INTERRUPT != rc )
      {
         std::cout << "Failed to write data to file" << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
      restLen -= writeSize ;
      writePos += writeSize ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

_recordBuffer::_recordBuffer( OSSFILE &out )
: _out( out )
{
   _rBufferSize   = CI_RECORD_BUFFER_SIZE ;
   _rValidSize    = 0 ;
   _rBuffer       = (CHAR*) SDB_OSS_MALLOC( _rBufferSize ) ;
}

_recordBuffer::~_recordBuffer()
{
   if( _rBuffer )
   {
      SDB_OSS_FREE(_rBuffer) ;
      _rBuffer     = NULL ;
   }
}

INT32 _recordBuffer::writeBuffer( const CHAR *pFormat, ... )
{
   INT32 rc = SDB_OK ;
   va_list ap ;
   INT64 len = _rValidSize ;

   if( _rBuffer == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   while ( true )
   {
      va_start( ap, pFormat ) ;
      len += ossVsnprintf( _rBuffer + len, _rBufferSize - len, pFormat, ap ) ;
      va_end( ap ) ;
      if( len >= _rBufferSize - 1 )
      {
         // Buffer space is full
         rc = writeToFile( _out, _rBuffer, _rValidSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         _rValidSize = 0 ;
         len = 0 ;
      }
      else
      {
         // Buffer space is enough
         _rValidSize = len ;
         break ;
      }
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 _recordBuffer::commitBuffer()
{
   INT32 rc = SDB_OK ;

   if( _rBuffer == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if( _rBuffer && _rValidSize > 0 )
   {
      rc = writeToFile( _out, _rBuffer, _rValidSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      _rValidSize = 0 ;
   }
done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciHeader from file
***/
INT32 readCiHeader( OSSFILE &in, ciHeader *header )
{
   INT32 rc          = SDB_OK ;
   INT32 len         = 0 ;
   INT32 mainVersion = 0 ;
   INT32 subVersion  = 0 ;
   INT64 tmpOffset   = 0 ;
   CHAR  eyeCatcher[ CI_EYECATCHER_SIZE ] = { 0 } ;
   CHAR  buffer[ CI_HEADER_SIZE ] = { 0 } ;

   rc = readFromFile( in, tmpOffset, buffer, CI_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // try to copy
   ossMemcpy( eyeCatcher, buffer, CI_EYECATCHER_SIZE ) ;
   if ( 0 != ossStrncmp( CI_HEADER_EYECATCHER,
      eyeCatcher, CI_EYECATCHER_SIZE ) )
   {
      std::cout << "Error: eyeCatcher is invalid" << std::endl ;
      goto error ;
   }
   len += CI_EYECATCHER_SIZE ;

   // try copy main version
   ossMemcpy( &mainVersion, buffer + len, sizeof( INT32 ) ) ;
   if ( mainVersion > header->_mainVersion )
   {
      std::cout << "Error: the main version in file header is larger "
                << "than the main version of current tools" << std::endl ;
      goto error ;
   }
   len += sizeof( INT32 ) ;

   //try copy sub version
   ossMemcpy( &subVersion, buffer + len, sizeof( INT32 ) ) ;
   if ( ( mainVersion == header->_mainVersion ) &&
        ( subVersion > header->_subVersion ) )
   {
      std::cout << "Error: the sub version in file header is larger "
                << "than the sub version of current tools" << std::endl ;
      goto error ;
   }
   len += sizeof( INT32 ) ;

   // copy loop
   ossMemcpy( &header->_loop, buffer + len, sizeof(INT32) ) ;
   if ( 0 >= header->_loop )
   {
      std::cout << "Warning: the value of loop is invalid. "
                << "Modify it to default value(5)." << std::endl ;
      header->_loop = 5 ;
   }
   len += sizeof( INT32 ) ;

   ossMemcpy( &header->_tailSize, buffer + len, sizeof(UINT64) ) ;
   len += sizeof( UINT64 ) ;

   // copy actions
   ossMemcpy( header->_action, buffer + len, CI_ACTION_SIZE ) ;
   if ( 0 != ossStrncmp( CI_ACTION_INSPECT,
                         header->_action, CI_ACTION_SIZE ) &&
        0 != ossStrncmp( CI_ACTION_REPORT,
                         header->_action, CI_ACTION_SIZE ) )
   {
      std::cout << "Error: action is invalid" << std::endl ;
      goto error ;
   }
   len += CI_ACTION_SIZE ;

   // copy coord hostname
   ossMemcpy( header->_coordAddr, buffer + len, CI_HOSTNAME_SIZE + 1 ) ;
   len += CI_HOSTNAME_SIZE + 1 ;
   // copy coord service name
   ossMemcpy( header->_serviceName, buffer + len, CI_SERVICENAME_SIZE + 1 ) ;
   len += CI_SERVICENAME_SIZE + 1 ;
   // copy username and password
   ossMemcpy( g_username, buffer + len, CI_USERNAME_SIZE + 1 ) ;
   len += CI_USERNAME_SIZE + 1 ;
   ossMemcpy( g_password, buffer + len, CI_PASSWD_SIZE + 1 ) ;
   len += CI_PASSWD_SIZE + 1 ;
   // copy group name
   ossMemcpy( header->_groupName, buffer + len, CI_GROUPNAME_SIZE + 1 ) ;
   len += CI_GROUPNAME_SIZE + 1 ;
   // copy collection space name
   ossMemcpy( header->_csName, buffer + len, CI_CS_NAME_SIZE + 1 ) ;
   len += CI_CS_NAME_SIZE + 1 ;
   // copy collection name
   ossMemcpy( header->_clName, buffer + len, CI_CL_NAME_SIZE + 1) ;
   len += CI_CL_NAME_SIZE + 1 ;
   // skip file path
   ossMemcpy( header->_filepath, buffer + len, OSS_MAX_PATHSIZE + 1) ;
   len += OSS_MAX_PATHSIZE + 1 ;
   // skip out file
   ossMemcpy( header->_outfile, buffer + len, OSS_MAX_PATHSIZE + 1) ;
   len += OSS_MAX_PATHSIZE + 1 ;
   // copy view format string
   ossMemcpy( header->_view, buffer + len, CI_VIEWOPTION_SIZE + 1 ) ;
   len += CI_VIEWOPTION_SIZE + 1 ;
   // copy padding? it seems useless
   // skip first

done:
   return rc ;
error:
   rc = HEADER_PARSE_ERROR ;
   goto done ;
}

/**
** copy ciHeader data to buffer
***/
INT32 ciHeaderToBuffer( const ciHeader *header, CHAR *&buffer,
                        INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;
   INT32 pos = 0 ;

   validSize = CI_HEADER_SIZE ;
   if ( validSize > bufferSize )
   {
      rc = reallocBuff( &buffer, bufferSize, validSize ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   ossMemcpy( buffer + pos, header->_eyeCatcher, CI_EYECATCHER_SIZE ) ;
   pos += CI_EYECATCHER_SIZE ;
   ossMemcpy( buffer + pos, &header->_mainVersion, sizeof(INT32) ) ;
   pos += sizeof( INT32 ) ;
   ossMemcpy( buffer + pos, &header->_subVersion, sizeof(INT32) ) ;
   pos += sizeof( INT32 ) ;
   ossMemcpy( buffer + pos, &header->_loop, sizeof(INT32) ) ;
   pos += sizeof( INT32 ) ;
   ossMemcpy( buffer + pos, &header->_tailSize, sizeof(UINT64) ) ;
   pos += sizeof( UINT64 ) ;
   ossMemcpy( buffer + pos, header->_action, CI_ACTION_SIZE ) ;
   pos += CI_ACTION_SIZE ;
   ossMemcpy( buffer + pos, header->_coordAddr, CI_HOSTNAME_SIZE + 1 ) ;
   pos += CI_HOSTNAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_serviceName, CI_SERVICENAME_SIZE + 1 ) ;
   pos += CI_SERVICENAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, g_username, CI_USERNAME_SIZE + 1 ) ;
   pos += CI_USERNAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, g_password, CI_PASSWD_SIZE + 1 ) ;
   pos += CI_PASSWD_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_groupName, CI_GROUPNAME_SIZE + 1 ) ;
   pos += CI_GROUPNAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_csName, CI_CS_NAME_SIZE + 1 ) ;
   pos += CI_CS_NAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_clName, CI_CL_NAME_SIZE + 1 ) ;
   pos += CI_CL_NAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_filepath, OSS_MAX_PATHSIZE + 1 ) ;
   pos += OSS_MAX_PATHSIZE + 1 ;
   ossMemcpy( buffer + pos, header->_outfile, OSS_MAX_PATHSIZE + 1 ) ;
   pos += OSS_MAX_PATHSIZE + 1 ;
   ossMemcpy( buffer + pos, header->_view, CI_VIEWOPTION_SIZE + 1 ) ;
   pos += CI_VIEWOPTION_SIZE + 1 ;
   ossMemset( buffer + pos, 0, validSize - pos ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciHeader to file
***/
INT32 writeCiHeader( OSSFILE &out, const ciHeader *header,
                     CHAR *&buffer, INT64 &bufferSize,
                     INT64 &validSize, BOOLEAN reWrite = FALSE )
{
   INT32 rc = SDB_OK ;

   rc = ciHeaderToBuffer( header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   if ( reWrite )
   {
      rc = writeToFileHeader( out, buffer, validSize ) ;
   }
   else
   {
      rc = writeToFile( out, buffer, validSize ) ;
   }
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciGroupHeader from file
***/
INT32 readCiGroupHeader( OSSFILE &in, INT64 &offset, ciGroupHeader *header )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_GROUP_HEADER_SIZE ] = { 0 } ;

   rc = readFromFile( in, offset, buffer, CI_GROUP_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   //offset += CI_GROUP_HEADER_SIZE ;

   ossMemcpy( &header->_groupID, buffer + len, sizeof( INT32 ) ) ;
   len += sizeof( INT32 ) ;
   ossMemcpy( &header->_nodeCount, buffer + len, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( &header->_clCount, buffer + len, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( &header->_groupName, buffer + len, CI_GROUPNAME_SIZE ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciGroupHeader to file
***/
INT32 writeCiGroupHeader( OSSFILE &out, const ciGroupHeader *header )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_GROUP_HEADER_SIZE ] = { 0 } ;

   ossMemcpy( buffer + len, &header->_groupID, sizeof( INT32 ) ) ;
   len += sizeof( INT32 ) ;
   ossMemcpy( buffer + len, &header->_nodeCount, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( buffer + len, &header->_clCount, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( buffer + len, header->_groupName, CI_GROUPNAME_SIZE ) ;

   rc = writeToFile( out, buffer, CI_GROUP_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciClHeader from file
***/
INT32 readCiClHeader( OSSFILE &in, INT64 &offset, ciClHeader *header )
{
   INT32 rc = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_CL_HEADER_SIZE ] = { 0 } ;

   rc = readFromFile( in, offset, buffer, CI_CL_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   //offset += CI_CL_HEADER_SIZE ;

   ossMemcpy( &header->_recordCount, buffer + len, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( header->_fullname, buffer + len, CI_CL_FULLNAME_SIZE ) ;
   len += CI_CL_FULLNAME_SIZE ;
   ossMemcpy( header->_mainClName, buffer + len, CI_CL_FULLNAME_SIZE ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciClHeader to file
***/
INT32 writeCiClHeader( OSSFILE &out, const ciClHeader *header )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_CL_HEADER_SIZE ] = { 0 } ;

   ossMemcpy( buffer + len, &header->_recordCount, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( buffer + len, header->_fullname, CI_CL_FULLNAME_SIZE ) ;
   len += CI_CL_FULLNAME_SIZE ;
   ossMemcpy( buffer + len, header->_mainClName, CI_CL_FULLNAME_SIZE ) ;

   rc = writeToFile( out, buffer, CI_CL_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

BOOLEAN normalNodes( ciLinkList< ciNode > &nodes )
{
   BOOLEAN normal = TRUE ;
   ciNode *curNode = NULL ;

   nodes.resetCurrentNode() ;
   curNode = nodes.getHead() ;
   while ( NULL != curNode )
   {
      if ( ciNode::STATE_NORMAL != curNode->_state )
      {
         normal = FALSE ;
         break ;
      }
      curNode = nodes.next() ;
   }

   return normal ;
}

/**
** read ciNode from file
***/
INT32 readCiNode( OSSFILE &in, INT64 &offset,
                  const ciGroupHeader &header, ciLinkList< ciNode > &nodes )
{
   INT32 rc                    = SDB_OK ;
   CHAR buffer[ CI_NODE_SIZE ] = { 0 } ;
   INT32 pos                   = 0 ;
   UINT32 idx                  = 0 ;
   while ( idx < header._nodeCount )
   {
      pos = 0 ;
      ossMemset( buffer, 0, CI_NODE_SIZE ) ;

      rc = readFromFile( in, offset, buffer, CI_NODE_SIZE ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      //offset += CI_NODE_SIZE ;
      ciNode *node = nodes.createNode() ;
      if ( NULL == node )
      {
         std::cout << "Error: failed to allocate ciNode" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( &node->_index, buffer + pos, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( &node->_nodeID, buffer + pos, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( &node->_state, buffer + pos, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( node->_hostname, buffer + pos, CI_HOSTNAME_SIZE + 1) ;
      pos += CI_HOSTNAME_SIZE + 1 ;
      ossMemcpy( node->_serviceName, buffer + pos, CI_SERVICENAME_SIZE + 1 );

      nodes.add( node ) ;
      ++idx ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** copy ciNode data to buffer
***/
INT32 ciNodeToBuffer( ciLinkList< ciNode > &nodes, CHAR *&buffer,
                      INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc        = SDB_OK ;
   INT64 pos       = 0 ;
   ciNode *curNode = NULL ;
   UINT32 count    = nodes.count() ;
   INT32 unitLen   = CI_NODE_SIZE ;

   validSize = count * unitLen ;

   if ( validSize > bufferSize )
   {
      rc = reallocBuff( &buffer, bufferSize, validSize ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   nodes.resetCurrentNode() ;
   curNode = nodes.getHead() ;
   while ( NULL != curNode )
   {
      ossMemcpy( buffer + pos, &curNode->_index, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( buffer + pos, &curNode->_nodeID, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( buffer + pos, &curNode->_state, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( buffer + pos, curNode->_hostname, CI_HOSTNAME_SIZE + 1) ;
      pos += CI_HOSTNAME_SIZE + 1 ;
      ossMemcpy( buffer + pos, curNode->_serviceName,
                 CI_SERVICENAME_SIZE + 1 ) ;
      pos += CI_SERVICENAME_SIZE + 1 ;
      curNode = nodes.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciNode to file
***/
INT32 writeCiNode( OSSFILE &out, ciLinkList< ciNode > &nodes,
                   CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;

   rc = ciNodeToBuffer( nodes, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** get next record
***/
INT32 getNext( ciState &st, ciLinkList< ciCursor > &cursors,
               const INT32 minIndex, ciBson &docs, BOOLEAN inited = TRUE )
{
   INT32 rc = SDB_OK ;
   cursors.resetCurrentNode() ;
   ciCursor *cursor = cursors.getHead() ;
   INT32 idx = 0 ;
   while ( NULL != cursor )
   {
      // check last compare is all the same.
      // if not, get next record of cursor which contains min bson object.
      if ( st.hit( ALL_THE_SAME_BIT ) || ( st.hit( idx ) ) )
      {
         if ( NULL != cursor->_cursor )
         {
            rc = cursor->_cursor->next( docs.objs[ idx ] ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  std::cout << "Error: get record failed, rc = "
                            << rc << std::endl ;
                  rc = CI_INSPECT_ERROR ; // redo inspect from begin
                  goto error ;
               }
               else
               {
                  docs.objs[ idx ] = sdbclient::_sdbStaticObject ;
                  rc = SDB_OK ; // need reset the return value
               }
            }
         }
         else
         {
            if ( !inited )
            {
               docs.objs[ idx ] = sdbclient::_sdbStaticObject ;
            }
         }
      }
      ++idx ;
      cursor = cursors.next() ;
   }

   if ( !inited )
   {
      for ( ; idx < MAX_NODE_COUNT ; ++idx )
      {
         docs.objs[ idx ] = sdbclient::_sdbStaticObject ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/**
** get the cursor of records, the collection full name specified
***/
INT32 getCiCursor( ciLinkList< ciNode > &nodes, const CHAR* clName,
                   ciLinkList< ciCursor > &cursors, bson::BSONObj &con,
                   BOOLEAN orderCon = FALSE )
{
   INT32 rc = SDB_OK ;

   nodes.resetCurrentNode() ;
   ciNode *curNode = nodes.getHead() ;
   while ( NULL != curNode )
   {
      ciCursor *cursor = cursors.createNode() ;
      if ( NULL == cursor )
      {
         std::cout << "Error: failed to allocate ciCursor" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      cursor->_nodeID = curNode->_nodeID ;
      cursor->_index = curNode->_index ;


      sdbclient::sdb *db = curNode->_db ;
      sdbclient::sdbCollection cl ;
      sdbclient::sdbCursor *cr = NULL ;

      if ( NULL == db )
      {
         db = new sdbclient::sdb() ;
         if ( NULL == db )
         {
            std::cout << "Error: failed to allocate sdbclient::sdb"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         curNode->_db = db;
         rc = db->connect( curNode->_hostname, curNode->_serviceName,
                           g_username, g_password ) ;
         if ( SDB_OK != rc )
         {
            curNode->_state = ciNode::STATE_DISCONN ;
         }
      }

      if ( ciNode::STATE_DISCONN != curNode->_state )
      {
         curNode->_state = ciNode::STATE_NORMAL ;
         rc = db->getCollection( clName, cl ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            curNode->_state = ciNode::STATE_CLNOTEXIST ;
         }
         else if ( SDB_OK != rc )
         {
            curNode->_state = ciNode::STATE_CLFAILED ;
         }
      }

      if ( ciNode::STATE_NORMAL == curNode->_state )
      {
         cr = new sdbclient::sdbCursor() ;
         if ( NULL == cr )
         {
            std::cout << "Error: failed to allocate sdbclient::sdbCursor"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }

         // success to get cl
         if ( orderCon )
         {
            rc = cl.query( *cr, sdbclient::_sdbStaticObject,
                           sdbclient::_sdbStaticObject, con ) ;
         }
         else
         {
            rc = cl.query( *cr, con ) ;
         }
         if ( SDB_OK != rc )
         {
            DELETE_PTR(cr);
            curNode->_state = ciNode::STATE_CUSURFAILED ;
         }
      }
      cursor->_db = db ;
      cursor->_cursor = cr ;
      rc = SDB_OK ;

      cursors.add( cursor ) ;
      curNode = nodes.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** query bson record in each cursor
***/
BOOLEAN recordQuery( ciLinkList< ciNode > &nodes,
                     ciLinkList< ciCursor > &cursors,
                     ciState &state, bson::BSONObj &obj, INT32 &rc )
{
   BOOLEAN same = FALSE ;
   INT32 nodeCount = nodes.count() ;
   ciNode *node = NULL ;
   ciBson docs ;

   state.reset() ;
   // it's a trick to make sure that all cursors can get next.
   state.set( ALL_THE_SAME_BIT ) ;
   rc = getNext( state, cursors, 0, docs, FALSE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), done ) ;

   nodes.resetCurrentNode() ;
   node = nodes.getHead() ;
   state.reset() ;

   for ( INT32 idx = 0 ; idx < nodeCount ; ++idx, node = nodes.next() )
   {
      // the collection exists but is not available in node,
      // we assume the record exists
      if ( ciNode::STATE_NORMAL != node->_state &&
           ciNode::STATE_CLNOTEXIST != node->_state )
      {
         state.set( idx ) ;
      }
      else if ( ciNode::STATE_NORMAL == node->_state &&
                docs.objs[idx].equal( obj ) )
      {
         state.set( idx ) ;
      }
   }

   if ( state._state == ( ( 1 << nodeCount ) - 1 ) || state._state == 0 )
   {
      same = TRUE ;
   }

done:
   return same ;
}

/**
** read ciRecord from file
***/
INT32 readCiRecord( OSSFILE &in, INT64 &offset,
                    ciLinkList< ciNode > &nodes,
                    const ciClHeader &header,
                    ciLinkList< ciRecord > &records, BOOLEAN dump = FALSE )
{
   INT32 rc         = SDB_OK ;
   CHAR *bsonBuffer = NULL ;
   INT64 bufferLen  = 0 ;

   UINT32 idx = 0 ;
   while ( idx < header._recordCount )
   {
      INT64 recordLen = 0 ;
      CHAR  state ;
      rc = readFromFile( in, offset, (CHAR *)&recordLen, sizeof( INT32 ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      if ( recordLen > bufferLen )
      {
         rc = reallocBuff( &bsonBuffer, bufferLen, recordLen ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      // read bson
      rc = readFromFile( in, offset, ( CHAR * )bsonBuffer, recordLen ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // read state
      rc = readFromFile( in, offset, ( CHAR * )&state, sizeof( CHAR ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // make a condition of query
      bson::BSONObj obj( bsonBuffer ) ;
      bson::BSONElement e ;
      obj.getObjectID( e ) ;
      bson::BSONObj con = bob().append( e ).obj() ;

      ciLinkList< ciCursor > cursors ;
      if ( !dump )
      {
         rc = getCiCursor( nodes, header._fullname, cursors, con ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      }

      ciState st ;
      if ( dump || !recordQuery( nodes, cursors, st, obj, rc ) )
      {
         ciRecord *record = records.createNode() ;
         if ( NULL == record )
         {
            std::cout << "Error: failed to allocate ciRecord "
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         record->_bson = obj.copy() ;
         record->_len = obj.objsize() ;
         record->_state = dump ? state : st._state ;
         records.add( record ) ;
      }
      ++idx ;
   }

done:
   if ( NULL != bsonBuffer )
   {
      SDB_OSS_FREE( bsonBuffer ) ;
      bsonBuffer = NULL ;
   }
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** copy record data to buffer
***/
INT32 ciRecordToBuffer( ciLinkList< ciRecord > &records, CHAR *&buffer,
                        INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc            = SDB_OK ;
   INT64 pos           = 0 ;
   ciRecord *curRecord = NULL ;

   validSize = 0 ;

   records.resetCurrentNode() ;
   curRecord = records.getHead() ;
   while ( NULL != curRecord )
   {
      validSize += sizeof( INT32 ) ;
      validSize += curRecord->_len ;
      validSize += sizeof( CHAR ) ;
      curRecord = records.next() ;
   }

   if ( validSize > bufferSize )
   {
      rc = reallocBuff( &buffer, bufferSize, validSize ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   records.resetCurrentNode() ;
   curRecord = records.getHead() ;
   while ( NULL != curRecord )
   {
      ossMemcpy( buffer + pos, &curRecord->_len, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      INT32 size = curRecord->_bson.objsize() ;
      ossMemcpy( buffer + pos, curRecord->_bson.objdata(), size ) ;
      pos += size ;
      ossMemcpy( buffer + pos, &curRecord->_state, sizeof( CHAR ) ) ;
      pos += 1 ;

      curRecord = records.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciRecord to file
***/
INT32 writeCiRecord( OSSFILE &out, ciLinkList< ciRecord > &records,
                     CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;

   rc = ciRecordToBuffer( records, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciOffset from file
***/
INT32 readCiOffset( OSSFILE &in, INT64 &offset,
                    const INT32 count, ciLinkList< ciOffset > &clo )
{
   INT32 rc = SDB_OK ;
   INT32 idx = 0 ;
   INT32 pos = 0 ;
   CHAR *buffer = NULL ;

   INT32 readSize = count * sizeof( INT64 ) ;
   buffer = ( CHAR * )SDB_OSS_MALLOC( readSize ) ;
   if ( NULL == buffer )
   {
      std::cout << "Error: failed to allocate memory. size = "
                << readSize << std::endl ;
      rc = SDB_OOM ;
      goto error ;
   }

   rc = readFromFile( in, offset, buffer, readSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   while ( idx < count )
   {
      ciOffset *clOff = clo.createNode() ;
      if ( NULL == clOff )
      {
         std::cout << "Error: failed to allocate ciClOffset" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( &clOff->_offset, buffer + pos, sizeof( INT64 ) ) ;
      pos += sizeof( INT64 ) ;

      clo.add( clOff ) ;
      ++idx ;
   }

done:

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** copy ciClOffset data to buffer
***/
INT32 ciOffsetToBuffer( ciLinkList< ciOffset > &offsets,
                        CHAR *&buffer, INT64 &bufferSize,
                        INT64 &validSize )
{
   INT32 rc            = SDB_OK ;
   INT64 pos           = 0 ;
   ciOffset *curNode = NULL ;
   UINT32 count        = offsets.count() ;
   INT32 unitLen       = sizeof( INT64 ) ;

   validSize = count * unitLen ;

   if ( validSize > bufferSize )
   {
      rc = reallocBuff( &buffer, bufferSize, validSize ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   //ossMemcpy( buffer + pos, &count, sizeof( INT32 ) ) ;
   //pos += sizeof( INT32 ) ;
   offsets.resetCurrentNode() ;
   curNode = offsets.getHead() ;
   while ( NULL != curNode )
   {
      ossMemcpy( buffer + pos, &curNode->_offset, sizeof( INT64 ) ) ;
      pos += sizeof( INT64 ) ;
      curNode = offsets.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciClOffset to file
***/
INT32 writeCiClOffset( OSSFILE &out, ciLinkList< ciOffset > &offsets,
                       CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;

   rc = ciOffsetToBuffer( offsets, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 readMainSubCl( OSSFILE &in, INT64 &offset,
                     const INT32 count, mainCl &mainCls )
{
   INT32 rc     = SDB_OK ;
   INT32 idx    = 0 ;
   INT32 subCount  = 0 ;
   CHAR buffer[ CI_CL_FULLNAME_SIZE + 1 ] = { 0 } ;

   mainCls.clear() ;

   while ( idx < count )
   {
      rc = readFromFile( in, offset, buffer, CI_CL_FULLNAME_SIZE ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      std::string mainClName = std::string( buffer ) ;
      rc = readFromFile( in, offset, (CHAR *)&subCount, sizeof( INT32 ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      INT32 subIdx = 0 ;
      while( subIdx < subCount )
      {
         rc = readFromFile( in, offset, buffer, CI_CL_FULLNAME_SIZE ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         std::string subClName = std::string( buffer ) ;
         subCl &subCls = mainCls[ mainClName ] ;
         subCls.push_back( subClName ) ;

         ++subIdx ;
      }
      ++idx ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}
INT32 writeMainSubCl( OSSFILE &out, const mainCl &mainCls, CHAR *&buffer,
                      INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc       = SDB_OK ;
   INT64 pos      = 0 ;
   INT64 totalLen = 0 ;

   mainCl::const_iterator it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      totalLen += CI_CL_FULLNAME_SIZE ;
      totalLen += sizeof( INT32 ) ;

      subCl::const_iterator sub_it = it->second.begin() ;
      for ( ; sub_it != it->second.end() ; ++sub_it )
      {
         totalLen += CI_CL_FULLNAME_SIZE ;
      }
   }

   validSize = totalLen ;

   if ( validSize > bufferSize )
   {
      rc = reallocBuff( &buffer, bufferSize, validSize ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      ossMemcpy( buffer + pos, it->first.c_str(), CI_CL_FULLNAME_SIZE ) ;
      pos += CI_CL_FULLNAME_SIZE ;

      INT32 count = it->second.size() ;
      ossMemcpy( buffer + pos, ( CHAR * )&count, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;

      subCl::const_iterator sub_it = it->second.begin() ;
      for ( ; sub_it != it->second.end() ; ++sub_it )
      {
         ossMemcpy( buffer + pos, sub_it->c_str(), CI_CL_FULLNAME_SIZE ) ;
         pos += CI_CL_FULLNAME_SIZE ;
      }
   }

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciTail from file
***/
INT32 readCiTail( OSSFILE &in, const INT64 offset, ciTail *tail )
{
   INT32 rc        = SDB_OK ;
   INT64 tmpOffset = offset ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_exitCode, sizeof( INT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_groupCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_clCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_diffCLCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_mainClCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_recordCount, sizeof( INT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_timeCount, sizeof( UINT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readMainSubCl( in, tmpOffset, tail->_mainClCount, tail->_mainCls ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readCiOffset( in, tmpOffset, tail->_groupCount, tail->_groupOffset );
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciTail to file
***/
INT32 writeCiTail( OSSFILE &out, ciTail *tail,
                   CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;
   INT32 len = 0 ;

   SDB_ASSERT( ( tail->_groupCount == tail->_groupOffset.count() ),
      "count of group is valid" ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_exitCode, sizeof( INT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( INT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_groupCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_clCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_diffCLCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_mainClCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_recordCount, sizeof( INT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( INT64 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_timeCount, sizeof( UINT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT64 ) ;

   rc = writeMainSubCl( out, tail->_mainCls,
                        buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += validSize ;

   rc = writeCiClOffset( out, tail->_groupOffset,
                         buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   validSize += len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpOneCl( OSSFILE &in, OSSFILE &out, ciOffset *groupOffset,
                 ciLinkList< ciOffset > &clOffsets, const CHAR *clName,
                 CHAR *&buffer, INT64 &bufferSize, INT64 &validSize,
                 recordBuffer *rBuffer )
{
   INT32 rc     = SDB_OK ;
   UINT32 idx   = 0 ;
   INT64 offset = 0 ;
   ciGroupHeader header ;
   ciLinkList< ciNode > nodes ;

   if ( NULL == groupOffset )
   {
      goto done ;
   }

   offset = groupOffset->_offset ;
   // read group
   rc = readCiGroupHeader( in, offset, &header ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // read global nodes info
   nodes.clear() ;
   rc = readCiNode( in, offset, header, nodes ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   while ( idx < header._clCount )
   {
      ciClHeader clHeader ;
      ciLinkList< ciRecord > records ;
      ciLinkList< ciNode > nodesForCL ;

      if ( !findCiOffset ( clOffsets, offset ) )
      {
         INT64 clOffset = offset ;
         rc = readCiClHeader( in, offset, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciClHeader" << std::endl ;
            goto error ;
         }

         rc = readCiNode( in, offset, header, nodesForCL ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciNode" << std::endl ;
            goto error ;
         }

         if ( NULL == clName || 0 == ossStrncmp( clName, clHeader._fullname,
                                                 CI_CL_NAME_SIZE ) )
         {
            // find and remember the offset
            ciOffset *cl = clOffsets.createNode() ;
            if ( NULL == cl )
            {
               std::cout << "Error: failed to allocate ciOffset"
                  << std::endl;
               rc = SDB_OOM ;
               goto error ;
            }
            cl->_offset = clOffset ;
            clOffsets.add( cl ) ;

            // dump
            rc = dumpCiClHeader( &clHeader, buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            // dump group and node, if view option is "collection"
            // dump group
            rc = dumpCiGroupHeader( &header, buffer, bufferSize, validSize );
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            // dump nodes
            rc = dumpCiNode( nodesForCL, buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            if ( clHeader._recordCount > 0 )
            {
               records.clear() ;

               rc = readCiRecord( in, offset, nodesForCL,
                                  clHeader, records, TRUE ) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
               rc = archiveCiRecord( nodesForCL, records, rBuffer) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            }

            rc = dumpOneCl( in, out, groupOffset->_next, clOffsets,
                            clHeader._fullname, buffer,
                            bufferSize, validSize, rBuffer ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            // collection found and dumped, then exit
            //goto done ;
         }
      }

      ++idx ;
   }
done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** initialize header with file specified
***/
INT32 initialize( ciHeader *header )
{
   INT32 rc                            = SDB_OK ;
   INT64 fileSize                      = 0 ;
   BOOLEAN opened                      = FALSE ;
   OSSFILE file ;
   ciHeader oldheader ;

   rc = ossOpen( header->_filepath, OSS_READONLY, OSS_RU, file ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file specified" << std::endl ;
      goto error ;
   }
   opened = TRUE ;

   rc = ossGetFileSize( &file, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: fail to get file size" << std::endl ;
      goto error ;
   }

   if ( fileSize < CI_HEADER_SIZE )
   {
      std::cout << "Error: file size is less than " << CI_HEADER_SIZE
                << std::endl ;
      goto error ;
   }

   rc = readCiHeader( file, &oldheader ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // skip eye catcher
   // skip main version
   // skip sub version
   // copy loop
   ossMemcpy( &header->_loop, &oldheader._loop, sizeof(INT32) ) ;
   // copy actions
   ossMemcpy( header->_action, oldheader._action, CI_ACTION_SIZE ) ;
   // copy coord hostname
   ossMemcpy( header->_coordAddr,
              oldheader._coordAddr, CI_HOSTNAME_SIZE + 1 ) ;
   // copy coord service name
   ossMemcpy( header->_serviceName,
              oldheader._serviceName, CI_SERVICENAME_SIZE + 1 ) ;
   // copy user name and password
   //ossMemcpy( header->_user, oldheader._user, CI_USERNAME_SIZE + 1 ) ;
   //ossMemcpy( header->_psw, oldheader._psw, CI_PASSWD_SIZE + 1 ) ;

   // copy group name
   ossMemcpy( header->_groupName,
              oldheader._groupName, CI_GROUPNAME_SIZE + 1 ) ;
   // copy collection space name
   ossMemcpy( header->_csName, oldheader._csName, CI_CS_NAME_SIZE + 1 ) ;
   // copy collection name
   ossMemcpy( header->_clName, oldheader._clName, CI_CL_NAME_SIZE + 1) ;
   // skip file path
   // skip out file
   // copy view format string
   ossMemcpy( header->_view, oldheader._view, CI_VIEWOPTION_SIZE + 1 ) ;

done:
   if ( opened )
   {
      ossClose( file ) ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 getMainAndSubCl( sdbclient::sdb *coord, mainCl &mainCls )
{
   INT32 rc = SDB_OK ;
   sdbclient::sdbCursor cursor ;
   bson::BSONObj record ;

   if ( NULL == coord )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !coord->isValid() )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   rc = coord->getSnapshot( cursor, 8 ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = cursor.next( record ) ;
   while ( SDB_DMS_EOC != rc )
   {
      if ( SDB_OK != rc )
      {
         std::cout << "Error: fail to get next record in cursor"
                   << std::endl ;
         goto error ;
      }
      else
      {
         bson::BSONElement mainCL ;
         mainCL = record.getField( "MainCLName" ) ;
         if ( !mainCL.eoo() )
         {
            std::string mainName = mainCL.String() ;
            bson::BSONElement subClName ;
            subClName = record.getField( "Name" ) ;
            std::string subName = subClName.String() ;
            subCl &subCls = mainCls[ mainName ] ;
            subCls.push_back( subName ) ;
         }
      }
      rc = cursor.next( record ) ;
   }

   rc = SDB_OK ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** get groups through coord
***/
INT32 getCiGroup( sdbclient::sdb *coord, const CHAR *groupName,
                  ciLinkList< ciGroup > &groupList )
{
   INT32 rc = SDB_OK ;

   BOOLEAN hasGroup = ( 0 != ossStrncmp( "", groupName,
                        CI_GROUPNAME_SIZE ) ) ;
   bson::BSONObj obj ;
   sdbclient::sdbCursor cursor ;

   if ( NULL == coord )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !coord->isValid() )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   rc = coord->listReplicaGroups( cursor ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to list replica groups" << std::endl ;
      goto error ;
   }

   rc = cursor.next( obj ) ;
   while( SDB_DMS_EOC != rc )
   {
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get current record." << std::endl ;
         goto error ;
      }
      else
      {
         bson::BSONElement name = obj.getField( "GroupName" ) ;
         std::string beginWith = name.String().substr( 0, 3 ) ;
         if ( 0 != ossStrncmp( beginWith.c_str(),
                               "SYS", ossStrlen( "SYS") ) )
         {
            // fill group item
            if ( !hasGroup || ( 0 == ossStrncmp( name.String().c_str(),
                 groupName, CI_GROUPNAME_SIZE ) ) )
            {
               ciGroup *group = groupList.createNode() ;
               if ( NULL == group )
               {
                  std::cout << "Error: failed to allocate ciGroup"
                            << std::endl ;
                  rc = SDB_OOM ;
                  goto error ;
               }

               ossStrncpy( group->_groupName, name.String().c_str(),
                           CI_GROUPNAME_SIZE ) ;
               group->_groupID = obj.getField( "GroupID" ).Int() ;

               groupList.add( group ) ;
            }
         }
      }
      rc = cursor.next( obj ) ;
   }

   if ( 0 >= groupList.count() )
   {
      std::cout << "Error: Cannot get replica group: "
                << groupName << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = SDB_OK ;
done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** get nodes info through group
***/
INT32 getCiNode( sdbclient::sdb *coord, ciGroup *group,
                 ciGroupHeader &header, ciLinkList<ciNode> &nodeList )
{
   INT32 rc = SDB_OK ;

   if ( NULL == coord )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL != group )
   {
      INT32 index = 0 ;
      // fill member of group
      header._groupID = group->_groupID ;
      ossMemset( header._groupName, 0, CI_GROUPNAME_SIZE ) ;
      ossMemcpy( header._groupName, group->_groupName, CI_GROUPNAME_SIZE ) ;

      // query replica group
      if ( !coord->isValid() )
      {
         rc = SDB_NETWORK ;
         goto error ;
      }

      sdbclient::sdbReplicaGroup rg ;
      rc = coord->getReplicaGroup( group->_groupName, rg ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get replica group: "
                   << group->_groupName << std::endl ;
         goto error ;
      }

      //get master node to make sure master is the head node of list
      sdbclient::sdbNode master ;
      rc = rg.getMaster( master ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get master node of group: "
                   << rg.getName() << std::endl ;
         goto error ;
      }

      ciNode *masterNode = nodeList.createNode() ;
      if ( NULL == masterNode )
      {
         std::cout << "Error: failed to allocate ciNode"
                   << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemcpy( masterNode->_hostname, master.getHostName(),
                 CI_HOSTNAME_SIZE ) ;
      ossMemcpy( masterNode->_serviceName, master.getServiceName(),
                 CI_SERVICENAME_SIZE ) ;
      ++index ;
      masterNode->_index = index ;
      nodeList.add( masterNode ) ;

      // query slave nodes of group
      bson::BSONObj result ;
      rc = rg.getDetail( result ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get detail of replica group: "
                   << rg.getName() << std::endl ;
         goto error ;
      }

      bson::BSONElement eGroup = result.getField("Group") ;
      if( bson::Array == eGroup.type() )
      {
         std::vector<bson::BSONElement> nodes = eGroup.Array() ;
         std::vector<bson::BSONElement>::iterator cit = nodes.begin() ;
         while ( nodes.end() != cit )
         {
            bson::BSONObj bsonNode ;
            cit->Val( bsonNode ) ;

            ciNode *node = nodeList.createNode() ;
            if ( NULL == node )
            {
               std::cout << "Error: failed to allocate ciNode" << std::endl ;
               rc = SDB_OOM ;
               goto error ;
            }
            // get hostname of node
            std::string hostname = bsonNode.getField( "HostName" ).String() ;
            // get servicename of node
            std::vector<bson::BSONElement> service ;
            service = bsonNode.getField( "Service" ).Array() ;
            std::string servicename = service[0][ "Name" ].String() ;
            INT32 nodeID = bsonNode.getField("NodeID").Int() ;

            if ( ( 0 == ossStrncmp( master.getHostName(),
                                    hostname.c_str(),
                                    CI_HOSTNAME_SIZE ) ) &&
               ( 0 == ossStrncmp( master.getServiceName(),
                                  servicename.c_str(),
                                  CI_SERVICENAME_SIZE ) ) )
            {
               masterNode->_nodeID = nodeID ;
               ++cit ;
               delete node ;
               node = NULL ;
               continue ;
            }

            // not master node
            ossStrncpy( node->_hostname, hostname.c_str(),
                        CI_HOSTNAME_SIZE ) ;
            ossStrncpy( node->_serviceName, servicename.c_str(),
                        CI_SERVICENAME_SIZE ) ;
            {
               sdbclient::sdb db ;
               rc = db.connect( node->_hostname, node->_serviceName,
                                g_username, g_password ) ;
               if ( SDB_OK != rc )
               {
                  node->_state = ciNode::STATE_DISCONN ;
                  rc = SDB_OK ;
               }
            }
            node->_nodeID = nodeID ;
            ++index ;
            node->_index = index ;
            // add to group
            nodeList.add( node ) ;
            ++cit ;
         }
         header._nodeCount = nodeList.count() ;
      }
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

BOOLEAN isInMainSubCl( const CHAR *mainClName,
                       const CHAR *subClName, const mainCl &mainCls )
{
   BOOLEAN in = FALSE ;

   mainCl::const_iterator it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      if ( 0 == it->first.compare( mainClName ) )
      {
         subCl::const_iterator sub_it = it->second.begin() ;
         for ( ; sub_it != it->second.end() ; ++sub_it )
         {
            if ( 0 == sub_it->compare( subClName ) )
            {
               in = TRUE ;
               break ;
            }
         }
      }
   }

   return in ;
}

const CHAR* getMainClName( const mainCl &mainCls, const CHAR *subClName )
{
   const CHAR *pName = NULL ;
   mainCl::const_iterator it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      subCl::const_iterator sub_it = it->second.begin() ;
      for ( ; sub_it != it->second.end() ; ++sub_it )
      {
         if ( 0 == sub_it->compare( subClName ) )
         {
            pName = it->first.c_str() ;
            break ;
         }
      }
   }

   return pName ;
}
/**
** get collection space in nodes
***/
INT32 getCiCollection( ciNode *master, const CHAR *csName,
                       const CHAR *clName,
                       ciLinkList< ciCollection > &collections,
                       const mainCl &mainCls )
{
   INT32 rc              = SDB_OK ;
   BOOLEAN hasCollection = FALSE ;
   BOOLEAN hasCs         = FALSE ;
   sdbclient::sdb db ;
   sdbclient::sdbCursor cursor ;
   bson::BSONObj collection ;
   CHAR fullName[ CI_CL_FULLNAME_SIZE + 1 ] = { 0 } ;

   SDB_ASSERT( NULL != master, "Error: master node cannot be NULL" ) ;

   hasCs = ( 0 != ossStrncmp( "", csName, CI_CS_NAME_SIZE ) ) ;
   hasCollection = ( 0 != ossStrncmp( "", clName, CI_CL_NAME_SIZE ) ) ;
   if ( hasCollection )
   {
      ossSnprintf( fullName, sizeof( fullName ), "%s.%s", csName, clName ) ;
   }

   // get collections from master node
   rc = db.connect( master->_hostname, master->_serviceName,
                    g_username, g_password ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to connect to master node: "
                << master->_hostname << ":"
                << master->_serviceName ;
      if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
      {
         std::cout << "user: " << g_username
                   << "   password: " << g_password;
      }
      std::cout << std::endl ;
      goto error ;
   }

   rc = db.listCollections( cursor ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to list collections" << std::endl ;
      goto error ;
   }

   rc = cursor.next( collection ) ;
   while ( SDB_DMS_EOC != rc )
   {
      if ( SDB_OK != rc )
      {
         std::cout << "Waring: failed to get record in cursor" << std::endl ;
         if ( collections.count() > 0)
         {
            // inspect with collections already exist.
            goto done ;
         }
         else
         {
            goto error ;
         }
      }
      else
      {
         std::string cs ;
         std::string cl ;
         BOOLEAN csMatch     = FALSE ;
         BOOLEAN allMatch    = FALSE ;
         BOOLEAN inMainSubCl = FALSE ;
         std::string name    = collection.getField( "Name" ).String() ;
         std::size_t dot     = name.find( '.' ) ;
         if ( std::string::npos == dot )
         {
            std::cout << "Error: cannot split collection fullname: "
                      << name << std::endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         cs = name.substr( 0, dot ) ;
         cl = name.substr( dot + 1 ) ;
         // no cl name input and cs name match
         csMatch = ( !hasCollection &&
                     ( 0 == ossStrncmp( csName, cs.c_str(),
                                        CI_CS_NAME_SIZE ) ) ) ;
         allMatch = ( hasCs && hasCollection &&
                      ( 0 == ossStrncmp( csName, cs.c_str(),
                                         CI_CS_NAME_SIZE ) ) &&
                      ( 0 == ossStrncmp( clName, cl.c_str(),
                                         CI_CL_NAME_SIZE ) ) ) ;
         inMainSubCl = ( hasCs && hasCollection &&
                         isInMainSubCl( fullName, name.c_str(), mainCls ) ) ;
         if ( !hasCs || csMatch || allMatch || inMainSubCl )
         {
            ciCollection *cl = collections.createNode() ;
            if ( NULL == cl )
            {
               std::cout << "Error: failed to allocate ciCollection"
                  << std::endl ;
               rc = SDB_OOM ;
               goto error ;
            }
            ossStrncpy( cl->_clName, name.c_str(), CI_CL_FULLNAME_SIZE ) ;
            if ( inMainSubCl )
            {
               ossMemcpy( cl->_mainClName, fullName, CI_CL_FULLNAME_SIZE ) ;
            }
            else
            {
               const CHAR *mainClName = getMainClName( mainCls, name.c_str() ) ;
               if ( NULL != mainClName )
               {
                  ossMemcpy( cl->_mainClName,
                             mainClName, CI_CL_FULLNAME_SIZE ) ;
               }
            }
            collections.add( cl ) ;
         }
      }
      rc = cursor.next( collection ) ;
   }

   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_OK ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** check if reach end of condition
***/
BOOLEAN reachEnd( const ciBson &doc, const INT32 nodeCount )
{
   BOOLEAN end = TRUE ;
   for ( INT32 idx = 0 ; idx < nodeCount ; ++idx )
   {
      if ( !doc.objs[idx].isEmpty() )
      {
         end = FALSE ;
         break;
      }
   }

   return end ;
}

// Objects(actually records in the collection) with the same fields and values
// are equal even their field orders are different.
BOOLEAN _objSortCmp( const BSONObj &left, const BSONObj &right )
{
   BOOLEAN equal = FALSE ;

   try
   {
      BSONObjIteratorSorted itrLeft( left ) ;
      BSONObjIteratorSorted itrRight( right ) ;

      while ( itrLeft.more() && itrRight.more() )
      {
         if ( itrLeft.next() == itrRight.next() )
         {
            continue ;
         }
         else
         {
            equal = FALSE ;
            goto done ;
         }
      }

      // Any one has more elements, they do not equal.
      equal = ( itrLeft.more() || itrRight.more() ) ? FALSE : TRUE ;
   }
   catch ( std::exception &e )
   {
      std::cerr << "Unexpected exception: " << e.what() << std::endl ;
   }
done:
   return equal ;
}

/**
** compare each record among nodes
***/
BOOLEAN compare( ciLinkList< ciNode > &nodes,
                 const bson::BSONObj &obj,
                 ciState &state, ciBson &doc )
{
   BOOLEAN equal = FALSE ;
   INT32 nodeCount = nodes.count() ;
   nodes.resetCurrentNode() ;
   ciNode *node = nodes.getHead() ;
   state.reset() ;

   for ( INT32 idx = 0 ; idx < nodeCount ; ++idx, node = nodes.next() )
   {
      // the collection exists but is not available in node,
      // we assume the record exists
      if ( ciNode::STATE_NORMAL != node->_state &&
           ciNode::STATE_CLNOTEXIST != node->_state )
      {
         state.set( idx ) ;
      }
      else if ( ciNode::STATE_NORMAL == node->_state )
      {
         if ( doc.objs[idx].equal( obj ) || _objSortCmp( doc.objs[idx], obj ) )
         {
            state.set( idx ) ;
         }
      }
   }

   if ( state._state == ( ( 1 << nodeCount ) - 1 ) || 0 == state._state )
   {
      state.reset() ;
      equal = TRUE ;
      // for next round
      state.set( ALL_THE_SAME_BIT ) ;
   }

   return equal ;
}

/**
** compare each record among nodes
***/
INT32 getCiRecord( ciLinkList< ciNode > &nodes,
                   ciLinkList< ciCursor > &cursors,
                   ciLinkList< ciRecord > &records )
{
   INT32 rc        = SDB_OK ;
   BOOLEAN equal   = FALSE ;
   INT32 min       = 0 ;
   INT32 nodeCount = 0 ;
   ciBson record ;
   ciState state ;
   state.reset() ;
   cursors.resetCurrentNode() ;

   nodeCount = cursors.count() ;
   // get first record and compare
   state.set( ALL_THE_SAME_BIT ) ;
   rc = getNext( state, cursors, 0, record, FALSE ) ;
   while ( !reachEnd( record, nodeCount ) )
   {
      min = getMinObjectIndex( record, nodeCount ) ;
      equal = compare( nodes, record.objs[ min ], state, record ) ;

      if ( !equal )
      {
         ciRecord *rd = records.createNode() ;
         if ( NULL == rd )
         {
            std::cout << "Error: failed to allocate ciRecord"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         rd->_bson = record.objs[min].copy() ;
         rd->_state = state._state ;
         rd->_len = rd->_bson.objsize() ;
         records.add( rd ) ;
      }
      rc = getNext( state, cursors, min, record ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** make tmpFile path from outFile
***/
void makeTmpFileName( const CHAR *outFile, UINT32 loopIndex, CHAR *tmpFile, UINT32 len )
{
   SDB_ASSERT( NULL != outFile && NULL != tmpFile, "outFile & tmpFile can't be NULL" ) ;

   ossMemset( tmpFile, 0, OSS_MAX_PATHSIZE ) ;
   ossSnprintf( tmpFile, len, "%s" CI_TMP_FILE_SUFFIX, outFile, loopIndex ) ;
}

/**
** inspect node without file specified
***/
INT32 inspectWithoutFile( sdbclient::sdb *coord, ciHeader *header,
                          const CHAR *outFile, UINT64 &count )
{
   INT32 rc                                 = SDB_OK ;
   BOOLEAN hasGroup                         = FALSE ;
   BOOLEAN opened                           = FALSE ;
   ciGroup *curGroup                        = NULL ;
   ciCollection *curCollection              = NULL ;
   CHAR *buffer                             = NULL ;
   INT64 offset                             = 0 ;
   INT64 bufferSize                         = 0 ;
   INT64 validSize                          = 0 ;
   CHAR fullName[ CI_CL_FULLNAME_SIZE + 1 ] = { 0 } ;
   ciLinkList< ciGroup > groupList ;
   ciLinkList< ciNode > nodeList ;
   ciLinkList< ciCollection > collections ;
   ciLinkList< ciCursor > cursors ;
   ciLinkList< ciRecord > records ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciTail tail ;
   OSSFILE file ;
   ossTimestamp beginTime ;
   ossTimestamp endTime ;

   ossGetCurrentTime( beginTime ) ;

   count = 0 ;
   rc = ossOpen( outFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, file ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file, rc = " << rc << std::endl ;
      goto error ;
   }
   opened = TRUE ;

   // write header to file
   rc = writeCiHeader( file, header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   offset += validSize ;

   rc = getCiGroup( coord, header->_groupName, groupList ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = getMainAndSubCl( coord, tail._mainCls ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tail._mainClCount = tail._mainCls.size() ;

   hasGroup = ( 0 != ossStrncmp( "", header->_groupName,
                CI_GROUPNAME_SIZE ) ) ;
   // combine collection full name
   ossSnprintf( fullName, sizeof( fullName ), "%s.%s",
                header->_csName, header->_clName ) ;

   groupList.resetCurrentNode() ;
   curGroup = groupList.getHead() ;
   while( NULL != curGroup )
   {
      if ( !hasGroup || 0 == ossStrncmp( curGroup->_groupName,
                                         header->_groupName,
                                         CI_GROUPNAME_SIZE ) )
      {
         nodeList.clear() ;
         collections.clear() ;

         rc = getCiNode( coord, curGroup, groupHeader, nodeList ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         // get collections
         rc = getCiCollection( nodeList.getHead(), header->_csName,
                               header->_clName, collections, tail._mainCls ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         groupHeader._clCount = collections.count() ;
         tail._clCount += collections.count() ;

         // remember the offset
         ciOffset *off = tail._groupOffset.createNode() ;
         if ( NULL == off )
         {
            std::cout << "Error: failed to allocate ciClOffset"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         off->_offset = offset ;
         tail._groupOffset.add( off ) ;
         ++tail._groupCount ;

         // write group header to file
         rc = writeCiGroupHeader( file, &groupHeader ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         offset += CI_GROUP_HEADER_SIZE ;

         curCollection = collections.getHead() ;

         // write the global nodes info
         rc = writeCiNode( file, nodeList, buffer, bufferSize, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         offset += validSize ;

         while ( NULL != curCollection )
         {
            cursors.clear() ;
            bson::BSONObj order = bob().append("_id", 1).obj();
            rc = getCiCursor( nodeList, curCollection->_clName,
                              cursors, order, TRUE ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            ossMemset( clHeader._fullname, 0, CI_CL_FULLNAME_SIZE ) ;
            ossMemcpy( clHeader._fullname, curCollection->_clName,
                       CI_CL_FULLNAME_SIZE ) ;
            ossMemcpy( clHeader._mainClName, curCollection->_mainClName,
                       CI_CL_FULLNAME_SIZE ) ;

            records.clear() ;
            rc = getCiRecord( nodeList, cursors, records ) ;
            if ( SDB_OK != rc )
            {
               std::cout << "Error: failed to re record among nodes"
                         << std::endl ;
               goto error ;
            }

            clHeader._recordCount = records.count() ;
            tail._recordCount += records.count() ;
            if ( !normalNodes( nodeList ) || records.count() > 0 )
            {
               tail._diffCLCount++ ;
            }

            // 1. write collection header
            rc = writeCiClHeader( file, &clHeader ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            offset += CI_CL_HEADER_SIZE ;

            // 2. write nodes info
            // write the nodes info from perspective of each collection
            rc = writeCiNode( file, nodeList, buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            offset += validSize ;

            // 3. write diff-records info
            if ( clHeader._recordCount > 0 )
            {
               count += clHeader._recordCount ;
               rc = writeCiRecord( file, records, buffer,
                                   bufferSize, validSize ) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
               offset += validSize ;
            }

            curCollection = collections.next() ;
         }
      }

      curGroup = groupList.next() ;
   }

   {
      //BOOLEAN hasCS = ( 0 != ossStrncmp( "", header->_csName,
      //                                       CI_CS_NAME_SIZE ) ) ;
      BOOLEAN hasCL = ( 0 != ossStrncmp( "", header->_clName,
         CI_CL_NAME_SIZE ) ) ;
      if ( 0 >= tail._clCount )
      {
         std::cout << "Not found any "
                   << ( hasCL ? "match " : "" )
                   << "collection" << std::endl ;
         rc = CI_INSPECT_CL_NOT_FOUND ;
         goto done ;
      }
   }

   ossGetCurrentTime( endTime ) ;
   {
      UINT64 begin = beginTime.time * 1000000 + beginTime.microtm ;
      UINT64 end   = endTime.time * 1000000 + endTime.microtm ;
      tail._timeCount += ( end - begin ) / 1000 ;
   }

   if ( count == 0 )
   {
      tail._exitCode = 0 ; // no records
   }
   else
   {
      tail._exitCode = 2 ;// loop count over
   }

   // append tail
   rc = writeCiTail( file, &tail, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   //offset += validSize ;

   // remember the tail size
   header->_tailSize = validSize ;
   // update file header to file
   rc = writeCiHeader( file, header, buffer, bufferSize, validSize, TRUE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   // close file
   if ( opened )
   {
      ossClose( file ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** inspect node with file specified
***/
INT32 inspectWithFile( ciHeader *header, const CHAR *inFile,
                       const CHAR *outFile, UINT64 &count, BOOLEAN &finish )
{
   INT32 rc           = SDB_OK ;
   BOOLEAN inOpened   = FALSE ;
   BOOLEAN outOpened  = FALSE ;
   CHAR *buffer       = NULL ;
   INT64 bufferSize   = 0 ;
   INT64 validSize    = 0 ;
   UINT64 totalRecord = 0 ;
   INT64 fileSize     = 0 ;
   INT64 offset       = 0 ;
   INT64 tailOffset   = 0 ;
   INT64 writeOffset  = 0 ;
   ciLinkList< ciNode > ciNodes ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciHeader tmpHeader;
   ciTail tail ;
   OSSFILE in ;
   OSSFILE out ;
   ossTimestamp beginTime ;
   ossTimestamp endTime ;

   ossGetCurrentTime( beginTime ) ;

   // open in file
   rc = ossOpen( inFile, OSS_RO, OSS_RU | OSS_RG, in ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << inFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   inOpened = TRUE ;
   // open out file
   rc = ossOpen( outFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, out ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << outFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   outOpened = TRUE ;

   rc = ossGetFileSize( &in, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to get file size" << std::endl ;
      goto error ;
   }

   if ( fileSize < CI_HEADER_SIZE )
   {
      std::cout << "Error: filesize is less than " << CI_HEADER_SIZE
                << std::endl ;
      goto error ;
   }

   rc = readCiHeader( in, &tmpHeader ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   tailOffset = fileSize - tmpHeader._tailSize ;
   rc = readCiTail( in, tailOffset, &tail ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tail._groupCount = 0 ;
   tail._groupOffset.clear() ;
   tail._recordCount = 0 ;
   tail._diffCLCount = 0 ;

   rc = writeCiHeader( out, header, buffer, bufferSize, validSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to write header to file" << std::endl ;
      goto error ;
   }
   writeOffset = CI_HEADER_SIZE ;

   //skip 65536 bytes
   offset = CI_HEADER_SIZE ;
   while ( offset < tailOffset )
   {
      rc = readCiGroupHeader( in, offset, &groupHeader ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // remember the offset
      ciOffset *off = tail._groupOffset.createNode() ;
      if ( NULL == off )
      {
         std::cout << "Error: failed to allocate ciOffset"
                   << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      off->_offset = writeOffset ;
      tail._groupOffset.add( off ) ;
      ++tail._groupCount ;

      // write to out file
      rc = writeCiGroupHeader( out, &groupHeader ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to write ciGroupHeader to file"
                   << std::endl ;
         goto error ;
      }

      writeOffset += CI_GROUP_HEADER_SIZE ;
      // read nodes
      ciNodes.clear() ;
      rc = readCiNode( in, offset, groupHeader, ciNodes ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // write the global nodes info
      rc = writeCiNode( out, ciNodes, buffer, bufferSize, validSize ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to write ciNodes to file" << std::endl ;
         goto error ;
      }
      writeOffset += validSize ;

      UINT32 idx = 0 ;
      while ( idx < groupHeader._clCount )
      {
         ciClHeader clHeader ;
         ciLinkList< ciRecord > records ;
         ciLinkList< ciNode > nodesForEachCL ;

         // 1. read collection header
         rc = readCiClHeader( in, offset, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciClHeader" << std::endl ;
            goto error ;
         }

         // 2. read nodes-info for each collection
         rc = readCiNode( in, offset, groupHeader, nodesForEachCL ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciNode" << std::endl ;
            goto error ;
         }

         // 3. read diff-records info
         if ( clHeader._recordCount > 0 )
         {
            records.clear() ;

            rc = readCiRecord( in, offset, ciNodes, clHeader, records ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         }

         // 4. count the final result
         clHeader._recordCount = records.count() ;
         tail._recordCount += records.count() ;
         if ( !normalNodes( ciNodes ) || records.count() > 0 )
         {
            tail._diffCLCount++ ;
         }

         // 5. write collection header
         rc = writeCiClHeader( out, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to write ciClHeader to file"
                      << std::endl ;
            goto error ;
         }
         writeOffset += CI_CL_HEADER_SIZE ;

         // 6. write nodes-info for each collection
         rc = writeCiNode( out, ciNodes, buffer, bufferSize, validSize ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to write ciNodes to file" << std::endl ;
            goto error ;
         }
         writeOffset += validSize ;

         // 7. write diff-records info
         if ( records.count() > 0 )
         {
            totalRecord += records.count() ;

            rc = writeCiRecord( out, records, buffer,
                                bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            writeOffset += validSize ;
         }
         ++idx ;
      }
   }

   ossGetCurrentTime( endTime ) ;
   {
      UINT64 begin = beginTime.time * 1000000 + beginTime.microtm ;
      UINT64 end   = endTime.time * 1000000 + endTime.microtm ;
      tail._timeCount += ( end - begin ) / 1000 ;
   }

   // tail
   if ( totalRecord == 0 )
   {
      finish = TRUE ;
      tail._exitCode = 0 ; // no records
   }
   else
   {
      double precent = ( totalRecord / (double)count ) * 100 ;
      if ( precent <= 1 )
      {
         finish = TRUE ;
         tail._exitCode = 1 ; // lt 1%
      }
   }

   if ( !finish )
   {
      count = totalRecord ;
      tail._exitCode = 2 ; // assume loop it over
   }

   // write header to file
   rc = writeCiTail( out, &tail, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // remember the tail size
   header->_tailSize = validSize ;
   // update file header to file
   rc = writeCiHeader( out, header, buffer, bufferSize, validSize, TRUE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   if ( inOpened )
   {
      ossClose( in ) ;
   }

   if ( outOpened )
   {
      ossClose( out ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

const CHAR *_ciNode::stateDesc[ _ciNode::STATE_COUNT ] =
{
   "Normal",
   "Failed to connect to node",
   "Collection does not exist",
   "Failed to get the collection",
   "Failed to get cursor"
} ;

_sdbCi::_sdbCi(): _cipher( FALSE )
{
   ossMemset( _coordAddr, 0, CI_ADDRESS_SIZE + 1 ) ;
   ossMemset( _auth, 0, CI_AUTH_SIZE + 1 ) ;
   ossMemset( _token, 0, CI_TOKEN_SIZE + 1 ) ;
   ossMemset( _cipherfile, 0, CI_CIPHERFILE_SIZE + 1 ) ;
}

_sdbCi::~_sdbCi()
{
}

void _sdbCi::displayArgs( const po::options_description &desc )
{
   std::cout << desc << std::endl ;
}

INT32 _sdbCi::init( INT32 argc, CHAR **argv,
                    po::options_description &desc,
                    po::variables_map &vm )
{
   INT32 rc = SDB_OK ;

   INSPECT_ADD_OPTIONS_BEGIN( desc )
      INSPECT_OPTIONS
   INSPECT_ADD_OPTIONS_END

   rc = utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Invalid parameters, rc: " << rc << std::endl ;
      displayArgs( desc ) ;
      goto error ;
   }

   rc = _pmdCfgRecord::init( NULL, &vm ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Invalid parameters" << std::endl ;
      displayArgs( desc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbCi::handle( const po::options_description &desc,
                      const po::variables_map &vm )
{
   INT32 rc = SDB_OK ;
   BOOLEAN byGroup = TRUE ;
   BOOLEAN useOutput = FALSE ;
   CHAR outReport[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   CHAR *tailBuffer = NULL ;
   INT64 tailBufferSize = 0 ;
   OSSFILE startupFile ;
   BOOLEAN startupFileOpened = FALSE ;
   BOOLEAN startupFileLocked = FALSE ;

   if ( vm.count( CONSISTENCY_INSPECT_HELP ) )
   {
      std::cout << "This tool is used to inspect data among nodes in each "
                << "group. It will scan all records."
                << std::endl << std::endl ;

      displayArgs(desc) ;
      goto done ;
   }

   if ( vm.count( CONSISTENCY_INSPECT_VER ) )
   {
      ossPrintVersion( "SequoiaDB version" ) ;

      CHAR version[ 64 ] = { 0 } ;
      std::cout << "sdbConsistencyInspect version : " ;
      ossSnprintf( version, 64, "%d.%d", CI_MAIN_VERSION, CI_SUB_VERSION ) ;
      std::cout << version << std::endl ;
      goto done ;
   }

   if( vm.count( CONSISTENCY_INSPECT_AUTH ) )
   {
      if ( NULL == ossStrrchr( _auth, ':' ) )
      {
         if( !_cipher && ( vm.count( CONSISTENCY_INSPECT_TOKEN ) ||
                           vm.count( CONSISTENCY_INSPECT_CIPHERFILE ) ) )
         {
            std::cout << "If you want to use cipher text, you should use"
                      << " \"--cipher true\"" << std::endl ;
         }
      }
   }

   if ( 0 != ossStrncmp( CI_ACTION_INSPECT, _header._action, CI_ACTION_SIZE ) &&
        0 != ossStrncmp( CI_ACTION_REPORT, _header._action, CI_ACTION_SIZE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "Unknown action : " << _header._action << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else if ( 0 == ossStrncmp( CI_ACTION_REPORT,
                              _header._action, CI_ACTION_SIZE ) &&
             !vm.count( CONSISTENCY_INSPECT_FILE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "a existed file need to be specified "
                   "when ACTION is \"report\"" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if (    vm.count( CONSISTENCY_INSPECT_CL )
       && !vm.count( CONSISTENCY_INSPECT_CS ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "collection space name should be specified when collection "
                   "name is specified" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 0 != ossStrncmp( CI_VIEW_CL, _header._view, CI_VIEWOPTION_SIZE ) &&
        0 != ossStrncmp( CI_VIEW_GROUP, _header._view, CI_VIEWOPTION_SIZE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "Unknown action : " << _header._action << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   byGroup = ( 0 != ossStrncmp( CI_VIEW_CL, _header._view,
                                CI_VIEWOPTION_SIZE ) ) ? TRUE : FALSE ;
   useOutput = vm.count( CONSISTENCY_INSPECT_OUTPUT ) ? TRUE : FALSE ;

   if ( 0 == ossStrncmp( CI_ACTION_REPORT,
                         _header._action, CI_ACTION_SIZE ) &&
        vm.count( CONSISTENCY_INSPECT_FILE ) )
   {
      // report file
      if ( !useOutput )
      {
         ossSnprintf( outReport, OSS_MAX_PATHSIZE, "%s%s",
                      _header._filepath, CI_FILE_REPORT ) ;
      }
      else
      {
         ossSnprintf( outReport, OSS_MAX_PATHSIZE, "%s%s",
                      _header._outfile, CI_FILE_REPORT ) ;
      }
      rc = byGroup ? report ( _header._filepath, outReport,
                              tailBuffer, tailBufferSize )
                   : report2( _header._filepath, outReport,
                              tailBuffer, tailBufferSize ) ;
      //rc = report2( _header._filepath ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      std::cout << _header._action << " done" << std::endl ;
      std::cout << tailBuffer << std::endl ;

      goto done ;
   }

   if ( vm.count( CONSISTENCY_INSPECT_FILE ) &&
        0 == ossStrncmp( CI_ACTION_INSPECT, _header._action, CI_ACTION_SIZE ) )
   {
      std::cout << "file is specified, initialize all options according to file"
                << std::endl ;
      rc = initialize( &_header ) ;
   }
   else
   {
      rc = splitAddr() ;
   }
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = splitAuth() ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // in one dir, sdbinspect can be started only once
   rc = ossOpen( CI_START_TMP_FILE, OSS_CREATE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, startupFile ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open startup-file: " CI_START_TMP_FILE
                << ", rc = " << rc << std::endl ;
      goto error ;
   }

   startupFileOpened = TRUE ;
   rc = ossLockFile( &startupFile, OSS_LOCK_EX ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: sdbinspect can't be executed concurrently "
                   "in the same directory"
                << endl ;
      goto error ;
   }
   startupFileLocked = TRUE ;

   rc = inspect() ;
   if ( CI_INSPECT_CL_NOT_FOUND == rc )
   {
      rc = SDB_OK ;
      goto done ;
   }

   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // report file
   ossSnprintf( outReport, OSS_MAX_PATHSIZE, "%s%s",
                _header._outfile, CI_FILE_REPORT ) ;

   rc = byGroup ? report ( _header._outfile, outReport,
                           tailBuffer, tailBufferSize )
                : report2( _header._outfile, outReport,
                           tailBuffer, tailBufferSize ) ;

   CHECK_VALUE( (SDB_OK != rc ), error ) ;
   std::cout << _header._action << " done" << std::endl ;
   std::cout << tailBuffer << std::endl ;

done:
   if ( startupFileLocked )
   {
      // if we have locked, we have all priority of this file.
      // and we should delete it
      ossLockFile( &startupFile, OSS_LOCK_UN ) ;
      ossClose( startupFile ) ;
      ossDelete( CI_START_TMP_FILE ) ;
   }
   else if ( startupFileOpened )
   {
      // if we have opened, but lock failed.
      // we do not have priority of this file. just close the file.
      ossClose( startupFile ) ;
   }

   if ( NULL != tailBuffer )
   {
      SDB_OSS_FREE( tailBuffer ) ;
      tailBuffer = NULL ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _sdbCi::inspect()
{
   INT32 rc           = SDB_OK ;
   INT32 curLoop      = 0 ;
   UINT64 totalRecord = 0 ;
   BOOLEAN finish     = FALSE ;
   sdbclient::sdb *coord = NULL ;
   CHAR inFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   CHAR tmpFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 }  ;

   coord = new sdbclient::sdb() ;
   if( NULL == coord )
   {
      std::cout << "Error: failed to allocate sdbclient::sdb" << std::endl ;
      rc = SDB_OOM ;
      goto error ;
   }

   rc = coord->connect( _header._coordAddr, _header._serviceName,
                        g_username, g_password ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to connect to " << _header._coordAddr
                << ":" << _header._serviceName << ", rc: "  << rc << std::endl ;
      goto error ;
   }

   if ( 0 == ossStrncmp( _header._filepath, "", OSS_MAX_PATHSIZE ) )
   {
      do
      {
         curLoop += 1 ;
         makeTmpFileName( _header._outfile, curLoop, tmpFile,
                          OSS_MAX_PATHSIZE ) ;

         rc = inspectWithoutFile( coord, &_header, tmpFile, totalRecord ) ;
      }while ( CI_INSPECT_ERROR == rc ) ;

      if ( CI_INSPECT_CL_NOT_FOUND == rc )
      {
         goto done ;
      }

      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      if ( 0 == totalRecord )
      {
         finish = TRUE ;
      }

      if ( _header._loop > 1 )
      {
         // use out file as input file for next loop
         ossMemcpy( inFile, tmpFile, OSS_MAX_PATHSIZE ) ;
      }
   }
   else
   {
      ossMemcpy( inFile, _header._filepath, OSS_MAX_PATHSIZE ) ;
   }

   for (INT32 idx = curLoop ; idx < _header._loop && !finish ; ++idx)
   {
      makeTmpFileName( _header._outfile, idx + 1, tmpFile, OSS_MAX_PATHSIZE ) ;

      rc = inspectWithFile( &_header, inFile, tmpFile, totalRecord, finish ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // use out file as input file for next loop
      ossMemset( inFile, 0, OSS_MAX_PATHSIZE ) ;
      ossMemcpy( inFile, tmpFile, OSS_MAX_PATHSIZE ) ;
   }

   rc = ossRenamePath( tmpFile, _header._outfile ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to rename temp file to \""
                << _header._outfile << "\"" << std::endl ;
      goto error ;
   }

   // delete temp file
   for ( INT32 idx = 0 ; idx < _header._loop ; ++idx )
   {
      makeTmpFileName( _header._outfile, idx + 1, tmpFile, OSS_MAX_PATHSIZE ) ;

      ossDelete( tmpFile ) ;
   }

done:
   if ( NULL != coord )
   {
      delete coord ;
      coord = NULL ;
   }
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 _sdbCi::report ( const CHAR *inFile, const CHAR *reportFile,
                       CHAR *&tailBuffer, INT64 &tailBufferSize )
{
   INT32 rc              = SDB_OK ;
   BOOLEAN inOpened      = FALSE ;
   BOOLEAN outOpened     = FALSE ;
   recordBuffer *rBuffer = NULL ;
   CHAR *buffer          = NULL ;
   INT64 bufferSize      = 0 ;
   INT64 validSize       = 0 ;
   INT64 fileSize        = 0 ;
   INT64 offset          = 0 ;
   INT64 tailOffset      = 0 ;

   ciHeader header ;
   ciLinkList< ciOffset > Offset ;
   ciLinkList< ciNode > ciNodes ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciTail tail ;
   OSSFILE in ;
   OSSFILE out ;
   // open in file
   rc = ossOpen( inFile, OSS_RO, OSS_RU | OSS_RG, in ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << inFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   inOpened = TRUE ;

   rc = ossGetFileSize( &in, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to get file size" << std::endl ;
      goto error ;
   }

   if ( fileSize < CI_HEADER_SIZE )
   {
      std::cout << "Error: filesize is less than " << CI_HEADER_SIZE
                << std::endl ;
      goto error ;
   }

   rc = ossOpen( reportFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, out ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file, rc = " << rc << std::endl ;
      goto error ;
   }
   outOpened = TRUE ;
   rBuffer = new recordBuffer( out ) ;

   // dump header
   rc = readCiHeader( in, &header ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = dumpCiHeader( &header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // read tail
   tailOffset = fileSize - header._tailSize ;
   rc = readCiTail( in, tailOffset, &tail ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   //skip 65536 bytes
   offset = CI_HEADER_SIZE ;
   while ( offset < tailOffset )
   {
      // dump group
      rc = readCiGroupHeader( in, offset, &groupHeader ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = dumpCiGroupHeader( &groupHeader, buffer, bufferSize, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = writeToFile( out, buffer, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // read global nodes info
      ciNodes.clear() ;
      rc = readCiNode( in, offset, groupHeader, ciNodes ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = dumpCiNode( ciNodes, buffer, bufferSize, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = writeToFile( out, buffer, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      UINT32 idx = 0 ;
      while ( idx < groupHeader._clCount )
      {
         ciClHeader clHeader ;
         ciLinkList< ciRecord > records ;
         ciLinkList< ciNode > nodesForCL ;

         rc = readCiClHeader( in, offset, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciClHeader" << std::endl ;
            goto error ;
         }
         rc = dumpCiClHeader( &clHeader, buffer, bufferSize, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         rc = writeToFile( out, buffer, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         rc = readCiNode( in, offset, groupHeader, nodesForCL ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciNode" << std::endl ;
            goto error ;
         }
         rc = dumpCiNodeSimple( nodesForCL, buffer, bufferSize, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         rc = writeToFile( out, buffer, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         if ( clHeader._recordCount > 0 )
         {
            records.clear() ;

            rc = readCiRecord( in, offset, ciNodes,
                               clHeader, records, TRUE ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = archiveCiRecord( ciNodes, records, rBuffer ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         }

         ++idx ;
      }
   }

   rc = dumpCiTail( tail, tailBuffer, tailBufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, tailBuffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tailBufferSize = validSize ;

done:
   if ( inOpened )
   {
      ossClose( in ) ;
   }

   if ( outOpened )
   {
      ossClose( out ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   if( NULL != rBuffer )
   {
      delete rBuffer ;
      rBuffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}



INT32 _sdbCi::report2( const CHAR *inFile, const CHAR *reportFile,
                       CHAR *&tailBuffer, INT64 &tailBufferSize )
{
   INT32 rc               = SDB_OK ;
   BOOLEAN inOpened       = FALSE ;
   BOOLEAN outOpened      = FALSE ;
   ciOffset *groupOffset  = NULL ;
   recordBuffer *rBuffer  = NULL ;
   CHAR *buffer           = NULL ;
   INT64 bufferSize       = 0 ;
   INT64 validSize        = 0 ;
   INT64 fileSize         = 0 ;
   INT64 tailOffset       = 0 ;

   ciHeader header ;
   ciLinkList< ciNode > ciNodes ;
   ciLinkList< ciOffset > clOffsets ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciTail tail ;
   OSSFILE in ;
   OSSFILE out ;
   // open in file
   rc = ossOpen( inFile, OSS_RO, OSS_RU | OSS_WU | OSS_RG, in ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << inFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   inOpened = TRUE ;

   rc = ossGetFileSize( &in, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to get file size" << std::endl ;
      goto error ;
   }

   if ( fileSize < CI_HEADER_SIZE )
   {
      std::cout << "Error: filesize is less than " << CI_HEADER_SIZE
                << std::endl ;
      goto error ;
   }

   rc = ossOpen( reportFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, out ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file, rc = " << rc << std::endl ;
      goto error ;
   }
   outOpened = TRUE ;
   rBuffer = new recordBuffer( out ) ;

   // dump header
   rc = readCiHeader( in, &header ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = dumpCiHeader( &header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // read tail
   tailOffset = fileSize - header._tailSize ;
   rc = readCiTail( in, tailOffset, &tail ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   tail._groupOffset.resetCurrentNode() ;
   groupOffset = tail._groupOffset.getHead() ;
   while ( NULL != groupOffset )
   {
      rc = dumpOneCl( in, out, groupOffset, clOffsets, NULL,
                      buffer, bufferSize, validSize, rBuffer ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      groupOffset = tail._groupOffset.next() ;
   }

   rc = dumpCiTail( tail, tailBuffer, tailBufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, tailBuffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tailBufferSize = validSize ;

done:
   if ( inOpened )
   {
      ossClose( in ) ;
   }

   if ( outOpened )
   {
      ossClose( out ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   if( NULL != rBuffer )
   {
      delete rBuffer ;
      rBuffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 _sdbCi::doDataExchange( engine::pmdCfgExchange *pEx )
{
   resetResult() ;

   rdxString( pEx, CONSISTENCY_INSPECT_COORD, _coordAddr, CI_ADDRESS_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, CI_COORD_DEFVAL, FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_AUTH, _auth, CI_AUTH_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, "\"\":\"\"", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_TOKEN, _token, CI_TOKEN_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", FALSE ) ;

   rdxBooleanS( pEx, CONSISTENCY_INSPECT_CIPHER, _cipher,
                FALSE, PMD_CFG_CHANGE_FORBIDDEN, FALSE, FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_CIPHERFILE, _cipherfile,
              CI_CIPHERFILE_SIZE , FALSE, PMD_CFG_CHANGE_FORBIDDEN,
              "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_ACTION, _header._action, CI_ACTION_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, CI_ACTION_INSPECT, FALSE ) ;

   rdxInt( pEx, CONSISTENCY_INSPECT_LOOP, _header._loop, FALSE,
           PMD_CFG_CHANGE_RUN, 5 ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_GROUP, _header._groupName,
              CI_GROUPNAME_SIZE, FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_CS, _header._csName, CI_CS_NAME_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_CL, _header._clName, CI_CL_NAME_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_FILE, _header._filepath,
              OSS_MAX_PATHSIZE, FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_OUTPUT, _header._outfile,
              OSS_MAX_PATHSIZE, FALSE, PMD_CFG_CHANGE_FORBIDDEN,
              CI_FILE_NAME ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_VIEW, _header._view, CI_VIEWOPTION_SIZE,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, CI_VIEW_GROUP, FALSE ) ;

   return getResult() ;
}

INT32 _sdbCi::postLoaded( PMD_CFG_STEP step )
{
   if ( _header._outfile[ 0 ] == '\0' )
   {
      ossStrcpy( _header._outfile, CI_FILE_NAME ) ;
   }
   return SDB_OK ;
}

INT32 _sdbCi::preSaving()
{
   return SDB_OK ;
}

INT32 _sdbCi::splitAddr()
{
   INT32 rc        = SDB_OK ;
   INT32 length    = ossStrlen( _coordAddr ) ;
   CHAR *begin     = _coordAddr ;
   CHAR *end       = begin + length ;
   const CHAR *pch = NULL ;

   if ( begin == end )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " Hostname and servicename of coord must be specified"
                << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pch = ossStrrchr( _coordAddr, ':' ) ;
   if ( begin == pch )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " Hostname of coord must be specified" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == pch || end == pch + 1 )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " Service Name must be specified after the hostname, "
                   " split by \":\"" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // initialize hostname and servicename in _header
   ossMemcpy( _header._coordAddr, _coordAddr, pch - begin ) ;
   ossMemcpy( _header._serviceName, pch + 1, end - pch ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbCi::splitAuth()
{
   INT32 rc        = SDB_OK ;
   INT32 length    = ossStrlen( _auth ) ;
   CHAR *begin     = _auth ;
   CHAR *end       = begin + length ;
   const CHAR *pch = NULL ;

   if ( begin == end )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " username and password of sequoiadb is NULL"
                << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pch = ossStrrchr( _auth, ':' ) ;

   // assume the user has only specified 'user' as in the case of cipherfile
   if ( NULL == pch )
   {
      utilPasswordTool passwdTool ;
      std::string user = _auth ;
      std::string connectionUserName ;
      std::string passwd ;
      std::string filePath = _cipherfile ;

      if ( _cipher )
      {
         rc = passwdTool.getPasswdByCipherFile( user, _token,
                                                filePath,
                                                passwd ) ;
         if ( SDB_OK != rc )
         {
            std::cerr << "Failed to get user[" << user.c_str()
                      << "] password from cipher file"
                      << "[" << filePath.c_str() << "], rc: " << rc
                      << std::endl ;
            goto error ;
         }

         if ( filePath.empty() || filePath.length() > CI_CIPHERFILE_SIZE )
         {
            rc = SDB_INVALIDARG ;
            std::cerr << "Invalid cipher file path[" << filePath.c_str()
                      << "]. Its maximum length is " <<  CI_CIPHERFILE_SIZE
                      << ", rc: " << rc << std::endl ;
            goto error ;
         }
         ossStrncpy ( _cipherfile, filePath.c_str(), filePath.length() ) ;
         connectionUserName = utilGetUserShortNameFromUserFullName( user ) ;
      }
      else
      {
         BOOLEAN isNormalInput = FALSE ;

         connectionUserName = _auth ;
         if ( 0 != ossStrlen( _token ) || 0 != ossStrlen( _cipherfile ) )
         {
            passwd = "" ;
            isNormalInput = TRUE ;
         }
         else
         {
            // if we execute Ctrl + c while entering the password,
            // interactivePasswdInput function will return false.
            isNormalInput = utilPasswordTool::interactivePasswdInput( passwd ) ;
         }

         if ( !isNormalInput )
         {
            rc = SDB_APP_INTERRUPT ;
            std::cerr << getErrDesp( rc ) << ", rc: " << rc << std::endl ;
            goto error ;
         }
      }
      ossStrcpy( g_username, connectionUserName.c_str() ) ;
      ossStrcpy( g_password, passwd.c_str() ) ;
   }
   else
   {
      // initialize hostname and servicename in _header
      ossMemcpy( g_username, _auth, pch - begin ) ;
      ossMemcpy( g_password, pch + 1, end - pch ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

//////////////////////////////////////////////////////////////////////////
///< main function
INT32 main(INT32 argc, CHAR** argv)
{
   INT32 rc  = SDB_OK ;
   sdbCi *ci = NULL ;
   po::options_description desc( "Command options" ) ;
   po::variables_map vm ;

   ci = SDB_OSS_NEW sdbCi() ;
   if ( NULL == ci )
   {
      std::cout << "Error: failed to allocate sdbCi" << std::endl ;
      rc = SDB_OOM ;
      goto done ;
   }

   rc = ci->init( argc, argv, desc, vm ) ;
   CHECK_VALUE( ( SDB_OK != rc ), done ) ;

   rc = ci->handle( desc, vm ) ;
   CHECK_VALUE( ( SDB_OK != rc ), done ) ;

done:
   if ( NULL != ci )
   {
      SDB_OSS_DEL ci ;
      ci = NULL ;
   }
   return rc ;
}
