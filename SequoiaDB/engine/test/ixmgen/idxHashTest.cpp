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

   Source File Name = idxHashTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dmsStorageDataCommon.hpp"
#include "mthModifier.hpp"
#include "ixmKey.hpp"
#include "../bson/bson.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

TEST( idxHashTest, test_base )
{
   dmsMBStatInfo mbStatInfo ;
   mbStatInfo.setIdxHash( 0, "_id" ) ;
   mbStatInfo.setIdxHash( 1, "a" ) ;
   mbStatInfo.setIdxHash( 1, "b" ) ;

   cout << "cl: " << mbStatInfo._clIdxHashBitmap.toString() << endl ;
   cout << "idx 0: " << mbStatInfo._idxHashFields[ 0 ].toString() << endl ;
   cout << "idx 1: " << mbStatInfo._idxHashFields[ 1 ].toString() << endl ;

   ixmIdxHashBitmap fieldBitmap1 ;
   fieldBitmap1.setFieldBit( "_id" ) ;

   cout << "id : " << fieldBitmap1.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap2 ;
   fieldBitmap2.setFieldBit( "a" ) ;

   cout << "a : " << fieldBitmap2.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap3 ;
   fieldBitmap3.setFieldBit( "a" ) ;
   fieldBitmap3.setFieldBit( "b" ) ;

   cout << "a, b : " << fieldBitmap3.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap4 ;
   fieldBitmap4.setFieldBit( "_id" ) ;
   fieldBitmap4.setFieldBit( "a" ) ;
   fieldBitmap4.setFieldBit( "b" ) ;

   cout << "id, a, b : " << fieldBitmap4.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap5 ;
   fieldBitmap5.setFieldBit( "d" ) ;

   cout << "d : " << fieldBitmap5.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap6 ;
   fieldBitmap6.setFieldBit( "a1" ) ;

   cout << "a1 : " << fieldBitmap6.toString() << endl ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 0, fieldBitmap1 ) ) ;
   for ( UINT32 i = 1 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap1 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap1 ) ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap2 ) ) ;
   ASSERT_FALSE( mbStatInfo.testIdxHash( 0, fieldBitmap2 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 1, fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap2 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap2 ) ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap3 ) ) ;
   ASSERT_FALSE( mbStatInfo.testIdxHash( 0, fieldBitmap3 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 1, fieldBitmap3 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap3 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap3 ) ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 0, fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 1, fieldBitmap4 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap4 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap4 ) ) ;
   }

   ASSERT_FALSE( mbStatInfo.testIdxHash( fieldBitmap5 ) ) ;
   for ( UINT32 i = 0 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap5 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap5 ) ) ;
   }

   ASSERT_FALSE( mbStatInfo.testIdxHash( fieldBitmap6 ) ) ;
   for ( UINT32 i = 0 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap6 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap6 ) ) ;
   }
}

TEST( idxHashTest, test_drop_index )
{
   dmsMBStatInfo mbStatInfo ;
   mbStatInfo.setIdxHash( 0, "_id" ) ;
   mbStatInfo.setIdxHash( 1, "a" ) ;
   mbStatInfo.setIdxHash( 1, "b" ) ;
   mbStatInfo.setIdxHash( 2, "c" ) ;
   mbStatInfo.setIdxHash( 2, "d" ) ;
   mbStatInfo.setIdxHash( 3, "e" ) ;

   cout << "cl: " << mbStatInfo._clIdxHashBitmap.toString() << endl ;
   cout << "idx 0: " << mbStatInfo._idxHashFields[ 0 ].toString() << endl ;
   cout << "idx 1: " << mbStatInfo._idxHashFields[ 1 ].toString() << endl ;
   cout << "idx 2: " << mbStatInfo._idxHashFields[ 2 ].toString() << endl ;
   cout << "idx 3: " << mbStatInfo._idxHashFields[ 3 ].toString() << endl ;

   ixmIdxHashBitmap fieldBitmap1 ;
   fieldBitmap1.setFieldBit( "_id" ) ;

   ixmIdxHashBitmap fieldBitmap2 ;
   fieldBitmap2.setFieldBit( "a" ) ;
   fieldBitmap2.setFieldBit( "b" ) ;

   ixmIdxHashBitmap fieldBitmap3 ;
   fieldBitmap3.setFieldBit( "c" ) ;
   fieldBitmap3.setFieldBit( "d" ) ;

   ixmIdxHashBitmap fieldBitmap4 ;
   fieldBitmap4.setFieldBit( "e" ) ;

   ixmIdxHashBitmap fieldBitmap5 ;
   fieldBitmap5.setFieldBit( "_id" ) ;
   fieldBitmap5.setFieldBit( "a" ) ;
   fieldBitmap5.setFieldBit( "b" ) ;
   fieldBitmap5.setFieldBit( "c" ) ;
   fieldBitmap5.setFieldBit( "d" ) ;
   fieldBitmap5.setFieldBit( "e" ) ;

   ixmIdxHashBitmap fieldBitmap6 ;
   fieldBitmap6.setFieldBit( "_id" ) ;
   fieldBitmap6.setFieldBit( "a" ) ;
   fieldBitmap6.setFieldBit( "b" ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap5 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap2 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 2 ].isEqual( fieldBitmap3 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 3 ].isEqual( fieldBitmap4 ) ) ;
   for ( UINT32 i = 4 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }

   // unset index 1
   mbStatInfo.resetIdxHashFrom( 2 ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEmpty() ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }

   // merge bitmaps of indexes into collection
   mbStatInfo.mergeIdxHash( 0 ) ;
   mbStatInfo.mergeIdxHash( 1 ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap6 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }
}

TEST( idxHashTest, test_add_index )
{
   dmsMBStatInfo mbStatInfo ;
   mbStatInfo.setIdxHash( 0, "_id" ) ;
   mbStatInfo.setIdxHash( 1, "a" ) ;
   mbStatInfo.setIdxHash( 1, "b" ) ;

   cout << "cl: " << mbStatInfo._clIdxHashBitmap.toString() << endl ;
   cout << "idx 0: " << mbStatInfo._idxHashFields[ 0 ].toString() << endl ;
   cout << "idx 1: " << mbStatInfo._idxHashFields[ 1 ].toString() << endl ;

   ixmIdxHashBitmap fieldBitmap1 ;
   fieldBitmap1.setFieldBit( "_id" ) ;

   ixmIdxHashBitmap fieldBitmap2 ;
   fieldBitmap2.setFieldBit( "a" ) ;
   fieldBitmap2.setFieldBit( "b" ) ;

   ixmIdxHashBitmap fieldBitmap3 ;
   fieldBitmap3.setFieldBit( "c" ) ;
   fieldBitmap3.setFieldBit( "d" ) ;

   ixmIdxHashBitmap fieldBitmap4 ;
   fieldBitmap4.setFieldBit( "_id" ) ;
   fieldBitmap4.setFieldBit( "a" ) ;
   fieldBitmap4.setFieldBit( "b" ) ;

   ixmIdxHashBitmap fieldBitmap5 ;
   fieldBitmap5.setFieldBit( "_id" ) ;
   fieldBitmap5.setFieldBit( "a" ) ;
   fieldBitmap5.setFieldBit( "b" ) ;
   fieldBitmap5.setFieldBit( "c" ) ;
   fieldBitmap5.setFieldBit( "d" ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }

   // set index 2
   mbStatInfo.resetIdxHashFrom( 2 ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEmpty() ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }

   // merge bitmaps of indexes into collection
   mbStatInfo.mergeIdxHash( 0 ) ;
   mbStatInfo.mergeIdxHash( 1 ) ;
   mbStatInfo.setIdxHash( 2, "c" ) ;
   mbStatInfo.setIdxHash( 2, "d" ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap5 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 1 ].isEqual( fieldBitmap2 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashFields[ 2 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 3 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashFields[ i ].isEmpty() ) ;
   }
}

TEST( idxHashTest, test_set )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;

   ixmIdxHashBitmap bitmap ;
   bitmap.setFieldBit( "a" ) ;
   bitmap.setFieldBit( "b" ) ;
   bitmap.setFieldBit( "c" ) ;
   bitmap.setFieldBit( "d" ) ;

   BSONObj updator = BSON( "$set" << BSON( "a" << 1 << "b" << 1 ) <<
                           "$unset" << BSON( "c" << 1 << "d" << 1 ) ) ;
   rc = modifier.loadPattern( updator, NULL, TRUE, NULL, TRUE,
                              DPS_LOG_WRITE_MOD_INCREMENT, TRUE ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( modifier.getIdxHashBitmap().isEqual( bitmap ) ) ;
}

TEST( idxHashTest, test_rename )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;

   ixmIdxHashBitmap bitmap ;
   bitmap.setFieldBit( "a" ) ;
   bitmap.setFieldBit( "b" ) ;

   BSONObj updator = BSON( "$rename" << BSON( "a" << "b" ) ) ;
   rc = modifier.loadPattern( updator, NULL, TRUE, NULL, TRUE,
                              DPS_LOG_WRITE_MOD_INCREMENT, TRUE ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( modifier.getIdxHashBitmap().isEqual( bitmap ) ) ;
}

TEST( idxHashTest, test_replace )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;

   BSONObj updator = BSON( "$replace" << BSON( "a" << 1 << "b" << 1 ) ) ;
   rc = modifier.loadPattern( updator, NULL, TRUE, NULL, TRUE,
                              DPS_LOG_WRITE_MOD_INCREMENT, TRUE ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( modifier.getIdxHashBitmap().isFull() ) ;
}

TEST( idxHashTest, test_multifield_index )
{
   ixmIdxHashArray bitmap ;

   for ( UINT32 i = 0 ; i < IXM_IDX_HASH_FIELD_NUM + 1 ; ++ i )
   {
      bitmap.setField( i ) ;
   }

   // too many fields, should not be valid
   ASSERT_TRUE( !bitmap.isValid() ) ;
}

static void _idxHashBSONIXMKey( const CHAR *buffer,
                                const BSONObj &object,
                                BOOLEAN canCompact,
                                UINT32 &hashValue )
{
   cout << "check BSON " << object.toString().c_str() << endl ;

   ixmKeyOwned keyComp( object ) ;
   ixmKey keyBSON( buffer ) ;

   ASSERT_TRUE( keyComp.isCompactFormat() == canCompact ) ;
   ASSERT_TRUE( !keyBSON.isCompactFormat() ) ;

   UINT32 hashComp = keyComp.toHash() ;
   UINT32 hashBSON = keyBSON.toHash() ;

   cout << "hash compact " << hashComp << " hash BSON " << hashBSON << endl ;

   ASSERT_TRUE( hashComp == hashBSON ) ;

   hashValue = hashComp ;
}

template < typename T >
static void _idxHashTypeIXMKey( BSONType bsonType,
                                T value,
                                BOOLEAN canCompact,
                                UINT32 &hashValue )
{
   cout << "check type " << (INT32)bsonType << endl ;

   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.append( "", value ) ;
   BSONObj bo = builder.done() ;

   ASSERT_TRUE( bsonType == bo.firstElement().type() ) ;

   _idxHashBSONIXMKey( buffer.buf(), bo, canCompact, hashValue ) ;
}

TEST( idxHash, test_ixmkey_min )
{
   UINT32 hashValue = 0 ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.appendMinKey( "" ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_max )
{
   UINT32 hashValue = 0 ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.appendMaxKey( "" ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_null )
{
   UINT32 hashValue = 0 ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.appendNull( "" ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_undefined )
{
   UINT32 hashValue = 0 ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.appendUndefined( "" ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_int )
{
   UINT32 hashInt = 0, hashLong = 0, hashDouble = 0, hashDecimal = 0 ;

   _idxHashTypeIXMKey<INT32>( NumberInt, 1, TRUE, hashInt ) ;
   _idxHashTypeIXMKey<INT64>( NumberLong, 1LL, TRUE, hashLong ) ;
   _idxHashTypeIXMKey<FLOAT64>( NumberDouble, 1.0f, TRUE, hashDouble ) ;

   BufBuilder bufferDecimal ;
   bufferDecimal.appendUChar( 0xFF ) ;
   BSONObjBuilder builderDecimal( bufferDecimal ) ;
   builderDecimal.appendDecimal( "", "1" ) ;
   BSONObj boDecimal = builderDecimal.done() ;
   _idxHashBSONIXMKey( bufferDecimal.buf(), boDecimal, FALSE, hashDecimal ) ;

   ASSERT_TRUE( hashInt == hashLong ) ;
   ASSERT_TRUE( hashInt == hashDouble ) ;
   ASSERT_TRUE( hashInt == hashDecimal ) ;
}

TEST( idxHash, test_ixmkey_large_long )
{
   UINT32 hashValue = 0 ;
   INT64 value = ( 2LL << 52 ) + 1LL ;
   _idxHashTypeIXMKey<INT64>( NumberLong, value, FALSE, hashValue ) ;
   _idxHashTypeIXMKey<INT64>( NumberLong, -value, FALSE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_nan_double )
{
   // special case for NAN, can not compact
   UINT32 hashValue = 0 ;
   FLOAT64 value = numeric_limits<double>::quiet_NaN() ;
   _idxHashTypeIXMKey<FLOAT64>( NumberDouble, value, FALSE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_string )
{
   UINT32 hashValue = 0 ;
   _idxHashTypeIXMKey<const CHAR *>( String, "abcdefg", TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_long_string )
{
   CHAR value[ 300 ] ;
   ossMemset( value, 'a', 299 ) ;
   value[ 299 ] = '\0' ;

   UINT32 hashValue = 0 ;
   _idxHashTypeIXMKey<const CHAR *>( String, value, FALSE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_bool )
{
   UINT32 hashValue = 0 ;
   _idxHashTypeIXMKey<bool>( Bool, true, TRUE, hashValue ) ;
   _idxHashTypeIXMKey<bool>( Bool, false, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_oid )
{
   UINT32 hashValue = 0 ;
   OID value = OID::gen() ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.appendOID( "", &value ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_date )
{
   UINT32 hashValue = 0 ;
   Date_t value( time( NULL ) * 1000 ) ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   builder.appendDate( "", value ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_bindata )
{
   UINT32 hashValue = 0 ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;
   const CHAR *value = "abcdefg" ;
   builder.appendBinData( "", ossStrlen( value ) - 1, BinDataGeneral, value ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_large_bindata )
{
   UINT32 hashValue = 0 ;
   BufBuilder buffer ;
   buffer.appendUChar( 0xFF ) ;
   BSONObjBuilder builder( buffer ) ;

   CHAR value[ 300 ] ;
   ossMemset( value, 'a', 299 ) ;
   value[ 299 ] = '\0' ;

   builder.appendBinData( "", ossStrlen( value ) - 1, BinDataGeneral, value ) ;
   BSONObj bo = builder.done() ;
   _idxHashBSONIXMKey( buffer.buf(), bo, FALSE, hashValue ) ;
}

TEST( idxHash, test_ixmkey_multi_field )
{
   UINT32 hashValue = 0 ;

   {
      BufBuilder buffer ;
      buffer.appendUChar( 0xFF ) ;
      BSONObjBuilder builder( buffer ) ;
      builder.appendDecimal( "", "1" ) ;
      builder.append( "", 2LL ) ;
      builder.append( "", 3 ) ;
      builder.append( "", 4.0 ) ;
      BSONObj bo = builder.done() ;
      _idxHashBSONIXMKey( buffer.buf(), bo, FALSE, hashValue ) ;
   }

   {
      BufBuilder buffer ;
      buffer.appendUChar( 0xFF ) ;
      BSONObjBuilder builder( buffer ) ;
      builder.append( "", 2LL ) ;
      builder.append( "", 3 ) ;
      builder.append( "", 4.0 ) ;
      BSONObj bo = builder.done() ;
      _idxHashBSONIXMKey( buffer.buf(), bo, TRUE, hashValue ) ;
   }
}
