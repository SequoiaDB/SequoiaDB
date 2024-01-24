package com.sequoiadb.clustermanager;

import java.util.Date;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.clustermanager.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @author zhaoyu
 * @version 1.00
 * @TestLink: seqDB-7065/seqDB-7066/seqDB-7068
 * @describe:use interfaces as follow: 1.createReplicaGroup() 2.createNode()
 *               3.activateReplicaGroup() 4.getReplicaGroup() 5.getDetail()
 *               6.listReplicaGroups() 7.getReplicaGroupsInfo() 8.stop()
 *               9.start() 10.getMaster() 11.getSlave() 12.removeNode()
 *               13.removeReplicaGroup()
 * @Date 2016.10.8
 */

public class ClusterManager7065 extends SdbTestBase {
    private Sequoiadb sdb;
    private String dataRGName = "dataAddGroup7065";
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
            try {
                sdb.getReplicaGroup( dataRGName );
            } catch ( BaseException e ) {
                if ( -154 != e.getErrorCode() ) {
                    sdb.removeReplicaGroup( dataRGName );
                }
            }

            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // set node configure
        int dataPortAdd1 = reservedPortBegin + 650;
        String dataPathAdd1 = reservedDir + "/" + dataPortAdd1 + "/";
        dataPathAdd1 = dataPathAdd1.replaceAll( "/+", "/" );
        BSONObject dataConfigue1 = ( BSONObject ) JSON
                .parse( "{logfilesz:64}" );

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
                    dataConfigue1 );
            sdb.activateReplicaGroup( dataRGName );
        } catch ( BaseException e ) {
            Assert.fail( "create data Node or activateReplicaGroup failed"
                    + e.getMessage() );
        }

        // check configure
        ReplicaGroup dataRG = null;
        String groupName = null;
        int groupStatus = 0;
        String hostName = null;
        int nodeStatus = 0;
        String dbpath = null;
        int service = 0;
        try {
            dataRG = sdb.getReplicaGroup( dataRGName );
            BSONObject dataRGDetail = dataRG.getDetail();
            groupName = ( String ) dataRGDetail.get( "GroupName" );
            groupStatus = ( int ) dataRGDetail.get( "Status" );
            BasicBSONList group = ( BasicBSONList ) dataRGDetail.get( "Group" );
            BasicBSONObject groupObj = ( BasicBSONObject ) group.get( 0 );
            hostName = ( String ) groupObj.get( "HostName" );
            nodeStatus = ( int ) groupObj.get( "Status" );
            dbpath = ( String ) groupObj.get( "dbpath" );
            BasicBSONList serviceList = ( BasicBSONList ) groupObj
                    .get( "Service" );
            BasicBSONObject serviceObj = ( BasicBSONObject ) serviceList
                    .get( 0 );
            service = Integer.parseInt( ( String ) serviceObj.get( "Name" ) );
        } catch ( BaseException e ) {
            Assert.fail(
                    "get replica group configure failed" + e.getMessage() );
        }

        Assert.assertEquals( groupName, dataRGName );
        Assert.assertEquals( groupStatus, 1 );
        Assert.assertEquals( hostName, coordIP );
        Assert.assertEquals( nodeStatus, 1 );
        Assert.assertEquals( dbpath, dataPathAdd1 );
        Assert.assertEquals( service, dataPortAdd1 );

        // check result use listReplicaGroups
        String groupName1 = null;
        int groupStatus1 = 0;
        String hostName1 = null;
        int nodeStatus1 = 0;
        String dbpath1 = null;
        int service1 = 0;
        try {
            DBCursor replicaCursor = sdb.listReplicaGroups();
            while ( replicaCursor.hasNext() ) {
                BSONObject replicaRecord = replicaCursor.getNext();
                groupName1 = ( String ) replicaRecord.get( "GroupName" );
                groupStatus1 = ( int ) replicaRecord.get( "Status" );
                if ( groupName1.equals( dataRGName ) ) {
                    BasicBSONList group = ( BasicBSONList ) replicaRecord
                            .get( "Group" );
                    BasicBSONObject groupObj = ( BasicBSONObject ) group
                            .get( 0 );
                    hostName1 = ( String ) groupObj.get( "HostName" );
                    nodeStatus1 = ( int ) groupObj.get( "Status" );
                    dbpath1 = ( String ) groupObj.get( "dbpath" );
                    BasicBSONList serviceList1 = ( BasicBSONList ) groupObj
                            .get( "Service" );
                    BasicBSONObject serviceObj1 = ( BasicBSONObject ) serviceList1
                            .get( 0 );
                    service1 = Integer
                            .parseInt( ( String ) serviceObj1.get( "Name" ) );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "listReplicaGroups failed" + e.getMessage() );
        }
        Assert.assertEquals( groupName1, dataRGName );
        Assert.assertEquals( groupStatus1, 1 );
        Assert.assertEquals( hostName1, coordIP );
        Assert.assertEquals( nodeStatus1, 1 );
        Assert.assertEquals( dbpath1, dataPathAdd1 );
        Assert.assertEquals( service1, dataPortAdd1 );

        // check result use getReplicaGroupsInfo
        String getHostName = null;
        try {
            List< String > replicaGroupsInfo = sdb.getReplicaGroupsInfo();
            int replicaGroupsSize = replicaGroupsInfo.size();
            String group = replicaGroupsInfo.get( replicaGroupsSize - 1 );
            BSONObject groupObj = ( BSONObject ) JSON.parse( group );
            BasicBSONList getlist = ( BasicBSONList ) groupObj.get( "Group" );
            BasicBSONObject HostNameObj = ( BasicBSONObject ) getlist.get( 0 );
            getHostName = ( String ) HostNameObj.get( "HostName" );
        } catch ( BaseException e ) {
            Assert.fail( "getReplicaGroupsInfo failed" + e.getMessage() );
        }
        Assert.assertEquals( getHostName, coordIP );

        // RG operate
        try {
            dataRG.stop();
            dataRG.start();
        } catch ( BaseException e ) {
            Assert.fail( "dataRG stop or start failed" + e.getMessage() );
        }

        // node operate
        try {
            Node node = sdb.getReplicaGroup( dataRGName )
                    .getNode( coordIP + ":" + dataPortAdd1 );
            node.stop();
            node.start();
        } catch ( BaseException e ) {
            Assert.fail( "node stop or start failed" + e.getMessage() );
        }

        // create another data node
        int dataPortAdd2 = reservedPortBegin + 660;
        String dataPathAdd2 = reservedDir + "/" + dataPortAdd2 + "/";
        BSONObject dataConfigue2 = null;
        try {
            Node node = dataRGAdd.createNode( coordIP, dataPortAdd2,
                    dataPathAdd2, dataConfigue2 );
            node.start();

        } catch ( BaseException e ) {
            Assert.fail(
                    "create or start data Node 2 failed" + e.getMessage() );
        }

        // get master and slave
        String actualMasterNodeName;
        String actualSlaveNodeName;

        try {
            for ( int i = 0; i < 120; i++ ) {
                try {
                    dataRG.getMaster();
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() == -71 ) {
                        try {
                            System.out.println( "get master time: " + i );
                            Thread.sleep( 3000 );
                        } catch ( InterruptedException e1 ) {
                            e1.printStackTrace();
                        }
                    } else {
                        Assert.fail( "get master fail:" + e.getMessage() );
                    }
                }
            }

            dataRG = sdb.getReplicaGroup( dataRGName );
            Assert.assertEquals( isPrimary( dataRG ), true );
        } catch ( BaseException e ) {
            Assert.fail( "get master and slave node failed" + e.getMessage() );
        }

        // remove node
        int removePort = dataRG.getSlave().getPort();
        dataRG.removeNode( coordIP, removePort, null );
        try {
            dataRG.getNode( coordIP, removePort );
            Assert.fail( "the node exists|" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -155 ) {
                Assert.fail( "node " + removePort
                        + " exists ,but expect result is removed!" );
            }
        }

        // remove replicaGroup
        sdb.removeReplicaGroup( dataRGName );
        try {
            sdb.getReplicaGroup( dataRGName );
            Assert.fail( "the remove group is exists!" );
        } catch ( BaseException e ) {
            if ( -154 != e.getErrorCode() ) {
                Assert.fail( "the remove group fail!" );
            }
        }

    }

    private boolean isPrimary( ReplicaGroup group ) {
        for ( int i = 0; i < 50; i++ ) {
            Sequoiadb db = null;
            try {
                Node master = group.getMaster();
                db = master.connect();
                DBCursor cursor = db.getSnapshot( 6, "", "", "" );
                BSONObject object = cursor.getNext();
                boolean isPrimary = ( Boolean ) object.get( "IsPrimary" );
                cursor.close();
                if ( isPrimary )
                    return true;
                else
                    Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }
        // should never come here!
        return false;
    }
}