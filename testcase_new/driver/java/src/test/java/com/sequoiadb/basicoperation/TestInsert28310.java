package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.options.InsertOption;
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
 * @Descreption seqDB-28310:不带OID批插数据,testa:使用BSONObject
 *              insertRecords(List<BSONObject> insertor, int flags)接口
 * @author wuyan
 * @Date 2022.10.20
 * @version 1.00
 */
public class TestInsert28310 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String csName = "insertcs28310";
    private String clNameBase = "insert28310";
    private String srcGroupName;
    private String destGroupName;

    @DataProvider(name = "operData")
    public Object[][] generateShardingKey() {
        return new Object[][] {
                // the parameters: clNo, clName
                new Object[] { 1, clNameBase + "_a" },
                new Object[] { 2, clNameBase + "_b" },
                new Object[] { 3, clNameBase + "_c" },
                new Object[] { 4, clNameBase + "_d" },
                new Object[] { 5, clNameBase + "_e" },};
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
        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupNames.get( 0 );
        destGroupName = groupNames.get( 1 );
    }

    @Test(dataProvider = "operData")
    public void test( int clNo, String clName ) {
        DBCollection cl = createCL( clName );
        int splitPercent = 50;
        cl.split( srcGroupName, destGroupName, splitPercent );

        int recordNum = 5000;
        List< BSONObject > expResults = new ArrayList< BSONObject >();
        if ( clNo == 1 ) {
            expResults = insertDataA( cl, recordNum );
        } else if ( clNo == 2 ) {
            expResults = insertDataB( cl, recordNum );
        } else if ( clNo == 3 ) {
            expResults = insertDataC( cl, recordNum );
        } else if ( clNo == 4 ) {
            expResults = insertDataD( cl, recordNum );
        } else if ( clNo == 5 ) {
            expResults = insertDataE( cl, recordNum );
        }

        Assert.assertEquals( cl.getCount(), recordNum );

        // 直连源组和目标组主节点检查数据
        Node srcMaterNodeInfo = sdb.getReplicaGroup( srcGroupName ).getMaster();
        Node destMaterNodeInfo = sdb.getReplicaGroup( destGroupName )
                .getMaster();
        long srcCount = checkRecordNumOnData( clName, srcMaterNodeInfo,
                recordNum );
        long destCount = checkRecordNumOnData( clName, destMaterNodeInfo,
                recordNum );
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

    private DBCollection createCL( String clName ) {
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "_id", 1 );
        options.put( "ShardingKey", keyValue );
        options.put( "Group", srcGroupName );
        DBCollection cl = cs.createCollection( clName, options );
        return cl;
    }

    private List< BSONObject > insertDataA( DBCollection cl, int recordNum ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "b", "testb" + i );
            insertRecords.add( obj );
        }
        cl.insertRecords( insertRecords, InsertOption.FLG_INSERT_CONTONDUP );
        return insertRecords;
    }

    private List< BSONObject > insertDataB( DBCollection cl, int recordNum ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "testb", "testb_" + i );
            insertRecords.add( obj );
        }
        cl.insert( insertRecords, InsertOption.FLG_INSERT_REPLACEONDUP );
        return insertRecords;
    }

    private List< BSONObject > insertDataC( DBCollection cl, int recordNum ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "testb", "testb_" + i );
            insertRecords.add( obj );
        }
        cl.bulkInsert( insertRecords );
        return insertRecords;
    }

    private List< BSONObject > insertDataD( DBCollection cl, int recordNum ) {
        List< BSONObject > insertRecords = new ArrayList<>();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "testb", "testb_" + i );
            insertRecords.add( obj );
        }
        cl.bulkInsert( insertRecords );
        return insertRecords;
    }

    private List< BSONObject > insertDataE( DBCollection cl, int recordNum ) {
        List< BSONObject > insertRecords = new ArrayList<>();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "testb", "testb_" + i );
            insertRecords.add( obj );
        }
        InsertOption option = new InsertOption();
        option.setFlag( InsertOption.FLG_INSERT_CONTONDUP );
        cl.bulkInsert( insertRecords, option );
        return insertRecords;
    }

    private long checkRecordNumOnData( String clName, Node nodeInfo,
            int recordNum ) {
        long count = 0;
        try ( Sequoiadb srcDB = new Sequoiadb( nodeInfo.getNodeName(), "",
                "" )) {
            DBCollection cl = srcDB.getCollectionSpace( csName )
                    .getCollection( clName );
            count = cl.getCount();
        }
        // 切分误差在不超过50%
        int expCount = recordNum / 2;
        Assert.assertTrue( Math.abs( count - expCount ) / expCount < 0.5,
                "actCount = " + count );
        return count;
    }
}