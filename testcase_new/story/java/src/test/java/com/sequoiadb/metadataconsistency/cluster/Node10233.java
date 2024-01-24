package com.sequoiadb.metadataconsistency.cluster;

import java.util.Random;

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
 * TestLink: seqDB-10233:concurrency[createNode, removeNode]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.25
 */

public class Node10233 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String rgName = "rg10233";
    private Random random = new Random();
    private int msec = 1000;

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

        sdb.createReplicaGroup( rgName );
        for ( int i = 0; i < 2; i++ ) {
            MetaDataUtils.createNode( sdb, rgName,
                    SdbTestBase.reservedPortBegin, SdbTestBase.reservedPortEnd,
                    SdbTestBase.reservedDir );
        }
        ReplicaGroup rgDB = sdb.getReplicaGroup( rgName );
        rgDB.start();

    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            MetaDataUtils.clearGroup( sdb, rgName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        CreateNode createNode = new CreateNode();
        createNode.start();

        RemoveNode removeNode = new RemoveNode();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        removeNode.start();

        if ( !( createNode.isSuccess() && removeNode.isSuccess() ) ) {
            Assert.fail( createNode.getErrorMsg() + removeNode.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkRGOfCatalog( rgName );
    }

    private class CreateNode extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                MetaDataUtils.createNode( db, rgName,
                        SdbTestBase.reservedPortBegin,
                        SdbTestBase.reservedPortEnd, SdbTestBase.reservedDir );
                ReplicaGroup rgDB = db.getReplicaGroup( rgName );
                rgDB.start();
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -156 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class RemoveNode extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                ReplicaGroup rgDB = db.getReplicaGroup( rgName );

                Node node = rgDB.getSlave();
                String hostName = node.getHostName();
                int svcName = node.getPort();
                rgDB.removeNode( hostName, svcName, null );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -155 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

}