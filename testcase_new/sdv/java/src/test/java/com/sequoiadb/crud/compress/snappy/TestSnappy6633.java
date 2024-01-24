package com.sequoiadb.crud.compress.snappy;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * @FileName:seqDB-6633: upsert批量更新不存在的字段
 *                       1、CL压缩类型为snappy，upsert指定常用更新符（如$gt/$lt/$set...）批量更新不存在的字段
 *                       2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-20
 * @Version 1.00
 */
public class TestSnappy6633 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6633";
    private String dataGroupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( SnappyUilts.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        try {
            DBCollection cl = createCL();
            SnappyUilts.insertData( cl, 1000 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
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
            DBCollection cl = null;
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            // do upsert(with $gte)
            BSONObject matcher = ( BSONObject ) JSON.parse( "{a:{$gte:500}}" );
            BSONObject modifier = ( BSONObject ) JSON.parse( "{$set:{c:1}}" );
            cl.upsert( matcher, modifier, null );

            // do upsert(with $lt)
            matcher = ( BSONObject ) JSON.parse( "{a:{$lt:600}}" );
            modifier = ( BSONObject ) JSON.parse( "{$set:{d:2}}" );
            cl.upsert( matcher, modifier, null );

            // check result
            checkUpserted( cl );
            SnappyUilts.checkCompressed( cl, dataGroupName );
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
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        BSONObject option = new BasicBSONObject();
        try {
            dataGroupName = SnappyUilts.getDataGroups( sdb ).get( 0 );
            option.put( "Group", dataGroupName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "snappy" );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    private void checkUpserted( DBCollection cl ) {
        // check counts
        int cCount = ( int ) cl.getCount( "{c:1}" );
        int dCount = ( int ) cl.getCount( "{d:2}" );
        if ( !( cCount == 500 && dCount == 600 ) ) {
            Assert.fail( "upsert fail!" );
        }
        // check data at [0,500)
        DBCursor cursor = cl.query( "", "{_id:{$include:0}}", "{a:1}", "" );
        for ( int i = 0; i < 500; i++ ) {
            BSONObject exp = new BasicBSONObject();
            exp.put( "a", i );
            exp.put( "b", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" );
            exp.put( "d", 2 );
            BSONObject act = cursor.getNext();
            Assert.assertEquals( exp, act, "data is different at [0, 500)" );
        }
        // check data at [500,600)
        for ( int i = 500; i < 600; i++ ) {
            BSONObject exp = new BasicBSONObject();
            exp.put( "a", i );
            exp.put( "b", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" );
            exp.put( "c", 1 );
            exp.put( "d", 2 );
            BSONObject act = cursor.getNext();
            Assert.assertEquals( exp, act, "data is different at [500, 600)" );
        }
        // check data at [500,1000)
        for ( int i = 600; i < 1000; i++ ) {
            BSONObject exp = new BasicBSONObject();
            exp.put( "a", i );
            exp.put( "b", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" );
            exp.put( "c", 1 );
            BSONObject act = cursor.getNext();
            Assert.assertEquals( exp, act, "data is different at [600, 1000)" );
        }
    }
}
