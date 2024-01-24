package com.sequoiadb.split.basefunc;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
 * @FileName:seqDB-10880: hash分区表指定分区键值为date类型，执行切分 1、创建cl，指定shardingType为“hash”
 *                        2、向cl中插入数据，其中分区键字段值为date类型
 *                        3、执行splitAysnc异步切分，设置百分比切分条件
 *                        4、查看数据切分结果（分别连接coord、源组data、目标组data查询
 * @Author linsuqiang
 * @Date 2017-01-04
 * @Version 1.00
 */
public class TestSplit10880 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl_10880";
    private String srcGroup = null;
    private String dstGroup = null;
    private List< BSONObject > insertedRecs = new ArrayList< BSONObject >();
    private double percent;
    private int recsCnt;
    private int srcExpCnt;
    private int dstExpCnt;
    private int offSet;
    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            if ( SplitBaseUtils.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( SplitBaseUtils.OneGroupMode( sdb ) ) {
                throw new SkipException( "skip One group mode" );
            }
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

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
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        try {
            initData();
            DBCollection cl = SplitBaseUtils.createHashCl( sdb, clName,
                    srcGroup );
            cl.bulkInsert( insertedRecs, 0 );
            // splitAsync
            long[] taskIDs = new long[ 1 ];
            taskIDs[ 0 ] = cl.splitAsync( srcGroup, dstGroup, percent );
            sdb.waitTasks( taskIDs );
            // check result
            SplitBaseUtils.checkSplitOnCoord( cl, insertedRecs );
            Sequoiadb srcDB = SplitBaseUtils.getDataDB( sdb, srcGroup );
            int srcCnt = SplitBaseUtils.checkSplitOnData( srcDB, clName,
                    srcExpCnt, offSet );
            Sequoiadb dstDB = SplitBaseUtils.getDataDB( sdb, dstGroup );
            int dstCnt = SplitBaseUtils.checkSplitOnData( dstDB, clName,
                    dstExpCnt, offSet );
            Assert.assertEquals( srcCnt + dstCnt, recsCnt, "srcCnt: " + srcCnt
                    + "dstCnt: " + dstCnt + "recsCnt:" + recsCnt );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    private void initData() {
        // initialize data group information
        List< String > rgNames = SplitBaseUtils.getDataGroups( sdb );
        srcGroup = rgNames.get( 0 );
        dstGroup = rgNames.get( 1 );
        // initialize records to insert
        recsCnt = 1000;
        for ( int i = 0; i < recsCnt; i++ ) {
            SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd" );
            Date date = null;
            try {
                date = sdf.parse( "2016-12-" + i );
            } catch ( ParseException e ) {
                Assert.fail( e.getMessage() );
            }
            BSONObject rec = new BasicBSONObject();
            rec.put( "a", date );
            insertedRecs.add( rec );
        }
        // initialize split condition
        percent = 50;
        // define excepted data
        srcExpCnt = recsCnt / 2;
        dstExpCnt = recsCnt / 2;
        offSet = ( int ) ( recsCnt * 0.3 );
    }
}
