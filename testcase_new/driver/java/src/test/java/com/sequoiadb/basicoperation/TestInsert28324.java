package com.sequoiadb.basicoperation;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-28324:分区表指定分区键包含oid，不带OID批插数据后执行切分
 * @author wuyan
 * @Date 2022.10.19
 * @version 1.00
 */
public class TestInsert28324 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String csName = "insertcs28324";
    private String clNameBase = "insert28324";
    private String srcGroupName;
    private String destGroupName;

    @DataProvider(name = "operData")
    public Object[][] generateShardingKey() {
        BasicBSONObject keyValue1 = new BasicBSONObject();
        keyValue1.put( "_id", 1 );
        keyValue1.put( "a", 1 );
        BasicBSONObject keyValue2 = new BasicBSONObject();
        keyValue2.put( "a", 1 );
        keyValue2.put( "_id", 1 );
        BasicBSONObject keyValue3 = new BasicBSONObject();
        keyValue3.put( "_id", -1 );
        keyValue3.put( "a", 1 );

        return new Object[][] {
                // the parameters: clNo, shardingKey
                new Object[] { 1, keyValue1 }, new Object[] { 2, keyValue2 },
                new Object[] { 3, keyValue3 }, };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        if ( Commlib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
    }

    @Test(dataProvider = "operData")
    public void test( int clNo, BasicBSONObject keyValue ) {
        String clName = clNameBase + "_" + clNo;
        DBCollection cl = createCL( clName, keyValue );
        int splitPercent = 50;
        cl.split( srcGroupName, destGroupName, splitPercent );
        int recordNum = 5000;
        ArrayList< BSONObject > expResults = insertData( cl, recordNum );
        Assert.assertEquals( cl.getCount(), recordNum );

        // 直连源组和目标组主节点检查数据
        Node srcMaterNodeInfo = sdb.getReplicaGroup( srcGroupName ).getMaster();
        Node destMaterNodeInfo = sdb.getReplicaGroup( destGroupName )
                .getMaster();
        long srcCount = Commlib.checkRecordNumOnData( csName, clName,
                srcMaterNodeInfo, recordNum );
        long destCount = Commlib.checkRecordNumOnData( csName, clName,
                destMaterNodeInfo, recordNum );
        Assert.assertEquals( srcCount + destCount, recordNum );
        DBCursor queryCursor = cl.query( null, null, "{a:1}", null );
        CommLib.checkRecordsResult( queryCursor, expResults );
        // 执行切分，设置100%切分到目标组
        cl.split( srcGroupName, destGroupName, 100 );
        // 直连源组和目标组主节点检查数据

        Commlib.checkAllSplitToDestGroupResult( csName, clName,
                srcMaterNodeInfo, destMaterNodeInfo, recordNum, expResults );
        // coord查询数据值正确
        DBCursor queryCursor1 = cl.query( null, null, "{a:1}", null );
        CommLib.checkRecordsResult( queryCursor1, expResults );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            sdb.close();
        }
    }

    private DBCollection createCL( String clName, BasicBSONObject keyValue ) {
        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupNames.get( 0 );
        destGroupName = groupNames.get( 1 );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", keyValue );
        options.put( "Group", srcGroupName );
        DBCollection cl = cs.createCollection( clName, options );
        return cl;
    }

    private ArrayList< BSONObject > insertData( DBCollection cl,
            int recordNum ) {
        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "testb", "testb_" + i );
            insertRecords.add( obj );
        }
        cl.bulkInsert( insertRecords );
        return insertRecords;
    }
}