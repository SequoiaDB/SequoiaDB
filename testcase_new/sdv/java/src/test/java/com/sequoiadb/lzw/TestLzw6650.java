package com.sequoiadb.lzw;

import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6650:插入记录数=100条，总记录大小=64M 1、CL压缩类型为lzw，插入记录满足如下条件：
 *                                           插入记录数=100条，总记录大小=64M
 *                                           2、检查返回结果，并检查压缩字典是否构建
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6650 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6650";
    private String dataGroupName = null;
    private AtomicInteger id = new AtomicInteger( 0 );
    private String bigStr = LzwUtils2.getRandomString( 512 * 1024 );
    private String smallStr = LzwUtils2.getRandomString( 1024 );
    private int recsNum1 = 120; // dictionary not created
    private int recsNum2 = 20; // dictionary created
    private int recsNum3 = 5; // dictionary created, insert again records

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( LzwUtils2.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @SuppressWarnings("deprecation")
    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl = createCL();
            // threshold is not reached
            insertData( cl, recsNum1, bigStr );
            Assert.assertEquals( LzwUtils2.isDictExist( cl, dataGroupName ),
                    false,
                    "Dictionary is created when threshold is not reached!" );

            // threshold is reached
            insertData( cl, recsNum2, bigStr );
            LzwUtils2.waitCreateDict( cl, dataGroupName );

            // insert some records for compression
            insertData( cl, recsNum3, smallStr );

            // check result
            checkData( cl, recsNum1 + recsNum2 + recsNum3 );
            LzwUtils2.checkCompressed( cl, dataGroupName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private DBCollection createCL() {
        DBCollection cl = null;
        BSONObject option = new BasicBSONObject();
        try {
            dataGroupName = LzwUtils2.getDataGroups( sdb ).get( 0 );
            option.put( "Group", dataGroupName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    private void insertData( DBCollection cl, int recSum, String str ) {
        for ( int i = 0; i < recSum; i++ ) {
            BSONObject rec = new BasicBSONObject();
            rec.put( "_id", id.getAndIncrement() );
            rec.put( "key", str + rec.get( "_id" ) );
            cl.insert( rec );
        }
    }

    public void checkData( DBCollection cl, int expCnt ) {
        int actCnt = ( int ) cl.getCount();
        Assert.assertEquals( actCnt, expCnt, "data is different at count" );
        DBCursor cursor = cl.query( null, null, "{_id:1}", null );
        BSONObject expRec = new BasicBSONObject();
        for ( int i = 0; i < recsNum1 + recsNum2; i++ ) {
            expRec.put( "_id", i );
            expRec.put( "key", bigStr + expRec.get( "_id" ) );
            BSONObject actRec = cursor.getNext();
            Assert.assertEquals( actRec, expRec, "data is different" );
            // clean up expRec
            expRec.removeField( "_id" );
            expRec.removeField( "key" );
        }
        for ( int i = recsNum1 + recsNum2; i < recsNum1 + recsNum2
                + recsNum3; i++ ) {
            expRec.put( "_id", i );
            expRec.put( "key", smallStr + expRec.get( "_id" ) );
            BSONObject actRec = cursor.getNext();
            Assert.assertEquals( actRec, expRec, "data is different" );
            // clean up expRec
            expRec.removeField( "_id" );
            expRec.removeField( "key" );
        }
        cursor.close();
    }
}