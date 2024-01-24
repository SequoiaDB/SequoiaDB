package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-32234:Location亲和性测试
 * @Author liuli
 * @Date 2023.06.19
 * @UpdateAuthor liuli
 * @UpdateDate 2023.06.19
 * @version 1.10
 */
@Test(groups = "location")
public class Location32234 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private String csName = "cs_32234";
    private String clName = "cl_32234";
    private List< BSONObject > batchRecords;
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

        BasicBSONObject option = new BasicBSONObject();
        option.put( "ReplSize", 0 );
        option.put( "Group", SdbTestBase.expandGroupNames.get( 0 ) );
        dbcs.createCollection( clName, option );
    }

    @DataProvider(name = "setLocation")
    public Object[][] configs() {
        return new Object[][] {
                { "guangzhou", "guangzhou.nansha", "shenzhan.nanshan" },
                { "guangzhou.panyu", "guangzhou.nansha.jiaomen",
                        "shenzhan.nanshan" },
                { "GUANGZHOU.panyu", "guangzhou.nansha", "shenzhan.nanshan" },
                { "GuangZhou.panyu", "guangzhou.nansha", "shenzhan.nanshan" },
                { "GuangZhou.panyu", "Guangzhou.nansha", "shenzhan.nanshan" },
                { "GuangZhou.nansha", "Guangzhou.nansha",
                        "shenzhan.nanshan" } };
    }

    @Test(dataProvider = "setLocation")
    public void test( String primaryLocation, String sameCityLocation,
            String offsiteLocation ) throws ReliabilityException {
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        String groupName = SdbTestBase.expandGroupNames.get( 0 );
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        group.setActiveLocation( primaryLocation );

        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        batchRecords = CommLib.insertData( dbcl, recordNum );

        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止 PrimaryLocation 中的所有节点
        for ( BasicBSONObject primaryLocationSlaveNode : primaryLocationSlaveNodes ) {
            String nodeName = primaryLocationSlaveNode.getString( "hostName" )
                    + ":" + primaryLocationSlaveNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 等待选出主节点
        int timeOut = 120;
        CommLib.waitGroupSelectPrimaryNode( sdb, groupName, timeOut );

        // 获取主节点
        String newMasterNodeName = group.getMaster().getNodeName();

        // 校验新主节点在同城备中心
        ArrayList< String > sameCityLocationNodeNames = new ArrayList<>();
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            sameCityLocationNodeNames.add( nodeName );
        }

        Assert.assertTrue(
                sameCityLocationNodeNames.contains( newMasterNodeName ),
                "new master node is not in same city location" );

        group.start();

        // 集群环境恢复后校验数据
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb,
                SdbTestBase.expandGroupNames.get( 0 ) );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
