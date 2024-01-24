package com.sequoiadb.clustermanager;

import java.util.Date;

import org.bson.BSONObject;
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
 * @TestLink: seqDB-7072
 * @describe:create the same node then check error message
 * @author zhaoyu
 * @Date 2016.10.9
 * @version 1.00
 */

public class ClusterManager7072 extends SdbTestBase {
    private Sequoiadb sdb;
    private String dataRGName = "dataAddGroup7072";
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
            if ( sdb.getReplicaGroup( dataRGName ) != null ) {
                sdb.removeReplicaGroup( dataRGName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // set data node configure
        int dataPortAdd1 = reservedPortBegin + 720;
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

        // create data node
        try {
            dataRGAdd.createNode( coordIP, dataPortAdd1, dataPathAdd1,
                    dataConfigue );
            sdb.activateReplicaGroup( dataRGName );
        } catch ( BaseException e ) {
            Assert.fail( "create data Node or activateReplicaGroup failed"
                    + e.getMessage() );
        }

        // create the same node repeat and check result
        try {
            dataRGAdd.createNode( coordIP, dataPortAdd1, dataPathAdd1,
                    dataConfigue );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            if ( -145 != e.getErrorCode() && -157 != e.getErrorCode() ) {
                Assert.assertTrue( false,
                        "create node, errMsg:" + e.getMessage() );
            }
        }
    }
}
