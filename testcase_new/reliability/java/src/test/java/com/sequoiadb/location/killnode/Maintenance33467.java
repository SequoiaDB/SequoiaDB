package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

/**
 * @version 1.0
 * @Description seqDB-33467:同城主中心异常，启动运维模式
 * @Author TangTao
 * @Date 2023.09.25
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.09.25
 */
@Test(groups = "location")
public class Maintenance33467 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_33467";
    private String clName = "cl_33467";
    private String primaryLocation = "guangzhou.nansha_33467";
    private String sameCityLocation = "guangzhou.panyu_33467";
    private String offsiteLocation = "shenzhen.nanshan_33467";
    private int recordNum = 10000;

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
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
    }

    @Test
    public void test() throws ReliabilityException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );

        // 停止主中心节点，开启运维模式
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );
        group.startMaintenanceMode(
                new BasicBSONObject( "Location", primaryLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );

        LocationUtils.checkGroupInMaintenanceMode( sdb, groupName );

        // 插入数据并校验
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 0 );
        option1.put( "Group", expandGroupName );
        dbcl = dbcs.createCollection( clName, option1 );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        ReplicaGroup group = sdb
                .getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) );
        group.start();
        group.stopMaintenanceMode();
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
