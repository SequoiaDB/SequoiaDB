package com.sequoiadb.datasrc;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: IndexUtils.java public call function for test index
 * 
 * @author wuyan
 * @Date 2020.12.04
 * @version 1.00
 */
public class DataSrcUtils {

    public static void clearDataSource( Sequoiadb db, String csName,
            String dataSrcName ) {
        if ( db.isCollectionSpaceExist( csName ) ) {
            db.dropCollectionSpace( csName );
        }
        if ( db.isDataSourceExist( dataSrcName ) ) {
            db.dropDataSource( dataSrcName );
        }
    }

    public static void createDataSource( Sequoiadb db, String name ) {
        BasicBSONObject obj = new BasicBSONObject();
        db.createDataSource( name, DataSrcUtils.getSrcUrl(),
                DataSrcUtils.getUser(), DataSrcUtils.getPasswd(), "", obj );
    }

    public static void createCSWithDataSource( Sequoiadb db, String name,
            String csName ) {
        BasicBSONObject obj = new BasicBSONObject();
        db.createDataSource( name, DataSrcUtils.getSrcUrl(),
                DataSrcUtils.getUser(), DataSrcUtils.getPasswd(), "", obj );
    }

    public static String getSrcUrl() {
        String dataSrcIp = SdbTestBase.dsHostName;
        String port = SdbTestBase.dsServiceName;
        return dataSrcIp + ":" + port;
    }

    public static String getUser() {
        String name = "sdbadmin";
        return name;
    }

    public static String getPasswd() {
        String passwd = "sdbadmin";
        return passwd;
    }

    public static DBCollection createCSAndCL( Sequoiadb sdb, String csName,
            String clName ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        DBCollection dbcl = cs.createCollection( clName );
        return dbcl;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int beginNo ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        Random random = new Random();
        for ( int i = beginNo; i < beginNo + recordNum; i++ ) {
            String keyValue = getRandomString( random.nextInt( 30 ) );
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue );
            obj.put( "no", i );
            obj.put( "num", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, String matcher ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'no':1}", "" );

        int count = 0;
        while ( cursor.hasNext() ) {

            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            if ( !expRecord.equals( record ) ) {
                throw new BaseException( 0, "record: " + record.toString()
                        + "\nexp: " + expRecord.toString() );
            }
            Assert.assertEquals( record, expRecord );
        }
        cursor.close();
        if ( count != expRecords.size() ) {
            throw new BaseException( 0,
                    "actNum: " + count + "\nexpNum: " + expRecords.size() );
        }
        Assert.assertEquals( count, expRecords.size() );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuffer sbBuffer = new StringBuffer();
        // random generation 80-length string.
        Random random = new Random();
        StringBuffer subBuffer = new StringBuffer();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuffer.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuffer.append( subBuffer );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuffer.append( str.substring( 0, subTimes ) );
        }
        return sbBuffer.toString();
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
    
}
