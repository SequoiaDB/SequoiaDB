package com.sequoiadb.clustermanager;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.clustermanager.CommLib;
import com.sequoiadb.net.ServerAddress;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-7064/seqDB-7070
 * @describe:use interfaces as follow: 1.getReplicaGroup() 2.getNode()
 *               3.getGroupName() 4.getReplicaGroupNames() 5.connect()
 *               6.disconnect()
 * @author zhaoyu
 * @Date 2016.10.8
 * @version 1.00
 */
public class ClusterManager7064 extends SdbTestBase {
    private Sequoiadb sdb;
    private String cataGroupName = "SYSCatalogGroup";
    private String coordIP;
    private String coordAddr;
    private String reservedDir;
    private int reservedPortBegin;
    private int cataPortAdd;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.reservedDir = SdbTestBase.reservedDir;
        this.reservedPortBegin = SdbTestBase.reservedPortBegin;
        // this.coordIP = SdbTestBase.hostName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }
            // get hostname
            coordIP = sdb.getReplicaGroup( cataGroupName ).getMaster()
                    .getHostName();
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            ReplicaGroup cataGroup = sdb.getReplicaGroup( cataGroupName );
            if ( null != cataGroup.getNode( coordIP, cataPortAdd ) ) {
                cataGroup.removeNode( coordIP, cataPortAdd, null );
                sdb.msg( "remove  catalog node " + cataPortAdd + " success." );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // create cata Node
        cataPortAdd = reservedPortBegin + 640;
        String cataPathAdd = reservedDir + "/" + cataPortAdd + "/";
        BSONObject cataConfigue = null;

        ReplicaGroup cataGroup = null;
        try {
            cataGroup = sdb.getReplicaGroup( cataGroupName );
            int rgId = cataGroup.getId();
            cataGroup = sdb.getReplicaGroup( rgId );
        } catch ( BaseException e ) {
            Assert.fail( "getReplicaGroup failed" + e.getMessage() );
        }

        Node cataNode = null;
        try {
            cataGroup.getNode( coordIP, cataPortAdd );
        } catch ( BaseException e ) {
            if ( -155 != e.getErrorCode() ) {
                cataGroup.removeNode( coordIP, cataPortAdd, null );
            }
        }
        cataNode = cataGroup.createNode( coordIP, cataPortAdd, cataPathAdd,
                cataConfigue );
        cataNode.start();
        sdb.msg( "create catalog node " + cataPortAdd + " success." );

        // check cata group name
        String actualCataGroupName = cataGroup.getGroupName();
        Assert.assertEquals( actualCataGroupName, cataGroupName );

        // check group names
        ArrayList< String > groupNames = new ArrayList< String >();
        try {
            groupNames = sdb.getReplicaGroupNames();
        } catch ( BaseException e ) {
            Assert.fail( "getGroupNames failed" + e.getMessage() );
        }
        if ( !groupNames.contains( cataGroupName ) ) {
            Assert.fail( groupNames + "is not contains " + cataGroupName );
        }

        // seqDB-7070
        Sequoiadb cata = null;
        try {
            cata = cataNode.connect();
        } finally {
            if ( cata != null ) {
                cata.disconnect();
            }

        }
    }
}
