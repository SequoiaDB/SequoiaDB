package com.sequoiadb.metadataconsistency.cluster;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10234:concurrency[attachNode, detachNode]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Node10234 extends SdbTestBase {
    private Random random = new Random();
    private static Sequoiadb sdb = null;
    private String rgName = "rg10234";
    private List< String > nodes = new ArrayList<>();
    private int msec = 100;

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode and group number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone, or only one group, skip the testCase." );
        }
        MetaDataUtils.clearGroup( sdb, rgName );

        ReplicaGroup rg = sdb.createReplicaGroup( rgName );
        createNode();
        rg.start();

        nodes = MetaDataUtils.getNodeAddress( sdb, rgName );
    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            this.attachNodeForCleanEnv();
            MetaDataUtils.clearGroup( sdb, rgName );
        } finally {
            sdb.close();
        }
    }

    @Test(invocationCount = 3, threadPoolSize = 3)
    public void test() {

        DetachNode detachNode = new DetachNode();
        detachNode.start();

        AttachNode attachNode = new AttachNode();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        attachNode.start();

        if ( !( detachNode.isSuccess() && attachNode.isSuccess() ) ) {
            Assert.fail( detachNode.getErrorMsg() + attachNode.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkRGOfCatalog( rgName );
    }

    private class DetachNode extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                ReplicaGroup rgDB = db.getReplicaGroup( rgName );
                Node slaveNode = rgDB.getSlave();
                String hostName = slaveNode.getHostName();
                int svcName = slaveNode.getPort();

                rgDB.detachNode( hostName, svcName,
                        new BasicBSONObject( "KeepData", false ) );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -204 // -204:Unable to remove the last node or
                                   // primary in a group
                        && eCode != -155 && eCode != -147 ) { // -155:Node does
                                                              // not exist(node
                                                              // has been to
                                                              // detach)
                    throw e;
                }
            }
        }
    }

    private class AttachNode extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                ReplicaGroup rgDB = db.getReplicaGroup( rgName );
                Node slaveNode = rgDB.getSlave();
                String hostName = slaveNode.getHostName();
                int svcName = slaveNode.getPort();

                rgDB.attachNode( hostName, svcName,
                        new BasicBSONObject( "KeepData", false ) );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -145 && eCode != -147 ) { // -145:Node already
                                                        // exists
                    throw e;
                }
            }
        }
    }

    public void createNode() {
        for ( int i = 0; i < 3; i++ ) {
            MetaDataUtils.createNode( sdb, rgName,
                    SdbTestBase.reservedPortBegin, SdbTestBase.reservedPortEnd,
                    SdbTestBase.reservedDir );
        }
    }

    private void attachNodeForCleanEnv() {
        ReplicaGroup rg = sdb.getReplicaGroup( rgName );
        for ( String nodeInfo : nodes ) {
            String[] info = nodeInfo.split( ":" );
            String hostName = info[ 0 ];
            int port = Integer.parseInt( info[ 1 ] );
            try {
                rg.attachNode( hostName, port,
                        new BasicBSONObject( "KeepData", false ) );
            } catch ( BaseException e ) {
                if ( -145 != e.getErrorCode() ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
            }
        }
    }

}