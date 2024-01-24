package com.sequoiadb.location.diskfull;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.location.LocationUtils;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-31325:容错级别为全容错，同步策略为位置多数派优先，优先同步节点状态为NOSPC或DEADSYNC
 * @Author liuli
 * @Date 2023.05.05
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.05
 * @version 1.10
 */
public class Location31325 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_31325";
    private String clName = "cl_31325";
    private String primaryLocation = "guangzhou.nansha_31325";
    private String secondaryLocation = "guangzhou.panyu_31325";
    private CollectionSpace cs = null;
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int[] replSizes = { 0, -1, 3 };

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

        setLocation( sdb, groupName, primaryLocation, secondaryLocation );

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

        BasicBSONObject configs = new BasicBSONObject( "ftmask", ftmask )
                .append( "ftlevel", ftlevel )
                .append( "ftfusingtimeout", ftfusingtimeout );
        sdb.updateConfig( configs,
                new BasicBSONObject( "GroupName", groupName ) );

        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        BasicBSONObject secondaryLocationNode = LocationUtils
                .getGroupLocationNodes( sdb, groupName, secondaryLocation )
                .get( 0 );
        String secondaryLocationNodeName = secondaryLocationNode
                .getString( "hostName" ) + ":"
                + secondaryLocationNode.getString( "svcName" );
        String dbPath = LocationUtils.getDBPath( sdb,
                secondaryLocationNodeName );

        DiskFull diskFull = new DiskFull(
                secondaryLocationNode.getString( "hostName" ), dbPath );
        diskFull.init();
        diskFull.make();

        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new putLob( clName + "_0", secondaryLocationNodeName,
                ftmask ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        FaultToleranceUtils.insertError( csName, clName + "_0", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_1", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_2", 0 );

        diskFull.restore();
        diskFull.checkRestoreResult();
        diskFull.fini();

        FaultToleranceUtils.checkNodeStatus( secondaryLocationNodeName, "" );

        FaultToleranceUtils.insertError( csName, clName + "_0", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_1", 0 );
        FaultToleranceUtils.insertError( csName, clName + "_2", 0 );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( dataGroup.checkInspect( 1 ) );
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            LocationUtils.cleanLocation( sdb, groupName );
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

    public static void setLocation( Sequoiadb sdb, String groupName,
            String primaryLocation, String secondaryLocation ) {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );

        // primaryLocation设置2个节点
        Node masterNode = group.getMaster();
        masterNode.setLocation( primaryLocation );
        ArrayList< BasicBSONObject > slaveNodes = LocationUtils
                .getGroupSlaveNodes( sdb, groupName );
        String slaveNodeName = "";
        Node slaveNode = null;
        slaveNodeName = slaveNodes.get( 0 ).getString( "hostName" ) + ":"
                + slaveNodes.get( 0 ).getString( "svcName" );
        slaveNode = group.getNode( slaveNodeName );
        slaveNode.setLocation( primaryLocation );

        // secondaryLocation设置1个节点
        slaveNodeName = slaveNodes.get( 1 ).getString( "hostName" ) + ":"
                + slaveNodes.get( 1 ).getString( "svcName" );
        slaveNode = group.getNode( slaveNodeName );
        slaveNode.setLocation( secondaryLocation );
    }
}
