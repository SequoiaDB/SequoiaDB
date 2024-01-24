package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-33456:备中心启动运维模式，主中心备节点故障
 * @Author liuli
 * @Date 2023.09.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.09.22
 * @version 1.10
 */
@Test(groups = "location")
public class Location33456 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private CollectionSpace dbcs = null;
    private String csName = "cs_33456";
    private String clName = "cl_33456";
    private String primaryLocation = "guangzhou.nansha_33456";
    private String sameCityLocation = "guangzhou.panyu_33456";
    private String offsiteLocation = "shenzhan.nanshan_33456";
    private String expandGroupName;
    private int recordNum = 1000;

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

        expandGroupName = SdbTestBase.expandGroupNames.get( 0 );
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        if ( !CommLib.isLSNConsistency( sdb, expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        dbcs = sdb.createCollectionSpace( csName );
    }

    @Test
    public void test() throws ReliabilityException {
        // 指定同城备中启动运维模式
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", 10 );
        options.put( "MaxKeepTime", 20 );
        options.put( "Location", sameCityLocation );
        group.startMaintenanceMode( options );

        // 停止主中心备节点
        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, expandGroupName,
                        primaryLocation );
        for ( BasicBSONObject primaryLocationSlaveNode : primaryLocationSlaveNodes ) {
            String nodeName = primaryLocationSlaveNode.getString( "hostName" )
                    + ":" + primaryLocationSlaveNode.getString( "svcName" );
            group.getNode( nodeName ).stop();
        }

        // 创建集合并插入数据
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        DBCollection dbcl = dbcs.createCollection( clName, option );

        // 集合插入数据
        List< BSONObject > batchRecords = CommLib.insertData( dbcl, recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );

        // 解除运维模式，节点未恢复
        group.stopMaintenanceMode();
        group.start();
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
