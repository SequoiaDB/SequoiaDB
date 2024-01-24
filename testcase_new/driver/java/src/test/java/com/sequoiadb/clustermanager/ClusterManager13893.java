package com.sequoiadb.clustermanager;

import java.util.ArrayList;
import java.util.Date;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-13893
 * @describe:use interfaces as follow: 1. getReplicaGroup(String rgName) 2.
 *               getReplicaGroup(int rgId) 3.getNode(String nodeName)
 *               4.getNode(String hostName, int port)
 * @author wuyan
 * @Date 2017.12.25
 * @version 1.00
 */
public class ClusterManager13893 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );

            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }

        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // getRGName,the RGname is not exist
        try {
            String RGName = "test13893";
            Assert.assertFalse( sdb.isReplicaGroupExist( RGName ) );
            sdb.getReplicaGroup( RGName );
            Assert.fail( "get RG should be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -154 ) {
                Assert.fail( "group does not exist is error -154,but e ="
                        + e.getErrorCode() );
            }
        }

        // getRGid,the RGid is not exist
        try {
            int RGid = 13893;
            Assert.assertFalse( sdb.isReplicaGroupExist( RGid ) );
            sdb.getReplicaGroup( RGid );
            Assert.fail( "get RG by RGid should be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -154 ) {
                Assert.fail( "group does not exist is error -154,but e ="
                        + e.getErrorCode() );
            }
        }

        // getNodeName,the NodeName is not exist
        ArrayList< String > dataGroupNames = commlib.getDataGroupNames( sdb );
        ReplicaGroup rGroup = sdb.getReplicaGroup( dataGroupNames.get( 0 ) );
        try {
            String nodeName = "localhost:13893";
            Assert.assertFalse( rGroup.isNodeExist( nodeName ) );
            rGroup.getNode( nodeName );
            Assert.fail( "get nodeName by RG should be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -155 ) {
                Assert.fail( "node does not exist is error -155,but e ="
                        + e.getErrorCode() );
            }
        }

        // getNode by hostname adn port, the node is not exist
        try {
            String hostName = rGroup.getMaster().getHostName();
            int port = 13893;
            Assert.assertFalse( rGroup.isNodeExist( hostName, port ) );
            rGroup.getNode( hostName, port );
            Assert.fail( "get nodeName by hostname and port should be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -155 ) {
                Assert.fail( "node does not exist is error -155,but e ="
                        + e.getErrorCode() );
            }
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

}
