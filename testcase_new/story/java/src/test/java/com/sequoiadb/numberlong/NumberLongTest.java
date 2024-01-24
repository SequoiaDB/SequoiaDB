package com.sequoiadb.numberlong;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

import com.sequoiadb.testcommon.SdbTestBase;

public class NumberLongTest extends SdbTestBase {
    private Sequoiadb sdb;
    private DBCollection cl;
    private CollectionSpace cs;

    private String clName;
    private Random random = new Random();

    @BeforeClass
    public void connectSdb() {
        if ( SdbTestBase.csName == null ) {
            Assert.fail( "csName is null" );
        }

        clName = "java_test_numberlong";
        clName += Thread.currentThread().getId();
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            System.out.printf( "connect %s failed, errMsg:%s\n", coordUrl,
                    e.getMessage() );
            Assert.assertTrue( false );
        }

        try {
            if ( sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                cs = sdb.getCollectionSpace( SdbTestBase.csName );
            } else {
                Assert.assertTrue( false, SdbTestBase.csName + " not exist" );
            }
        } catch ( BaseException e ) {
            System.out.printf(
                    "get|crate CollectionSpace %s failed, errMsg:%s\n",
                    SdbTestBase.csName, e.getMessage() );
            Assert.assertTrue( false );
        }

        try {
            if ( cs.isCollectionExist( clName ) ) {
                cl = cs.getCollection( clName );
            } else {
                cl = cs.createCollection( clName );
            }
        } catch ( BaseException e ) {
            System.out.printf(
                    "get|crate CollectionSpace %s failed, errMsg:%s\n",
                    SdbTestBase.csName + "." + clName, e.getMessage() );
            Assert.assertTrue( false );
        }
    }

    private String getNewFormatDoc( Object val ) {
        String doc = null;
        if ( val instanceof Long ) {
            doc = String.format( "{test:{$numberLong:\"%d\"}}", val );
        } else {
            doc = String.format( "{test:{$numberLong:\"%s\"}}",
                    val.toString() );
        }
        return doc;
    }

    private String getOldFormatDoc( Long val ) {
        String doc = null;
        doc = String.format( "{test:{$numberLong:%d}}", val );

        return doc;
    }

    private BSONObject getDoc( Long val ) {
        BSONObject doc = new BasicBSONObject();
        doc.put( "test", val );
        return doc;
    }

    private BSONObject testInsertJSON( String inDoc ) {
        BSONObject retDoc = null;

        try {
            cl.insert( inDoc );
            DBCursor cursor = cl.query( inDoc, null, null, null );
            while ( cursor.hasNext() ) {
                retDoc = cursor.getNext();
            }
        } catch ( BaseException e ) {
            System.out.println( e.getMessage() );
        }
        return retDoc;
    }

    private BSONObject testInsertBSON( BSONObject inDoc ) {
        BSONObject retDoc = null;

        try {
            cl.insert( inDoc );
            DBCursor cursor = cl.query( inDoc, null, null, null );
            while ( cursor.hasNext() ) {
                retDoc = cursor.getNext();
            }
        } catch ( BaseException e ) {
            System.out.println( e.getMessage() );
        }
        return retDoc;
    }

    private String getRandomString() {

        final char[] arrChar = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8',
                '9', '&', '|', '{', '.', '^', '%', '$', '#', '@', '!', '~', '*',
                '(', ')' };
        int numberCharNum = 0;
        int len = 0;
        StringBuffer buf = null;
        do {
            buf = new StringBuffer();
            len = random.nextInt( 20 );
            numberCharNum = 0;
            for ( int i = 0; i < len; ++i ) {
                char ch = arrChar[ random.nextInt( arrChar.length ) ];
                if ( ch >= '0' && ch <= '9' )
                    numberCharNum++;
                buf.append( ch );
            }
        } while ( numberCharNum == len );

        return buf.toString();
    }

    @AfterMethod
    public void removeAll() {
        try {
            cl.delete( new BasicBSONObject() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        }
    }

    @Test
    public void testInsert() {
        // BSONObject doc = getDoc(random.nextLong());
        long val = random.nextLong();
        String doc = getNewFormatDoc( val );
        BSONObject outDoc = testInsertJSON( doc );
        Assert.assertNotNull( outDoc );
        Assert.assertEquals( val,
                ( ( BasicBSONObject ) outDoc ).getLong( "test" ) );

        outDoc = testInsertBSON( getDoc( val ) );
        Assert.assertNotNull( outDoc );
        Assert.assertEquals( val,
                ( ( BasicBSONObject ) outDoc ).getLong( "test" ) );

    }

    @Test
    public void testInvalidParameter() {
        boolean ret = false;
        String doc = "";
        try {
            doc = getOldFormatDoc( random.nextLong() );
            cl.insert( doc );
        } catch ( ClassCastException e ) {
            ret = true;
        }
        Assert.assertTrue( ret, doc.toString() );

        ret = false;
        String numberLong = getRandomString();
        try {
            doc = getNewFormatDoc( numberLong );
            cl.insert( doc );
        } catch ( NumberFormatException e ) {
            ret = true;
        }
        Assert.assertTrue( ret, numberLong );
    }

    @Test
    public void testBoundaryValue() {

        // 最大值ֵ
        String doc = getNewFormatDoc( Long.MAX_VALUE );
        BSONObject outDoc = testInsertJSON( doc );
        Assert.assertNotNull( outDoc );
        Assert.assertEquals( Long.MAX_VALUE,
                ( ( BasicBSONObject ) outDoc ).getLong( "test" ) );

        outDoc = testInsertBSON( getDoc( Long.MAX_VALUE ) );
        Assert.assertNotNull( outDoc );
        Assert.assertEquals( Long.MAX_VALUE,
                ( ( BasicBSONObject ) outDoc ).getLong( "test" ) );

        // 最小值ֵ
        doc = getNewFormatDoc( Long.MIN_VALUE );
        outDoc = testInsertJSON( doc );
        Assert.assertNotNull( outDoc );
        Assert.assertEquals( Long.MIN_VALUE,
                ( ( BasicBSONObject ) outDoc ).getLong( "test" ) );

        outDoc = testInsertBSON( getDoc( Long.MIN_VALUE ) );
        Assert.assertNotNull( outDoc );
        Assert.assertEquals( Long.MIN_VALUE,
                ( ( BasicBSONObject ) outDoc ).getLong( "test" ) );
    }

    @AfterClass
    public void releaseSdb() {
        /*
         * try{ sdb.dropCollectionSpace(csName); }catch(BaseException e){
         * System.out.printf("drop CollectionSpace %s failed, errMsg:%s\n",
         * csName + "." + clName, e.getMessage()); Assert.assertTrue(false); }
         */

        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            System.out.printf( "disconnect failed, errMsg:%s\n",
                    e.getMessage() );
            Assert.assertTrue( false );
        }
    }
}
