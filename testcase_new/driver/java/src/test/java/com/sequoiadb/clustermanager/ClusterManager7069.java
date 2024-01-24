package com.sequoiadb.clustermanager;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.clustermanager.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-7069
 * @describe:detach and attach node use interfaces as follow: 1.detachNode()
 *                  2.attachNode()
 * @author zhaoyu
 * @Date 2016.10.8
 * @version 1.00
 */

public class ClusterManager7069 extends SdbTestBase {
    private Sequoiadb sdb;
    private String dataRGName1 = "dataAddGroup70691";
    private String dataRGName2 = "dataAddGroup70692";
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
            sdb.removeReplicaGroup( dataRGName1 );
            sdb.removeReplicaGroup( dataRGName2 );

            sdb.close();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // set node configure
        int dataPortAdd1 = reservedPortBegin + 690;
        String dataPathAdd1 = reservedDir + "/" + dataPortAdd1 + "/";
        BSONObject dataConfigue = ( BSONObject ) JSON
                .parse( "{KeepData:true}" );

        int dataPortAdd2 = reservedPortBegin + 700;
        String dataPathAdd2 = reservedDir + "/" + dataPortAdd2 + "/";
        BSONObject dataConfigue1 = ( BSONObject ) JSON
                .parse( "{KeepData:true}" );

        ReplicaGroup dataRGAdd1 = null;
        ReplicaGroup dataRGAdd2 = null;

        // create data RG1
        try {
            sdb.getReplicaGroup( dataRGName1 );
        } catch ( BaseException e ) {
            if ( -154 != e.getErrorCode() ) {
                sdb.removeReplicaGroup( dataRGName1 );
            }
        }
        dataRGAdd1 = sdb.createReplicaGroup( dataRGName1 );

        // create data node
        dataRGAdd1.createNode( coordIP, dataPortAdd1, dataPathAdd1,
                dataConfigue );
        dataRGAdd1.createNode( coordIP, dataPortAdd2, dataPathAdd2,
                dataConfigue );
        sdb.activateReplicaGroup( dataRGName1 );

        // create data RG2 for backup
        try {
            sdb.getReplicaGroup( dataRGName2 );
        } catch ( BaseException e ) {
            if ( -154 != e.getErrorCode() ) {
                sdb.removeReplicaGroup( dataRGName2 );
            }
        }
        dataRGAdd2 = sdb.createReplicaGroup( dataRGName2 );

        // detach node from RG1
        try {
            dataRGAdd1.detachNode( coordIP, dataPortAdd1, dataConfigue );
        } catch ( BaseException e ) {
            Assert.fail(
                    "detachNode " + dataPortAdd1 + " failed" + e.getMessage() );
        }
        try {
            dataRGAdd1.getNode( coordIP, dataPortAdd1 );
            Assert.fail( "the detach node is exists!" );
        } catch ( BaseException e ) {
            if ( -155 != e.getErrorCode() ) {
                Assert.fail( "detachNode fail!" );
            }
        }

        // attach node to RG2
        try {
            dataRGAdd2.attachNode( coordIP, dataPortAdd1, dataConfigue1 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "attachNode " + dataPortAdd1 + " failed" + e.getMessage() );
        }
        Assert.assertNotNull(
                dataRGAdd2.getNode( coordIP + ":" + dataPortAdd1 ),
                "node " + dataPortAdd1 + " not exists in " + dataRGName2
                        + " ,but expect result is attach!" );

        // RG operate
        try {
            dataRGAdd1.stop();
            dataRGAdd1.start();
        } catch ( BaseException e ) {
            Assert.fail( "dataRG stop or start failed" + e.getMessage() );
        }
    }
}
