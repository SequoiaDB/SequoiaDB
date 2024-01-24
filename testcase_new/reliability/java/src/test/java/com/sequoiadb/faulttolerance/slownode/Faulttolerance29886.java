package com.sequoiadb.faulttolerance.slownode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @Description seqDB-29866:容错级别为半容错，只有1个副本状态正常，其他副本状态为:SLOWNODE，开启/不开启事务读其他集合数据
 * @Author liuli
 * @Date 2023.01.11
 * @UpdateAuthor liuli
 * @UpdateDate 2023.01.11
 * @version 1.10
 */
public class Faulttolerance29886 extends SdbTestBase {

    private String csName = "cs_29866";
    private String clName1 = "cl_29866_1";
    private String clName2 = "cl_29866_2";
    private String testCLName = "cl_29866_test";
    private byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );
    private GroupMgr groupMgr = null;
    private List< String > groupNames = new ArrayList<>();
    private List< String > slaveNodeNames = new ArrayList<>();
    private Sequoiadb sdb = null;
    private boolean shutoff = false;
    private boolean runSuccess = false;
    private List< BSONObject > insertRecords1 = new ArrayList<>();
    private List< BSONObject > insertRecords2 = new ArrayList<>();

    @BeforeClass
    public void setUp() throws Exception {

        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }

        groupNames = groupMgr.getAllDataGroupName();

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        DBCollection dbcl1 = dbcs.createCollection( clName1,
                new BasicBSONObject( "Group", groupNames.get( 0 ) ) );
        int recordNum = 5;
        insertRecords1 = insertData( dbcl1, recordNum );

        DBCollection dbcl2 = dbcs.createCollection( clName2,
                new BasicBSONObject( "Group", groupNames.get( 1 ) ) );
        insertRecords2 = insertData( dbcl2, recordNum );

        dbcs.createCollection( testCLName,
                new BasicBSONObject( "Group", groupNames.get( 0 ) ) );

        BSONObject config = new BasicBSONObject();
        config.put( "ftlevel", 2 );
        config.put( "ftmask", "SLOWNODE" );
        config.put( "ftfusingtimeout", 10 );
        config.put( "ftconfirmperiod", 3 );
        config.put( "ftslownodethreshold", 1 );
        config.put( "ftslownodeincrement", 1 );
        sdb.updateConfig( config, null );

        GroupWrapper gwp = groupMgr.getGroupByName( groupNames.get( 0 ) );
        String masterName = sdb.getReplicaGroup( groupNames.get( 0 ) )
                .getMaster().getNodeName();
        List< String > allNodeName = gwp.getAllUrls();
        for ( String nodeName : allNodeName ) {
            if ( !nodeName.equals( masterName ) ) {
                slaveNodeNames.add( nodeName );
            }
        }
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {

        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < 5; i++ ) {
            mgr.addTask( new Insert() );
            mgr.addTask( new Update() );
            mgr.addTask( new PutLobTask() );
        }
        mgr.addTask( new TestSlowNode() );

        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            // 恢复配置
            BSONObject config = new BasicBSONObject();
            config.put( "ftlevel", 1 );
            config.put( "ftmask", 1 );
            config.put( "ftconfirmperiod", 1 );
            config.put( "ftslownodethreshold", 1 );
            config.put( "ftslownodeincrement", 1 );
            sdb.deleteConfig( config, new BasicBSONObject() );
            sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ) );

            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    class Insert extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( testCLName );
                while ( !shutoff ) {
                    // 构造插入数据
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 5000; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdhfssdljfhjdshfjksdhfsdfhsdjdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshafdsdljfhjdshfjksdhfsdfhsdjkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkdssdljfhjdshfjksdhfsdfhsdjhfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwnqsdljfhjdshfjksdhfsdfhsdjefiuokdjf" );
                        records.add( record );
                    }
                    // 当插入数据报错时不停止线程，只有shutoff为true时才停止线程
                    cl.bulkInsert( records );
                }
            }
        }
    }

    class Update extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( testCLName );
                for ( int i = 0; i < 5000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    BasicBSONObject modifier = new BasicBSONObject();
                    modifier.put( "$inc",
                            new BasicBSONObject( "a", 1 ).append( "b", 1 ) );
                    modifier.put( "$set", new BasicBSONObject( "str",
                            "update str times " + i ) );
                    cl.upsertRecords( null, modifier, null );
                }
            }
        }
    }

    class PutLobTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( testCLName );
                while ( !shutoff ) {
                    DBLob lob = cl.createLob();
                    lob.write( lobBuff );
                    lob.close();
                }
            }
        }
    }

    class TestSlowNode extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( testCLName );
                int slowNodeNum;
                for ( int i = 0; i < 6000; i++ ) {
                    boolean nodeSlow = false;
                    slowNodeNum = 0;
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        if ( ft.equals( "SLOWNODE" )
                                || ft.equals( "SLOWNODE|DEADSYNC" ) ) {
                            slowNodeNum++;
                        }
                        if ( slowNodeNum == 2 ) {
                            nodeSlow = true;
                            break;
                        }
                    }
                    if ( nodeSlow ) {
                        break;
                    } else {
                        if ( i == 5999 ) {
                            System.out.println(
                                    "600 seconds still not executed." );
                            Assert.fail( "600 seconds still not executed." );
                        }
                        Thread.sleep( 100 );
                    }
                }

                try ( Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        System.out.println( new Date() + " "
                                + this.getClass().getName() + " begin insert "
                                + nodeName + " ft is : " + ft );
                        sdb.msg( "Faulttolerance22202 begin insert: " + nodeName
                                + " ft is : " + ft );
                    }

                    db2.beginTransaction();
                    DBCollection dbcl1 = db2.getCollectionSpace( csName )
                            .getCollection( clName1 );
                    DBCollection dbcl2 = db2.getCollectionSpace( csName )
                            .getCollection( clName2 );
                    DBCursor cursor1 = dbcl1.query( null, null,
                            new BasicBSONObject( "a", 1 ), null );
                    checkRecords( cursor1, insertRecords1 );
                    DBCursor cursor2 = dbcl2.query( null, null,
                            new BasicBSONObject( "a", 1 ), null );
                    checkRecords( cursor2, insertRecords2 );
                    db2.commit();

                    cursor1 = dbcl1.query( null, null,
                            new BasicBSONObject( "a", 1 ), null );
                    checkRecords( cursor1, insertRecords1 );
                    cursor2 = dbcl2.query( null, null,
                            new BasicBSONObject( "a", 1 ), null );
                    checkRecords( cursor2, insertRecords2 );
                }
            } finally {
                shutoff = true;
            }
        }
    }

    public static void checkRecords( DBCursor cursor,
            List< BSONObject > expRecords ) {
        int count = 0;
        while ( cursor.hasNext() ) {

            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            if ( !expRecord.equals( record ) ) {
                Assert.fail( "record: " + record.toString() + "\nexp: "
                        + expRecord.toString() );
            }
            Assert.assertEquals( record, expRecord );
        }
        cursor.close();
        if ( count != expRecords.size() ) {
            Assert.fail(
                    "actNum: " + count + "\nexpNum: " + expRecords.size() );
        }
        Assert.assertEquals( count, expRecords.size() );
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {
        ArrayList< BSONObject > insertRecord = new ArrayList<>();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "b", i );
            obj.put( "c", i );
            obj.put( "d", i );
            insertRecord.add( obj );
        }
        dbcl.bulkInsert( insertRecord );
        return insertRecord;
    }

}
