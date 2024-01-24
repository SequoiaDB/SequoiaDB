package com.sequoiadb.location.killnode;

import java.util.ArrayList;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-33451:启动运维模式后剩余节点选举
 * @Author liuli
 * @Date 2023.09.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.09.22
 * @version 1.10
 */
@Test(groups = "location")
public class Location33451 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl = null;
    private String csName = "cs_33451";
    private String clName = "cl_33451";
    private String expandGroupName;
    private int recordNum = 200000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        expandGroupName = SdbTestBase.expandGroupNames.get( 0 );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        dbcl = dbcs.createCollection( clName, option );
    }

    @Test
    public void test() throws ReliabilityException {
        // 半数备节点启动运维模式，并停止启动运维模式的节点
        ArrayList< BasicBSONObject > slaveNodeAddrs = LocationUtils
                .getGroupSlaveNodes( sdb, expandGroupName );
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        int halfSlaveNodeNums = slaveNodeAddrs.size() % 2;
        for ( int i = 0; i <= halfSlaveNodeNums; i++ ) {
            String nodeName = slaveNodeAddrs.get( i ).getString( "hostName" )
                    + ":" + slaveNodeAddrs.get( i ).getString( "svcName" );
            BasicBSONObject options = new BasicBSONObject();
            options.put( "MinKeepTime", 10 );
            options.put( "MaxKeepTime", 20 );
            options.put( "NodeName", nodeName );
            group.startMaintenanceMode( options );
            group.getNode( nodeName ).stop();
        }

        // 再停一个未启动运维模式的备节点，此时停的备节点超过半数
        String nodeName = slaveNodeAddrs.get( halfSlaveNodeNums + 1 )
                .getString( "hostName" ) + ":"
                + slaveNodeAddrs.get( halfSlaveNodeNums + 1 )
                        .getString( "svcName" );
        group.getNode( nodeName ).stop();

        // 集合插入数据
        CommLib.insertData( dbcl, recordNum );

        // group获取主节点
        group.getMaster();

        group.start();
        group.stopMaintenanceMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( expandGroupName ).start();
        sdb.getReplicaGroup( expandGroupName ).stopMaintenanceMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
