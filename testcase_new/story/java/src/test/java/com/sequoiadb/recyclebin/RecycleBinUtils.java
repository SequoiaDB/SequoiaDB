package com.sequoiadb.recyclebin;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.base.DBLob;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class RecycleBinUtils {

    public static void cleanRecycleBin( Sequoiadb db, String originName ) {
        BasicBSONObject query = new BasicBSONObject();
        query.put( "OriginName",
                new BasicBSONObject( "$regex", originName + "+" ) );
        query.put( "Type", "CollectionSpace" );
        DBCursor rc = db.getRecycleBin().list( query,
                new BasicBSONObject( "RecycleName", "" ).append( "OriginName",
                        "" ),
                null );
        while ( rc.hasNext() ) {
            BSONObject cur = rc.getNext();
            String recycleName = cur.get( "RecycleName" ).toString();
            String origin = cur.get( "OriginName" ).toString();
            try {
                db.getRecycleBin().dropItem( recycleName,
                        new BasicBSONObject( "Recursive", true ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        rc.close();

    }

    public static List< String > getRecycleName( Sequoiadb db,
            String originName ) {
        BasicBSONObject query = new BasicBSONObject();
        query.put( "OriginName", originName );
        query.put( "OpType", "Drop" );
        return getRecycleName( db, query );
    }

    public static List< String > getRecycleName( Sequoiadb db,
            String originName, String opType ) {
        BasicBSONObject query = new BasicBSONObject();
        query.put( "OriginName", originName );
        query.put( "OpType", opType );
        return getRecycleName( db, query );
    }

    public static List< String > getRecycleName( Sequoiadb db,
            BasicBSONObject query ) {
        List< String > recycleNames = new ArrayList<>();
        DBCursor rc = db.getRecycleBin().list( query,
                new BasicBSONObject( "RecycleName", 1 ),
                new BasicBSONObject( "RecycleName", 1 ) );
        while ( rc.hasNext() ) {
            String recycleName = ( String ) rc.getNext().get( "RecycleName" );
            recycleNames.add( recycleName );
        }
        rc.close();
        return recycleNames;
    }

    public static void checkRecycleItem( Sequoiadb db, String recycleName ) {
        BasicBSONObject option = new BasicBSONObject();
        option.put( "RecycleName", recycleName );
        long recycleItem = db.getRecycleBin().getCount( option );
        if ( recycleItem != 0 ) {
            Assert.fail( "Item not cleared : " + recycleName );
        }
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, String orderBy ) {
        DBCursor cursor = dbcl.query( "", "", orderBy, "" );

        int count = 0;
        while ( cursor.hasNext() ) {

            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            if ( !expRecord.equals( record ) ) {
                Assert.fail( "record: " + record.toString() + "\nexp: "
                        + expRecord.toString() );
            }
            Assert.assertEquals( record, expRecord );
        }
        if ( count != expRecords.size() ) {
            Assert.fail(
                    "actNum: " + count + "\nexpNum: " + expRecords.size() );
        }
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        int batchNum = 5000;
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        int count = 0;
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                String stringValue = getRandomString( length );
                int value = count++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "a", value );
                obj.put( "b", stringValue );
                obj.put( "c", value );
                obj.put( "d", value );
                batchRecords.add( obj );
            }
            dbcl.bulkInsert( batchRecords );
            insertRecord.addAll( batchRecords );
            batchRecords.clear();
        }
        return insertRecord;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {
        return insertData( dbcl, recordNum, 5 );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuilder sbBuilder = new StringBuilder();

        // random generation 80-length string.
        Random random = new Random();
        StringBuilder subBuilder = new StringBuilder();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuilder.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuilder.append( subBuilder );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuilder.append( str.substring( 0, subTimes ) );
        }
        return sbBuilder.toString();
    }

    public static LinkedBlockingQueue< SaveOidAndMd5 > writeLobAndGetMd5(
            DBCollection dbcl, int lobtimes ) {
        Random random = new Random();
        LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue<>();
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = getRandomBytes( writeLobSize );
            ObjectId oid = createAndWriteLob( dbcl, wlobBuff );

            // save oid and md5
            String prevMd5 = getMd5( wlobBuff );
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
        }
        return id2md5;
    }

    public static class SaveOidAndMd5 {
        private ObjectId oid;
        private String md5;

        public SaveOidAndMd5( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }

        public ObjectId getOid() {
            return oid;
        }

        public String getMd5() {
            return md5;
        }
    }

    public static void ReadLob( DBCollection dbcl,
            LinkedBlockingQueue< SaveOidAndMd5 > id2md5 )
            throws InterruptedException {
        SaveOidAndMd5 oidAndMd5 = id2md5.take();
        ObjectId oid = oidAndMd5.getOid();

        try ( DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ )) {
            byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
            rLob.read( rbuff );
            String curMd5 = getMd5( rbuff );
            String prevMd5 = oidAndMd5.getMd5();
            Assert.assertEquals( curMd5, prevMd5 );
        }
        id2md5.offer( oidAndMd5 );
    }

    /**
     * generating byte to write lob
     *
     * @param length
     *            generating byte stream size
     * @return byte[] bytes
     */
    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    public static ObjectId createAndWriteLob( DBCollection dbcl, byte[] data ) {
        return createAndWriteLob( dbcl, null, data );
    }

    static ObjectId createAndWriteLob( DBCollection dbcl, ObjectId id,
            byte[] data ) {
        DBLob lob = dbcl.createLob( id );
        lob.write( data );
        lob.close();
        return lob.getID();
    }

    /**
     * get the buff MD5 value
     *
     * @param inbuff
     *            the object of need to get the MD5
     * @return the MD5 value
     */
    public static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
    }

}
