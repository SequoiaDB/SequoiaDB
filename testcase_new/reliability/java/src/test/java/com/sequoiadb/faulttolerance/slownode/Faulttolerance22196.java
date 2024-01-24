package com.sequoiadb.faulttolerance.slownode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupWrapper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22196:容错级别为熔断，1个副本状态为:SLOWNODE，不同replSize的集合中插入数据
 * @Author luweikang
 * @Date 2020.06.05
 * @UpdateAuthor liuli
 * @UpdateDate 2021.12.01
 * @version 1.10
 */
public class Faulttolerance22196 extends SdbTestBase {

    private String csName = "cs22196";
    private String clName = "cl22196";
    private String clName2 = "newcl22196_2";
    private int[] replSizes = { -1, 0, 1, 2 };
    private byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );
    private List< String > slaveNodeNames = new ArrayList<>();
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private boolean shutoff = false;
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() throws Exception {
        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }

        String groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        DBCollection dbcl = cs.createCollection( clName2,
                new BasicBSONObject( "Group", groupName ) );

        int recordNum = 500000;
        FaultToleranceUtils.insertData( dbcl, recordNum );

        for ( int i = 0; i < replSizes.length; i++ ) {
            cs.createCollection( clName + "_" + i,
                    new BasicBSONObject( "Group", groupName )
                            .append( "ReplSize", replSizes[ i ] ) );
        }

        BSONObject config = new BasicBSONObject();
        config.put( "ftlevel", 1 );
        config.put( "ftmask", "SLOWNODE" );
        config.put( "ftfusingtimeout", 10 );
        sdb.updateConfig( config,
                new BasicBSONObject( "GroupName", groupName ) );

        GroupWrapper gwp = groupMgr.getGroupByName( groupName );
        String masterName = sdb.getReplicaGroup( groupName ).getMaster()
                .getNodeName();
        List< String > allNodeName = gwp.getAllUrls();
        for ( String slaveNodeName : allNodeName ) {
            if ( !slaveNodeName.equals( masterName ) ) {
                slaveNodeNames.add( slaveNodeName );
            }
        }

        BSONObject slaveConfig1 = new BasicBSONObject();
        slaveConfig1.put( "ftconfirmperiod", 3 );
        slaveConfig1.put( "ftslownodethreshold", 1 );
        slaveConfig1.put( "ftslownodeincrement", 1 );
        sdb.updateConfig( slaveConfig1,
                new BasicBSONObject( "NodeName", slaveNodeNames.get( 0 ) ) );

        BSONObject slaveConfig2 = new BasicBSONObject();
        slaveConfig2.put( "ftconfirmperiod", 600 );
        slaveConfig2.put( "ftslownodethreshold", 1000 );
        slaveConfig2.put( "ftslownodeincrement", 1000 );
        sdb.updateConfig( slaveConfig2,
                new BasicBSONObject( "NodeName", slaveNodeNames.get( 1 ) ) );
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
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                DBCollection cl2 = dbcs.getCollection( clName2 );
                while ( !shutoff ) {
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 5000; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdjflsdljfhjdshfjksdhfsdfhsdjkhfsdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshajflsdljfhjdshfjksdhfsdfhsdjkfdkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkjflsdljfhjdshfjksdhfsdfhsdjkdshfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwjflsdljfhjdshfjksdhfsdfhsdjknqefiuokdjf" );
                        records.add( record );
                    }
                    cl2.insert( records );
                }
            }
        }
    }

    class Update extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                DBCollection cl2 = dbcs.getCollection( clName2 );
                for ( int i = 0; i < 5000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    BasicBSONObject modifier = new BasicBSONObject();
                    modifier.put( "$inc",
                            new BasicBSONObject( "a", 1 ).append( "b", 1 ) );
                    modifier.put( "$set", new BasicBSONObject( "str",
                            "update str times " + i ) );
                    cl2.update( null, modifier, null );
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
                        .getCollection( clName2 );
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
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                DBCollection cl1 = dbcs.getCollection( clName + "_0" );
                DBCollection cl2 = dbcs.getCollection( clName + "_1" );
                DBCollection cl3 = dbcs.getCollection( clName + "_2" );
                DBCollection cl4 = dbcs.getCollection( clName + "_3" );
                for ( int i = 0; i < 6000; i++ ) {
                    String ft = FaultToleranceUtils.getNodeFTStatus( db,
                            slaveNodeNames.get( 0 ) );
                    if ( "SLOWNODE".equals( ft )
                            || "SLOWNODE|DEADSYNC".equals( ft ) ) {
                        System.out.println( new Date() + " "
                                + this.getClass().getName() + " "
                                + slaveNodeNames.get( 0 ) + " ft is : " + ft );
                        break;
                    } else {
                        if ( i == 5999 ) {
                            shutoff = true;
                            System.out.println(
                                    "600 seconds still not executed." );
                            Assert.fail( "600 seconds still not executed." );
                        }
                        Thread.sleep( 100 );
                    }
                }

                try {
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        System.out.println( new Date() + " "
                                + this.getClass().getName() + " begin insert "
                                + nodeName + " ft is : " + ft );
                        sdb.msg( "Faulttolerance22196 begin insert ReplSize -1: "
                                + nodeName + " ft is : " + ft );
                    }
                    cl1.insert( "{a:1}" );
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        System.out.println( new Date() + " "
                                + this.getClass().getName() + " insert success "
                                + nodeName + " ft is : " + ft );
                        sdb.msg( "Faulttolerance22196 insert success ReplSize -1: "
                                + nodeName + " ft is : " + ft );
                    }
                    // 当节点状态恢复时会插入成功，插入成功后校验数据
                    // Assert.fail(
                    // "ReplSize:-1 cl write data must be error when node slow"
                    // );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -105
                            && e.getErrorCode() != -252 ) {
                        throw e;
                    }
                }

                try {
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        System.out.println( new Date() + " "
                                + this.getClass().getName() + " begin insert "
                                + nodeName + " ft is : " + ft );
                        sdb.msg( "Faulttolerance22196 begin insert ReplSize 0: "
                                + nodeName + " ft is : " + ft );
                    }
                    cl2.insert( "{a:1}" );
                    for ( String nodeName : slaveNodeNames ) {
                        String ft = FaultToleranceUtils.getNodeFTStatus( db,
                                nodeName );
                        System.out.println( new Date() + " "
                                + this.getClass().getName() + " insert success "
                                + nodeName + " ft is : " + ft );
                        sdb.msg( "Faulttolerance22196 insert success ReplSize 0: "
                                + nodeName + " ft is : " + ft );
                    }
                    // 当节点状态恢复时会插入成功，插入成功后校验数据
                    // Assert.fail(
                    // "ReplSize:0 cl write data must be error when node slow"
                    // );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -105
                            && e.getErrorCode() != -252 ) {
                        throw e;
                    }
                }

                cl3.insert( "{a:1}" );
                cl4.insert( "{a:1}" );
            } finally {
                shutoff = true;
            }
        }
    }
}
