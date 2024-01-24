package com.sequoiadb.faulttolerance.diskfull;

import java.util.List;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
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
 * @Description seqDB-22210: 主节点的选主权重最高，主备节点lsn一致，主节点磁盘满，部分备节点磁盘满
 * @Author Zhao Xiaoni
 * @Date 2020-6-4
 * @UpdatreAuthor liuli
 * @UpdateDate 2020-12-19
 */
public class FaultTolerance22210 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_22210";
    private String clName = "cl_22210";
    private CollectionSpace cs = null;
    private GroupMgr groupMgr = null;
    private GroupWrapper dataGroup = null;
    private String groupName = null;
    private String masterNode = "";
    private int[] replSizes = { 0, 1 };

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }

        groupMgr = GroupMgr.getInstance();
        groupName = groupMgr.getAllDataGroupName().get( 0 );
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

    @Test()
    public void test() throws Exception {
        sdb.updateConfig(
                new BasicBSONObject( "ftmask", "NOSPC" )
                        .append( "ftfusingtimeout", 10 ),
                new BasicBSONObject( "GroupName", groupName ) );
        dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();
        masterNode = master.hostName() + ":" + master.svcName();
        List< NodeWrapper > slaveNodes = FaultToleranceUtils.getSlaveNodes( sdb,
                groupName, groupMgr );
        String slaveNode1 = slaveNodes.get( 0 ).hostName() + ":"
                + slaveNodes.get( 0 ).svcName();
        String slaveNode2 = slaveNodes.get( 1 ).hostName() + ":"
                + slaveNodes.get( 1 ).svcName();
        sdb.updateConfig( new BasicBSONObject( "weight", 50 ),
                new BasicBSONObject( "NodeName", masterNode ) );

        DiskFull diskFull1 = new DiskFull( slaveNodes.get( 0 ).hostName(),
                slaveNodes.get( 0 ).dbPath() );
        diskFull1.init();
        diskFull1.make();

        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new putLob( csName, clName + "_0", slaveNode1, "NOSPC" ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        FaultToleranceUtils.checkNodeStatus( slaveNode1, "NOSPC" );

        DiskFull diskFull2 = new DiskFull( master.hostName(), master.dbPath() );
        diskFull2.init();
        diskFull2.make();

        TaskMgr mgr2 = new TaskMgr();
        mgr2.addTask(
                new putLob( csName, clName + "_1", masterNode, "NOSPC" ) );
        mgr2.execute();
        Assert.assertTrue( mgr2.isAllSuccess(), mgr2.getErrorMsg() );
        FaultToleranceUtils.checkNodeStatus( masterNode, "NOSPC" );

        checkMasterChanged( slaveNode2 );

        diskFull1.restore();
        diskFull2.restore();
        diskFull1.checkRestoreResult();
        diskFull2.checkRestoreResult();
        diskFull1.fini();
        diskFull2.fini();

        FaultToleranceUtils.checkNodeStatus( slaveNode1, "" );
        FaultToleranceUtils.checkNodeStatus( masterNode, "" );

        FaultToleranceUtils.insertError( csName, clName + "_0", 0 );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            sdb.deleteConfig( new BasicBSONObject( "weight", 1 ),
                    new BasicBSONObject( "NodeName", masterNode ) );
            sdb.deleteConfig( new BasicBSONObject( "ftmask", 1 ),
                    new BasicBSONObject( "GroupName", groupName ) );
            sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ) );
            sdb.close();
        }
    }

    private class putLob extends OperateTask {
        String csName;
        String clName;
        String nodeName;
        String ftmask;

        public putLob( String csName, String clName, String nodeName,
                String ftmask ) {
            this.csName = csName;
            this.clName = clName;
            this.nodeName = nodeName;
            this.ftmask = ftmask;
        }

        @Override
        public void exec() throws Exception {
            byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 3000; i++ ) {
                    try {
                        DBLob lob = dbcl.createLob();
                        lob.write( lobBuff );
                        lob.close();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_CLS_WAIT_SYNC_FAILED
                                .getErrorCode()
                                && e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                                        .getErrorCode()
                                && e.getErrorCode() != SDBError.SDB_NOSPC
                                        .getErrorCode() ) {
                            throw e;
                        }
                    }
                    String ftStatus = FaultToleranceUtils.getNodeFTStatus( db,
                            nodeName );
                    if ( ftmask.equals( ftStatus ) ) {
                        break;
                    }
                }
            }
        }
    }

    public void checkMasterChanged( String slaveNode )
            throws ReliabilityException {
        NodeWrapper testmaster = null;
        int testTimes = 0;
        int timeout = 600;
        String masterNode = "";
        do {
            testTimes++;
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
            testmaster = dataGroup.getMaster();
            if ( testmaster != null ) {
                masterNode = testmaster.hostName() + ":" + testmaster.svcName();
            }
        } while ( !masterNode.equals( slaveNode ) && testTimes < timeout );
        if ( testTimes > timeout ) {
            Assert.fail( "The expected master node is " + slaveNode
                    + ", but the actual master node is " + masterNode );
        }
    }
}
