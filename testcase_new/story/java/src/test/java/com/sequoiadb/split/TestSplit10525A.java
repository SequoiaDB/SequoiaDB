package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 创建cl时指定副本数为0，执行切分
 * 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestSplit10525A extends SdbTestBase {
    private Sequoiadb sdb;
    private ArrayList< String > rgNames;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10525_A";
    private List< BSONObject > insertRecods = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone, or only one group, "
                            + "skip the testCase." );
        }

        rgNames = CommLib.getDataGroupNames( sdb );

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ReplSize:0,ShardingKey:{age:1},"
                        + "ShardingType:\"range\",Group:\"" + rgNames.get( 0 )
                        + "\"}" );
        cl = SplitUtils2.createCL( cs, clName, option );
    }

    /**
     * 1. 创建cl到指定组 2. 分区键字段排序为正序 3. 执行切分 4. 分别连接coord、源组data、目标组data查询
     * 
     * @throws InterruptedException
     */
    @Test
    public void test() throws InterruptedException {
        BSONObject startCondition = ( BSONObject ) JSON.parse( "{age:25}" );
        BSONObject endCondition = ( BSONObject ) JSON.parse( "{age:50}" );
        long splitAsyncId = cl.splitAsync( rgNames.get( 0 ), rgNames.get( 1 ),
                startCondition, endCondition );

        insertRecods = SplitUtils2.insertData( cl, 100 );

        // 等待切分任务完成再校验数据
        long[] taskIDs = { splitAsyncId };
        this.sdb.waitTasks( taskIDs );

        // 连接coord节点验证数据是否正确
        testCoordSplitResult();

        // 连接源组data验证数据
        List< BSONObject > srcExpectData = this.getSrcExpectData();
        checkSplitResult( rgNames.get( 0 ), srcExpectData );

        // 连接目标组data验证数据
        List< BSONObject > dstExpectData = this.getDstExpectData();
        checkSplitResult( rgNames.get( 1 ), dstExpectData );
    }

    public void testCoordSplitResult() {
        List< BSONObject > actual = new ArrayList< BSONObject >();
        DBCursor cursor = cl.query( null, null, "{\"_id\":1}", null );
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            actual.add( obj );
        }
        cursor.close();
        Assert.assertEquals( actual, insertRecods );
    }

    public void checkSplitResult( String rgName, List< BSONObject > expData )
            throws InterruptedException {
        Sequoiadb dataDb = null;
        try {
            // 连接源组从节点data验证数据
            ReplicaGroup replicaGroup = this.sdb.getReplicaGroup( rgName );
            dataDb = replicaGroup.getSlave().connect();

            // 元数据
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( clName );
            Thread.sleep( 100 );

            // 数据
            DBCursor cursor = dbcl.query( null, null, "{\"_id\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            Assert.assertEquals( actual, expData );
        } finally {
            dataDb.disconnect();
        }
    }

    private List< BSONObject > getSrcExpectData() {
        // 切分键[25-50)
        // 期望结果[1-25),[50,100)
        List< BSONObject > expData = new ArrayList< BSONObject >();
        for ( int i = 1; i < 25; i++ ) {
            expData.add( insertRecods.get( i - 1 ) );
        }
        for ( int i = 50; i < 100; i++ ) {
            expData.add( insertRecods.get( i - 1 ) );
        }
        return expData;
    }

    private List< BSONObject > getDstExpectData() {
        // 切分键[25-50)
        // 期望结果[25-50)
        List< BSONObject > expData = new ArrayList< BSONObject >();
        for ( int i = 25; i < 50; i++ ) {
            expData.add( insertRecods.get( i - 1 ) );
        }
        return expData;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            this.sdb.disconnect();
        }
    }
}
