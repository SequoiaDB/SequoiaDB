package com.sequoiadb.faulttolerance.diskfull;

import java.util.List;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

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
 * @Description seqDB-22201:容错级别为半容错，只有1个副本状态正常，其他副本状态为:NOSPC或者DEADSYNC，replSize=-1的集合下插入数据
 * @Author Zhao Xiaoni
 * @Date 2020-6-4
 * @UpdatreAuthor liuli
 * @UpdateDate 2020-12-19
 */
public class FaultTolerance22201 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_22201";
    private String clName = "cl_22201";
    private GroupMgr groupMgr = null;
    private String groupName = null;

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
        sdb.createCollectionSpace( csName ).createCollection( clName,
                new BasicBSONObject( "Group", groupName ).append( "ReplSize",
                        -1 ) );
    }

    @DataProvider(name = "circuitBreaker")
    public Object[][] configs() {
        return new Object[][] { { "NOSPC", 2, 10 }, { "DEADSYNC", 2, 10 } };
    }

    @Test(dataProvider = "circuitBreaker")
    public void test( String ftmask, int ftlevel, int ftfusingtimeout )
            throws Exception {
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
        mgr.addTask( new putLob( slaveNode1, slaveNode2, ftmask ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        FaultToleranceUtils.checkNodeStatus( slaveNode1, ftmask );
        FaultToleranceUtils.checkNodeStatus( slaveNode2, ftmask );

        FaultToleranceUtils.putLob( csName, clName, 10, -105 );

        diskFull1.restore();
        diskFull2.restore();
        diskFull1.checkRestoreResult();
        diskFull2.checkRestoreResult();
        diskFull1.fini();
        diskFull2.fini();

        FaultToleranceUtils.checkNodeStatus( slaveNode1, "" );
        FaultToleranceUtils.checkNodeStatus( slaveNode2, "" );

        FaultToleranceUtils.insertError( csName, clName, 0 );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( dataGroup.checkInspect( 1 ) );
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

    private class putLob extends OperateTask {
        String nodeName1;
        String nodeName2;
        String ftmask;

        public putLob( String nodeName1, String nodeName2, String ftmask ) {
            this.nodeName1 = nodeName1;
            this.nodeName2 = nodeName2;
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
                                && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                        .getErrorCode() ) {
                            throw e;
                        }
                    }
                    String ftStatus1 = FaultToleranceUtils.getNodeFTStatus( db,
                            nodeName1 );
                    String ftStatus2 = FaultToleranceUtils.getNodeFTStatus( db,
                            nodeName2 );
                    if ( ftmask.equals( ftStatus1 )
                            && ftmask.equals( ftStatus2 ) ) {
                        break;
                    }
                }
            }
        }
    }
}
