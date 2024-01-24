package com.sequoiadb.clustermanager;

import java.util.Date;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.Node.NodeStatus;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-7071
 * @describe:query node information use interfaces as follow: 1.getNodeId()
 *                 2.getHostName() 3.getNodeName(); 4.getPort();
 *                 5.getReplicaGroup(); 6.getSdb(); 7.getStatus();
 * @author zhaoyu
 * @Date 2016.10.8
 * @version 1.00
 */

public class ClusterManager7071 extends SdbTestBase {
    private Sequoiadb sdb;
    private String dataRGName = "dataAddGroup7071";
    private String coordAddr;
    private String reservedDir;
    private int reservedPortBegin;
    private String coordIP;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.reservedDir = SdbTestBase.reservedDir;
        this.reservedPortBegin = SdbTestBase.reservedPortBegin;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }
            // get hostname
            coordIP = sdb.getReplicaGroup( "SYSCatalogGroup" ).getMaster()
                    .getHostName();
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.removeReplicaGroup( dataRGName );
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // set node configure
        int dataPortAdd1 = reservedPortBegin + 710;
        String dataPathAdd1 = reservedDir + "/" + dataPortAdd1 + "/";
        BSONObject dataConfigue = null;

        // create data groups
        ReplicaGroup dataRGAdd = null;
        try {
            sdb.getReplicaGroup( dataRGName );
        } catch ( BaseException e ) {
            if ( -154 != e.getErrorCode() ) {
                sdb.removeReplicaGroup( dataRGName );
            }
        }
        dataRGAdd = sdb.createReplicaGroup( dataRGName );
        Assert.assertTrue( sdb.isReplicaGroupExist( dataRGName ) );
        int dataRGId = dataRGAdd.getId();
        Assert.assertTrue( sdb.isReplicaGroupExist( dataRGId ) );

        // create data node
        Node data = null;
        try {
            data = dataRGAdd.createNode( coordIP, dataPortAdd1, dataPathAdd1,
                    dataConfigue );
            sdb.activateReplicaGroup( dataRGName );
        } catch ( BaseException e ) {
            Assert.fail( "create data Node or activateReplicaGroup failed"
                    + e.getMessage() );
        }

        int nodeId = 0;
        String dataHostName = null;
        String nodeName = null;
        int port = 0;
        ReplicaGroup dataReplicaGroup = null;
        // Sequoiadb sdb1 = null;
        com.sequoiadb.base.Node.NodeStatus status = null;
        try {
            nodeId = data.getNodeId();
            dataHostName = data.getHostName();
            nodeName = data.getNodeName();
            port = data.getPort();
            dataReplicaGroup = data.getReplicaGroup();
            // sdb1 = data.getSdb();
            status = data.getStatus();
        } catch ( BaseException e ) {
            Assert.fail( "get node configure failed" + e.getMessage() );
        }
        // System.out.println("sdb1:"+sdb1);
        Assert.assertNotEquals( nodeId, 0 );
        // SEQUOIADBMAINSTREAM-2003
        // Assert.assertEquals(dataHostName, coordIP);
        // Assert.assertEquals(nodeName, coordIP + ":" + dataPortAdd1);
        Assert.assertEquals( port, dataPortAdd1 );
        Assert.assertEquals( dataReplicaGroup.getGroupName(), dataRGName );
        // Assert.assertEquals(sdb1, sdb);
        Assert.assertEquals( status, NodeStatus.SDB_NODE_ACTIVE );

        Assert.assertTrue( dataRGAdd.isNodeExist( nodeName ) );
        Assert.assertTrue( dataRGAdd.isNodeExist( dataHostName, port ) );
    }
}
