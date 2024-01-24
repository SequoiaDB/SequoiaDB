package com.sequoiadb.faulttolerance.slownode;

import java.util.ArrayList;
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
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22208:容错级别为全容错，只有1个副本状态正常，其他副本状态为:SLOWNODE，不同replSize的集合中插入数据
 * @author wuyan
 * @Date 2020.06.09
 * @version 1.00
 */
public class faulttolerance22208 extends SdbTestBase {

    private String csName = "cs22208";
    private String clName = "cl22208";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private int[] replSizes = { 0, 1, -1, 2 };
    private byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );
    private String ftmask = "SLOWNODE";
    private boolean isStopOpr = false;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{'Group': '" + groupName + "'}" ) );

        for ( int i = 0; i < replSizes.length; i++ ) {
            BSONObject option = ( BSONObject ) JSON.parse( "{Group:'"
                    + groupName + "'," + "ReplSize:" + replSizes[ i ] + " }" );
            cs.createCollection( clName + "_" + i, option );
        }

        updateConf( ftmask );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        List< NodeWrapper > nodes = dataGroup.getNodes();
        for ( int i = 0; i < nodes.size(); i++ ) {
            if ( nodes.get( i ).isMaster() ) {
                nodes.remove( i );
            }
        }
        System.out.println( "---nodes=" + nodes.toString() );
        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < 20; i++ ) {
            mgr.addTask( new InsertTask() );
            mgr.addTask( new UpdateTask() );
            mgr.addTask( new PutLobTask() );
        }
        mgr.addTask( new TestSlowNode( nodes ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        insertAndCheckResult( sdb, csName, clName, false );
        FaultToleranceUtils
                .checkNodeStatus( nodes.get( 0 ).connect().toString(), "" );
        FaultToleranceUtils
                .checkNodeStatus( nodes.get( 1 ).connect().toString(), "" );
        // insertAgainAndCheckResult(csName, clName,true);
        insertAndCheckResult( sdb, csName, clName, true );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            deleteConf( groupName );
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }

    private class InsertTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( isStopOpr ) {
                        break;
                    }
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 5000; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdhfsdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshafdkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkdshfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwnqefiuokdjf" );
                        records.add( record );
                    }
                    cl.insert( records );
                }
            }
        }
    }

    private class UpdateTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( isStopOpr ) {
                        break;
                    }
                    cl.update( null,
                            "{$inc:{a:1, b:1}, $set:{'str':'update str times "
                                    + i + "'}}",
                            null );
                }
            }
        }
    }

    private class PutLobTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( isStopOpr ) {
                        break;
                    }
                    DBLob lob = cl.createLob();
                    lob.write( lobBuff );
                    lob.close();
                }
            }
        }
    }

    private class TestSlowNode extends OperateTask {
        List< NodeWrapper > nodes;

        public TestSlowNode( List< NodeWrapper > nodes ) {
            this.nodes = nodes;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                int eachSleepTime = 2;
                int maxWaitTime = 600000;
                int alreadyWaitTime = 0;
                boolean isSlowNode = false;

                do {
                    NodeWrapper node1 = nodes.get( 0 );
                    NodeWrapper node2 = nodes.get( 1 );
                    String NodeName1 = node1.hostName() + ":" + node1.svcName();
                    String NodeName2 = node2.hostName() + ":" + node2.svcName();
                    String ftStatus1 = FaultToleranceUtils.getNodeFTStatus( db,
                            NodeName1 );
                    String ftStatus2 = FaultToleranceUtils.getNodeFTStatus( db,
                            NodeName2 );
                    if ( ftStatus1.equals( ftmask )
                            && ftStatus2.equals( ftmask ) ) {
                        isSlowNode = true;
                        insertAndCheckResult( db, csName, clName, false );
                        isStopOpr = true;
                    }
                    try {
                        Thread.sleep( eachSleepTime );
                    } catch ( InterruptedException e ) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    alreadyWaitTime += eachSleepTime;
                    if ( alreadyWaitTime > maxWaitTime ) {
                        System.out.println(
                                "---node status is not slownode, in maxWaitTime ! waitTime is"
                                        + alreadyWaitTime );
                        Assert.fail();
                    }
                } while ( !isSlowNode );

            } finally {
                isStopOpr = true;
            }
        }
    }

    private void updateConf( String ftmask ) {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "ftmask", ftmask );
        configs.put( "ftlevel", 3 );
        configs.put( "ftslownodethreshold", 1 );
        configs.put( "ftslownodeincrement", 1 );
        configs.put( "ftfusingtimeout", 10 );
        options.put( "GroupName", groupName );
        sdb.updateConfig( configs, options );
    }

    private void deleteConf( String ftmask ) {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "ftmask", 1 );
        configs.put( "ftlevel", 1 );
        configs.put( "ftslownodethreshold", 1 );
        configs.put( "ftslownodeincrement", 1 );
        options.put( "GroupName", groupName );
        sdb.deleteConfig( configs, options );
        sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ),
                options );
    }

    private void insertAndCheckResult( Sequoiadb sdb, String csName,
            String clName, boolean isInsertAgain ) {
        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName + "_" + i );
            BSONObject record = new BasicBSONObject();
            record.put( "a", "text22208_test" + i );
            if ( i == 1 || isInsertAgain ) {
                dbcl.insert( record );
                long count = dbcl.getCount( record );
                Assert.assertEquals( count, 1, "the cl " + clName + "_" + i
                        + " insert context is:" + record );
            } else {
                try {
                    dbcl.insert( record );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -105 && e.getErrorCode() != -13
                            && e.getErrorCode() != -252 ) {
                        throw e;
                    }
                }
            }
        }
    }
}
