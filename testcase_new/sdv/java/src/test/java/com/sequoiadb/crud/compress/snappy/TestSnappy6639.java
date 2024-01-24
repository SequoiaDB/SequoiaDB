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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6639: hash范围切分，分区键为单个字段
 *                       1、CL为hash分区集合，分区键为单个字段，压缩类型为snappy，对CL做hash范围切分
 *                       2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-21
 * @Version 1.00
 */
public class TestSnappy6639 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6639";
    private String srcGroup = null;
    private String dstGroup = null;
    private int recsSum = 4096;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect failed," + SdbTestBase.coordUrl + e.getMessage() );
        }
        if ( SnappyUilts.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SnappyUilts.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }
        try {
            DBCollection cl = createCL();
            SnappyUilts.insertData( cl, recsSum );
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
        DBCollection cl = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            // do split
            BSONObject startCondition = ( BSONObject ) JSON
                    .parse( "{Partition:0}" );
            BSONObject endCondition = ( BSONObject ) JSON
                    .parse( "{Partition:2048}" );
            cl.split( srcGroup, dstGroup, startCondition, endCondition );

            // check source group
            Sequoiadb srcDataDB = SnappyUilts.getDataDB( db, srcGroup );
            SnappyUilts.checkCompression( srcDataDB, clName );
            checkSplit( srcDataDB );

            // check destination group
            Sequoiadb dstDataDB = SnappyUilts.getDataDB( db, srcGroup );
            SnappyUilts.checkCompression( dstDataDB, clName );
            checkSplit( dstDataDB );
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
            option.put( "ShardingKey", JSON.parse( "{a:1}" ) );
            option.put( "ShardingType", "hash" );
            option.put( "Compressed", true );
            option.put( "CompressionType", "snappy" );
            srcGroup = SnappyUilts.getDataGroups( sdb ).get( 0 );
            dstGroup = SnappyUilts.getDataGroups( sdb ).get( 1 );
            option.put( "Group", srcGroup );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    private void checkSplit( Sequoiadb dataDB ) {
        DBCollection cl = dataDB.getCollectionSpace( csName )
                .getCollection( clName );
        dataDB.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        int actCnt = ( int ) cl.getCount();
        int expCnt = ( int ) ( 0.5 * recsSum );
        int offSet = ( int ) ( 0.3 * recsSum );
        if ( Math.abs( actCnt - expCnt ) > offSet ) {
            Assert.fail( "the split result is wrong " + "actucl count:["
                    + actCnt + "] " + "excepted count:[" + expCnt + "]" );
        }
    }
}