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
 * @FileName:seqDB-6654:插入16M大小的记录 1、CL压缩类型为lzw，插入多条16M大小的记录 2、检查返回结果，并检查数据压缩情况
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6654 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6654";
    private String dataGroupName = null;
    private AtomicInteger id = new AtomicInteger( 0 );
    private String bigStr = LzwUtils2.getRandomString( 15 * 1024 * 1024 );
    private String smallStr = LzwUtils2.getRandomString( 512 * 1024 );

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
            // insert many records for creating dictionary
            insertData( cl, 100, smallStr );
            insertData( cl, 1, bigStr );
            LzwUtils2.waitCreateDict( cl, dataGroupName );
            // insert some records for compression
            insertData( cl, 2, bigStr );
            // check result
            checkData( cl, 100 + 1 + 2 );
            LzwUtils2.checkCompressed( cl, dataGroupName );
            // CRUD
            doAndCheckCRUD( cl );
        } catch ( BaseException e ) {
            e.printStackTrace();
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
        BSONObject rec = new BasicBSONObject();
        for ( int i = 0; i < recSum; i++ ) {
            rec.put( "_id", id.getAndIncrement() );
            rec.put( "key", str + rec.get( "_id" ) );
            cl.insert( rec );
            rec.removeField( "_id" );
            rec.removeField( "key" );
        }
    }

    public void checkData( DBCollection cl, int expCnt ) {
        int actCnt = ( int ) cl.getCount();
        Assert.assertEquals( actCnt, expCnt, "data is different at count" );
        DBCursor cursor = cl.query( null, null, "{_id:1}", null );
        BSONObject expRec = new BasicBSONObject();
        for ( int i = 0; i < 100; i++ ) {
            expRec.put( "_id", i );
            expRec.put( "key", smallStr + expRec.get( "_id" ) );
            BSONObject actRec = cursor.getNext();
            Assert.assertEquals( actRec, expRec, "data is different" );
            // clean up expRec
            expRec.removeField( "_id" );
            expRec.removeField( "key" );
        }
        for ( int i = 100; i < 100 + 1 + 2; i++ ) {
            expRec.put( "_id", i );
            expRec.put( "key", bigStr + expRec.get( "_id" ) );
            BSONObject actRec = cursor.getNext();
            Assert.assertEquals( actRec, expRec, "data is different" );
            // clean up expRec
            expRec.removeField( "_id" );
            expRec.removeField( "key" );
        }
        cursor.close();
    }

    private void doAndCheckCRUD( DBCollection cl ) {
        // insert
        BSONObject insRec = new BasicBSONObject();
        long currId = id.get();
        insRec.put( "_id", currId );
        insRec.put( "key", "aaaaaaaaaaaaaaa" );
        cl.insert( insRec );

        // check insert by query
        BSONObject qryMatcher = new BasicBSONObject();
        qryMatcher.put( "_id", currId );
        DBCursor cursor1 = cl.query( qryMatcher, null, null, null );
        if ( !( cursor1.hasNext() && cursor1.getNext().equals( insRec ) ) ) {
            Assert.fail( "fail to query" );
        }
        cursor1.close();

        // update
        BSONObject updRec = new BasicBSONObject();
        updRec.put( "_id", currId );
        updRec.put( "key", "bbbbbbbbbbbbbbbbb" );
        BSONObject updMatcher = new BasicBSONObject();
        updMatcher = qryMatcher;
        BSONObject updModifier = new BasicBSONObject();
        BSONObject setOption = new BasicBSONObject();
        setOption.put( "key", updRec.get( "key" ) );
        updModifier.put( "$set", setOption );
        cl.update( updMatcher, updModifier, null );

        // check update by query
        DBCursor cursor2 = cl.query( qryMatcher, null, null, null );
        if ( !( cursor2.hasNext() && cursor2.getNext().equals( updRec ) ) ) {
            Assert.fail( "fail to update" );
        }
        cursor2.close();

        // delete
        BSONObject delMatcher = qryMatcher;
        cl.delete( delMatcher );

        // check delete by query
        DBCursor cursor3 = cl.query( qryMatcher, null, null, null );
        if ( cursor3.hasNext() ) {
            Assert.fail( "fail to delete" );
        }
        cursor3.close();
    }
}