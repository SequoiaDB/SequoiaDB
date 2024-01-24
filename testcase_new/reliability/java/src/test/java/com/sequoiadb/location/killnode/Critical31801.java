package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import org.bson.types.ObjectId;
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
 * @Description seqDB-31801:灾备中心均异常，节点已经恢复，超过MinKeepTime自动停止 Critical 模式
 * @Author TangTao
 * @Date 2023.06.02
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.02
 */
@Test(groups = "location")
public class Critical31801 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31801";
    private String clName1 = "cl_31801_1";
    private String clName2 = "cl_31801_2";
    private String primaryLocation = "guangzhou.nansha_31801";
    private String sameCityLocation = "guangzhou.panyu_31801";
    private String offsiteLocation = "shenzhen.nanshan_31801";
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

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", -1 );
        option1.put( "ConsistencyStrategy", 2 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );
        ArrayList< BasicBSONObject > otherLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation );

        // 停止同城备中心与异地中心，主中心启动critical模式
        LocationUtils.stopNodeAbnormal( sdb, groupName, sameCityLocationNodes );
        LocationUtils.stopNodeAbnormal( sdb, groupName, otherLocationNodes );

        group.startCriticalMode(
                new BasicBSONObject( "Location", primaryLocation )
                        .append( "MinKeepTime", 2 )
                        .append( "MaxKeepTime", 10 ) );
        Date startTime = new Date();
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 插入数据并校验
        sdb.beginTransaction();
        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        sdb.commit();

        // 集群环境恢复后校验数据
        group.start();
        LocationUtils.validateWaitTime( startTime, 3 );
        LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

        // 再次创建集合并插入数据
        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", -1 );
        option2.put( "ConsistencyStrategy", 2 );
        option2.put( "Group", expandGroupName );
        dbcl2 = sdb.getCollectionSpace( csName ).createCollection( clName2,
                option2 );
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );

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
