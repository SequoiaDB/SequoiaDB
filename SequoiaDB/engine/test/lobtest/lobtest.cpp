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

   Source File Name = lobtest.cpp

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

#include "client.h"

#include <gtest/gtest.h>
#include <boost/thread.hpp>

using namespace std ;

void hexDump( const CHAR *src, UINT32 size, CHAR *dst )
{
   static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
   for ( UINT32 i = 0; i < size; ++i )
   {
      dst[i*2] = hex[( src[i] & 0xf0 ) >> 4];
      dst[2*i + 1] = hex[ src[i] & 0x0f ];
   }
   dst[size * 2] = '\0' ;
}


void insert_1()
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   const UINT32 putNum = 1000 ;
   bson_oid_t oids[putNum] ;
   const UINT32 bufSize = 1231 ;
   CHAR buf[bufSize] = { 0 } ;

   for ( UINT32 i = 0 ; i < putNum ; ++i )
   {
      bson_oid_gen( &oid ) ;
      rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      oids[i] = oid ;
      memset( buf, 'a' + i, bufSize ) ;
      rc = sdbWriteLob( lob, buf, bufSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   cout << "write done" << endl ;

   CHAR buf2[bufSize] = { 0 } ;
   for ( UINT32 i = 0 ; i < putNum ; ++i )
   {
      CHAR tmp[25] ;
      bson_oid_to_string( &( oids[i]), tmp ) ;

      rc = sdbOpenLob( cl, &( oids[i] ), SDB_LOB_READ, &lob ) ;
      if ( SDB_OK != rc )
      {
         cout << tmp << " " << rc << endl ;
         ASSERT_TRUE( FALSE ) ;
      }
      SINT64 lobSize = 0 ;
      rc = sdbGetLobSize( lob, &lobSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      if ( bufSize != lobSize )
      {
         cout << tmp << " " << lobSize << endl ;
         ASSERT_TRUE( FALSE ) ;
      }
      UINT32 readSize = 0 ;
      rc = sdbReadLob( lob, bufSize, buf, &readSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( readSize, bufSize ) ;
      memset( buf2, 'a' + i, bufSize ) ;
      if ( 0 != memcmp(buf, buf2, bufSize ) )
      {
         cout << tmp << endl ;
         ASSERT_TRUE( FALSE ) ;
      }
      rc = sdbReadLob( lob, bufSize, buf, &readSize ) ;
      ASSERT_EQ( SDB_EOF, rc ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbRemoveLob( cl, &( oids[i] ) ) ;
      ASSERT_EQ( SDB_OK ,rc ) ;
   }
}

TEST(lobTest, insert_1)
{
   insert_1() ;
}

void insert_2()
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 putNum = 1000 ;
   const UINT32 bufSize = 1024 * 1024 * 10 + 1231 ;
   CHAR *buf = new CHAR[bufSize] ;
   CHAR *buf2 = new CHAR[bufSize] ;
   bson_oid_t oids[putNum] ;

   for ( UINT32 i = 0 ; i < putNum ; ++i )
   {
      for ( UINT32 j = 0 ; j < bufSize; ++j )
      {
         buf[j] = ( CHAR )rand() ;
      }

      bson_oid_gen( &oid ) ;
      rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      oids[i] = oid ;
      memset( buf, 'a' + i, bufSize ) ;
      SINT64 totalWriteLen = 0 ;
      while ( totalWriteLen < bufSize )
      {
         UINT32 writeLen = bufSize - totalWriteLen < 1452 ? bufSize - totalWriteLen : 1452 ;
         rc = sdbWriteLob( lob, buf + totalWriteLen, writeLen ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         totalWriteLen += writeLen ;
      }
      ASSERT_EQ( totalWriteLen, bufSize ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      SINT64 lobSize = 0 ;
      rc = sdbGetLobSize( lob, &lobSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( bufSize, lobSize ) ;
      SINT64 totalReadLen = 0 ;
      UINT32 readLen = 0 ;
      const UINT32 readSize = 456 ;
      while ( totalReadLen < lobSize )
      { 
         readLen = 0 ;
         rc = sdbReadLob( lob, readSize, buf2 + totalReadLen, &readLen ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         totalReadLen += readLen ;
      }
      ASSERT_EQ( 0, memcmp(buf, buf2, bufSize )) ;
      rc = sdbReadLob( lob, readSize, buf2, &readLen ) ;
      ASSERT_EQ( SDB_EOF, rc ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      CHAR oidstr[25] ;
      bson_oid_to_string( &oid, oidstr ) ;
      cout << "oid done:" << oidstr << endl ;
   }

   for ( UINT32 i = 0 ; i < putNum ; ++i )
   {
      rc = sdbRemoveLob( cl, &(oids[i])) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   delete []buf ;
   delete []buf2 ;
}

TEST(lobTest, insert_2)
{
   insert_2() ;
}

TEST(lobTest, seek_1)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 bufSize = 1024 * 1024 * 10 + 1924 ;
   CHAR *buf = new CHAR[bufSize] ;
   for ( UINT32 j = 0 ; j < bufSize; ++j )
   {
      buf[j] = ( CHAR )rand() ;
   }
   
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 totalWriteLen = 0 ;
   while ( totalWriteLen < bufSize )
   {
      UINT32 writeLen = bufSize - totalWriteLen < 1844 ? bufSize - totalWriteLen : 1844 ;
      rc = sdbWriteLob( lob, buf + totalWriteLen, writeLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      totalWriteLen += writeLen ;
   }
   ASSERT_EQ( totalWriteLen, bufSize ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 lobSize = 0 ;
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( bufSize, lobSize ) ;

   const UINT32 seekReadSize = 2000 ;

   CHAR buf2[seekReadSize] ;
   for ( UINT32 i = 0; i < 10000; ++i )
   {
      memset( buf2, 0, seekReadSize ) ;
      SINT64 seekSize = rand() % bufSize ;
      rc = sdbSeekLob( lob, seekSize, SDB_LOB_SEEK_SET ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      UINT32 readSize = bufSize - seekSize < seekReadSize ? bufSize - seekSize : seekReadSize ;
      readSize = rand() % readSize ;
      UINT32 read = 0 ;
      rc = sdbReadLob( lob, readSize, buf2, &read ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( readSize, read ) ;
      
      if ( 0 != memcmp( buf + seekSize, buf2, readSize ) )
      {

         CHAR dump[seekReadSize * 2 + 1];
         hexDump( buf2, readSize, dump) ;
         cout << "buf2:" << dump << endl ;
         hexDump( buf+ seekSize, readSize, dump );
         cout << "buf:" << dump << endl ;
         cout << "seek size:" << seekSize << endl ;

         ASSERT_TRUE( FALSE ) ;
      }
      cout << "seek times:" << i << ", seek size:" << seekSize << endl ;
   }

   UINT32 read = 0 ;
   rc = sdbSeekLob( lob, bufSize, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK,rc ) ;
   rc = sdbReadLob( lob, 1, buf2, &read ) ;
   ASSERT_EQ( SDB_EOF, rc ) ;
   rc = sdbSeekLob( lob, bufSize, SDB_LOB_SEEK_END ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const UINT32 lastReadSize = 1855 ;
   rc = sdbReadLob( lob, lastReadSize, buf2, &read ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( lastReadSize, read ) ;
   if ( 0 != memcmp( buf, buf2, lastReadSize ) )
   {
      CHAR dump[lastReadSize * 2 + 1];
      hexDump( buf2, lastReadSize, dump) ;
      cout << "buf2:" << dump << endl ;
      hexDump( buf, lastReadSize, dump );
      cout << "buf:" << dump << endl ;
      ASSERT_TRUE( FALSE ) ;
   }

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST(lobTest, seek_2)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 bufSize = 1024 * 1024 * 10 + 1924 ;
   CHAR *buf = new CHAR[bufSize] ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 totalWriteLen = 0 ;
   while ( totalWriteLen < bufSize )
   {
      UINT32 writeLen = bufSize - totalWriteLen < 1844 ? bufSize - totalWriteLen : 1844 ;
      rc = sdbWriteLob( lob, buf + totalWriteLen, writeLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      totalWriteLen += writeLen ;
   }
   ASSERT_EQ( totalWriteLen, bufSize ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   for ( UINT32 i = 0; i < 10000; ++i )
   {
      cout << "the " << i << "th time" << endl ;
      SINT64 seekSize = rand() % bufSize ;
      rc = sdbSeekLob( lob, seekSize, SDB_LOB_SEEK_SET ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      UINT32 readSize = bufSize ;
      UINT32 read = 0 ;
      rc = sdbReadLob( lob, readSize, buf, &read ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( bufSize - seekSize, read ) ;
   }

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST(lobTest, cache_1)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 bufSize = 1024 * 1024 * 10 + 1924 ;
   CHAR *buf = new CHAR[bufSize] ;
   for ( UINT32 j = 0 ; j < bufSize; ++j )
   {
      buf[j] = ( CHAR )rand() ;
   }

   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 totalWriteLen = 0 ;
   while ( totalWriteLen < bufSize )
   {
      UINT32 writeLen = bufSize - totalWriteLen < 1844 ? bufSize - totalWriteLen : 1844 ;
      rc = sdbWriteLob( lob, buf + totalWriteLen, writeLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      totalWriteLen += writeLen ;
   }
   ASSERT_EQ( totalWriteLen, bufSize ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   for ( UINT32 i = 0; i < 10; ++i )
   {
      CHAR readBuf = '\0' ;
      cout << "the " << i << "th time" << endl ;
      UINT32 read = 0 ;
      UINT32 totalRead = 0 ;

      rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      do
      {
         rc = sdbReadLob( lob, 1, &readBuf, &read ) ;
         if ( SDB_OK == rc )
         {
            ASSERT_EQ( 1, read ) ;
            ASSERT_EQ( readBuf, buf[totalRead] ) ;
            ++totalRead ;
         }
         else if ( SDB_EOF == rc )
         {
            break ;
         }
         else
         {
            ASSERT_EQ( SDB_OK , rc ) ;
         }
      } while ( TRUE ) ;

      ASSERT_EQ( totalRead, bufSize ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST(lobTest, cache_2)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 bufSize = 1024 * 1024 * 10 + 1924 ;
   CHAR *buf = new CHAR[bufSize] ;
   for ( UINT32 j = 0 ; j < bufSize; ++j )
   {
      buf[j] = ( CHAR )rand() ;
   }

   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 totalWriteLen = 0 ;
   while ( totalWriteLen < bufSize )
   {
      UINT32 writeLen = bufSize - totalWriteLen < 1844 ? bufSize - totalWriteLen : 1844 ;
      rc = sdbWriteLob( lob, buf + totalWriteLen, writeLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      totalWriteLen += writeLen ;
   }
   ASSERT_EQ( totalWriteLen, bufSize ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const UINT32 needRead = 1000 ;
   CHAR readBuf[needRead] ;

   for ( UINT32 i = 0; i < 10; ++i )
   {
      cout << i << endl ;
      UINT32 readLen = 0 ;
      rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      SINT64 seekSize = rand() % bufSize ;
      rc = sdbSeekLob( lob, seekSize, SDB_LOB_SEEK_SET ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbReadLob( lob, needRead, readBuf, &readLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_LE( readLen, needRead ) ;
      if ( 0 != memcmp( buf + seekSize, readBuf, readLen ))
      {
         ASSERT_TRUE( false ) ;
      }
      
      for ( UINT32 j = 0; j < 1000; ++j )
      {
         SINT64 newSeek = rand() % readLen ;
         rc = sdbSeekLob( lob, seekSize + newSeek, SDB_LOB_SEEK_SET ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         rc = sdbReadLob( lob, needRead, readBuf, &readLen ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         ASSERT_LE( readLen, needRead ) ;
         if ( 0 != memcmp( buf + seekSize + newSeek, readBuf, readLen ))
         {
            ASSERT_TRUE( false ) ;
         }
      }

      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

void remove_1()
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 putNum = 1000 ;
   bson_oid_t oids[putNum] ;
   const UINT32 bufSize = 1231 ;
   CHAR buf[bufSize] = { 0 } ;

   for ( UINT32 i = 0 ; i < putNum ; ++i )
   {
      bson_oid_gen( &oid ) ;
      rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      oids[i] = oid ;
      memset( buf, 'a' + i, bufSize ) ;
      rc = sdbWriteLob( lob, buf, bufSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK ,rc ) ;
   }

   cout << "write done" << endl ; 

   CHAR buf2[bufSize] = { 0 } ;
   for ( UINT32 i = 0 ; i < putNum ; ++i )
   {
      for ( UINT32 j = i ; j < putNum ; ++j )
      {
      CHAR tmp[25] ;
      bson_oid_to_string( &( oids[j]), tmp ) ;

      rc = sdbOpenLob( cl, &( oids[j] ), SDB_LOB_READ, &lob ) ;
      if ( SDB_OK != rc )
      {
         cout << i << "," << j << "," << rc << " " << tmp << endl  ;
         ASSERT_TRUE( FALSE ) ;
      }
      SINT64 lobSize = 0 ;
      rc = sdbGetLobSize( lob, &lobSize ) ;
      ASSERT_EQ( SDB_OK,  rc ) ;
      if ( bufSize != lobSize )
      {
         cout << i << "," << j << "," << tmp << endl  ;

         ASSERT_TRUE( FALSE ) ;
      }
      UINT32 readSize = 0 ;
      rc = sdbReadLob( lob, bufSize, buf, &readSize ) ;
      if ( SDB_OK != rc )
      {
         cout << i << "," << j << "," << tmp << endl  ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      ASSERT_EQ( readSize, bufSize ) ;
      memset( buf2, 'a' + j, bufSize ) ;
      if ( 0 != memcmp(buf, buf2, bufSize ) )
      {
         cout << tmp << endl ;
         ASSERT_TRUE( FALSE ) ;
      }
      rc = sdbReadLob( lob, bufSize, buf, &readSize ) ;
      ASSERT_EQ( SDB_EOF, rc ) ;
      rc = sdbCloseLob( &lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      }

      rc = sdbRemoveLob( cl, &( oids[i] ) ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      CHAR tmp[25] ;
      bson_oid_to_string( &( oids[i]), tmp ) ;
      cout << "remove:" << tmp << endl ;
   }
}

TEST(lobTest, remove_1)
{
   remove_1() ;
}

void multi_insert1()
{
   insert_1() ; 
}

TEST(lobTest, multi_1)
{
   const UINT32 threadNum = 5 ;
   boost::thread *ts[threadNum] ;
   for ( UINT32 i = 0; i < threadNum; i++ )
   {
      ts[i] = new boost::thread( multi_insert1 ) ;
   }

   for ( UINT32 i = 0; i < threadNum; i++ )
   {
      ts[i]->join() ;
      delete ts[i] ;
   }
}

void multi_insert2()
{
   insert_2() ;
}

TEST(lobTest, multi_2)
{
   const UINT32 threadNum = 5 ;
   boost::thread *ts[threadNum] ;
   for ( UINT32 i = 0; i < threadNum; i++ )
   {
      ts[i] = new boost::thread( multi_insert2 ) ;
   }

   for ( UINT32 i = 0; i < threadNum; i++ )
   {
      ts[i]->join() ;
      delete ts[i] ;
   }
}

TEST(lobTest, hugeLob)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ;

   rc = sdbConnect( "localhost", "11810", "", "", &conn ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetCollection( conn, "foo.bar", &cl ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   const UINT32 bufSize = 1024 * 1024 * 5 ;
   CHAR *buf = ( CHAR * )malloc(bufSize) ;
   ASSERT_TRUE( NULL != buf ) ;

   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   for ( UINT32 i = 0; i < 2048; ++i )
   {
      rc = sdbWriteLob( lob, buf, bufSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "write done" << endl ;

   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 lobSize = 0 ;
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   SINT64 expectSize = ( SINT64 )bufSize * 2048 ;
   ASSERT_EQ( expectSize, lobSize ) ;
   SINT64 totalReadLen = 0 ;
   UINT32 readLen = 0 ;
   const UINT32 readSize = bufSize ;
   while ( totalReadLen < lobSize )
   {
      readLen = 0 ;
      rc = sdbReadLob( lob, readSize, buf, &readLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      totalReadLen += readLen ;
   }
 
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   free( buf ) ;
}

