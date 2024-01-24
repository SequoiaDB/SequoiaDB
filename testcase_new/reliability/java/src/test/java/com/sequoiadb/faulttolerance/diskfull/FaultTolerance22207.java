package com.sequoiadb.faulttolerance.diskfull;

import java.util.List;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22207:容错级别为全容错，只有1个副本状态正常，其他副本状态为:NOSPC或者DEADSYNC，不同replSize的集合中插入数据
 * @Author Zhao Xiaoni
 * @Date 2020-6-4
 * @UpdatreAuthor liuli
 * @UpdateDate 2020-12-19
 */
public class FaultTolerance22207 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_22207";
    private String clName = "cl_22207";
    private CollectionSpace cs = null;
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int[] replSizes = { 0, -1, 1, 2 };
    private boolean reachStatus;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }

        groupMgr = GroupMgr.getInstance();
        groupName = groupMgr.getAllDataGroupName().get( 1 );
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        cs = sdb.createCollectionSpace( csName );
        for ( int i = 0; i < replSizes.length; i++ ) {
            cs.createCollection( clName + "_" + i,
                    new BasicBSONObject( "Group", groupName )
                            .append( "ReplSize", replSizes[ i ] ) );
        }
    }

    @DataProvider(name = "circuitBreaker")
    public Object[][] configs() {
        return new Object[][] { { "NOSPC", 3, 10 }, { "DEADSYNC", 3, 10 } };
    }

    @Test(dataProvider = "circuitBreaker")
    public void test( String ftmask, int ftlevel, int ftfusingtimeout )
            throws Exception {
        reachStatus = false;
        BasicBSONObject configs = new BasicBSONObject( "ftmask", ftmask )
                .append( "ftlevel", ftlevel )
                .append( "ftfusingtimeout", ftfusingtimeout );
        sdb.updateConfig( configs,
                new BasicBSONObject( "GroupName", groupName ) );

        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        List< NodeWrapper > slaveNodes = FaultToleranceUtils.getSlaveNodes( sdb,
                groupName, groupMgr );
        String slaveNode1 = slaveNodes.get( 1 ).hostName() + ":"
                + slaveNodes.get( 1 ).svcName();
        String slaveNode2 = slaveNodes.get( 0 ).hostName() + ":"
                + slaveNodes.get( 0 ).svcName();

        DiskFull diskFull1 = new DiskFull( slaveNodes.get( 1 ).hostName(),
                slaveNodes.get( 1 ).dbPath() );
        DiskFull diskFull2 = new DiskFull( slaveNodes.get( 0 ).hostName(),
                slaveNodes.get( 0 ).dbPath() );
        diskFull1.init();
        diskFull2.init();
        diskFull1.make();
        diskFull2.make();

        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new putLob( clName + "_0" ) );
        mgr.addTask( new putLob( clName + "_1" ) );
        mgr.addTask( new putLob( clName + "_2" ) );
        mgr.addTask( new putLob( clName + "_3" ) );
        mgr.addTask( new checkNodeFTStatus( slaveNode1, slaveNode2, ftmask ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        diskFull1.restore();
        diskFull2.restore();
        diskFull1.checkRestoreResult();
        diskFull2.checkRestoreResult();
        diskFull1.fini();
        diskFull2.fini();

        FaultToleranceUtils.checkNodeStatus( slaveNode1, "" );
        FaultToleranceUtils.checkNodeStatus( slaveNode2, "" );

        FaultToleranceUtils.insertError( csName, clName + "_0", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_1", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_2", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_3", 0 );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            sdb.deleteConfig(
                    new BasicBSONObject( "ftmask", 1 ).append( "ftlevel", 1 ),
                    new BasicBSONObject( "GroupName", groupName ) );
            sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ) );
            sdb.close();
        }
    }

    // putLob失败后将putLobSuc改为false，当reachStatus为true，putLobSuc为false时，则代表节点达到状态且插入数据失败，此时跳出循环
    private class putLob extends OperateTask {
        String clName;

        public putLob( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );
            boolean putLobSuc = true;
            int doTime = 0;
            int timeOut = 300000;
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                while ( doTime < timeOut ) {
                    try {
                        DBLob lob = dbcl.createLob();
                        lob.write( lobBuff );
                        lob.close();
                        putLobSuc = true;
                        sleep( 60 );
                        doTime += 60;
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_CLS_WAIT_SYNC_FAILED
                                .getErrorCode()
                                && e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                                        .getErrorCode() ) {
                            throw e;
                        }
                        putLobSuc = false;
                    }
                    if ( !putLobSuc && reachStatus ) {
                        break;
                    }
                }
                if ( doTime >= timeOut ) {
                    Assert.fail( "putLob time out, putLobSuc is " + putLobSuc
                            + ", reachStatus is " + reachStatus );
                }
            }
        }
    }

    // 检测达到状态后将reachStatus改为true
    private class checkNodeFTStatus extends OperateTask {
        String nodeName1;
        String nodeName2;
        String ftmask;

        public checkNodeFTStatus( String nodeName1, String nodeName2,
                String ftmask ) {
            this.nodeName1 = nodeName1;
            this.nodeName2 = nodeName2;
            this.ftmask = ftmask;
        }

        @Override
        public void exec() throws Exception {
            int doTime = 0;
            int timeOut = 600000;
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                while ( doTime < timeOut ) {
                    String ftStatus1 = FaultToleranceUtils.getNodeFTStatus( db,
                            nodeName1 );
                    String ftStatus2 = FaultToleranceUtils.getNodeFTStatus( db,
                            nodeName2 );
                    if ( ftmask.equals( ftStatus1 )
                            && ftmask.equals( ftStatus2 ) ) {
                        reachStatus = true;
                        break;
                    }
                    sleep( 200 );
                    doTime += 200;
                }
            }
        }
    }
}
