package com.sequoiadb.faulttolerance.diskfull;

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
 * @Description seqDB-22195: 容错级别为熔断，1个副本状态为:NOSPC或者DEADSYNC，不同replSize的集合中插入数据
 * @Author Zhao Xiaoni
 * @Date 2020-6-4
 * @UpdatreAuthor liuli
 * @UpdateDate 2020-12-19
 */
public class FaultTolerance22195 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_22195";
    private String clName = "cl_22195";
    private CollectionSpace cs = null;
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int[] replSizes = { 0, -1, 1, 2 };

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

    @DataProvider(name = "circuitBreaker")
    public Object[][] configs() {
        return new Object[][] { { "NOSPC", 1, 10 }, { "DEADSYNC", 1, 10 } };
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
        NodeWrapper slave = dataGroup.getSlave();
        String slaveNode = slave.hostName() + ":" + slave.svcName();

        DiskFull diskFull = new DiskFull( slave.hostName(), slave.dbPath() );
        diskFull.init();
        diskFull.make();

        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new putLob( clName + "_0", slaveNode, ftmask ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        FaultToleranceUtils.checkNodeStatus( slaveNode, ftmask );

        FaultToleranceUtils.putLob( csName, clName + "_0", 10,
                SDBError.SDB_CLS_NODE_NOT_ENOUGH.getErrorCode() );
        FaultToleranceUtils.putLob( csName, clName + "_1", 10,
                SDBError.SDB_CLS_NODE_NOT_ENOUGH.getErrorCode() );
        FaultToleranceUtils.insertError( csName, clName + "_2", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_3", 0 );

        diskFull.restore();
        diskFull.checkRestoreResult();
        diskFull.fini();

        FaultToleranceUtils.checkNodeStatus( slaveNode, "" );

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
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class putLob extends OperateTask {
        String clName;
        String nodeName;
        String ftmask;

        public putLob( String clName, String nodeName, String ftmask ) {
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
                                        .getErrorCode() ) {
                            throw e;
                        }
                    }
                    String ftStatus1 = FaultToleranceUtils.getNodeFTStatus( db,
                            nodeName );
                    if ( ftmask.equals( ftStatus1 ) ) {
                        break;
                    }
                }
            }
        }
    }
}
