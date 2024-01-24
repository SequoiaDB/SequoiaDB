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
 * @Description seqDB-31816:主中心开启Critical模式，保持同步一致性
 * @Author TangTao
 * @Date 2023.05.24
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.24
 */
@Test(groups = "location")
public class Critical31816 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private DBCollection dbcl4 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31816";
    private String clName1 = "cl_31816_1";
    private String clName2 = "cl_31816_2";
    private String clName3 = "cl_31816_3";
    private String clName4 = "cl_31816_4";
    private String primaryLocation = "guangzhou.nansha_31816";
    private String sameCityLocation = "guangzhou.panyu_31816";
    private String offsiteLocation = "shenzhen.nanshan_31816";
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
    public void test() throws ReliabilityException, InterruptedException {
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );

        // 主中心启动critical模式
        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }
        group.startCriticalMode(
                new BasicBSONObject( "Location", primaryLocation )
                        .append( "MinKeepTime", 5 )
                        .append( "MaxKeepTime", 10 ) );
        Date startTime = new Date();
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        int syncNum = 0;
        // 优先同步到PrimaryLocation中的一个备节点
        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 2 );
        option1.put( "ConsistencyStrategy", 2 );
        option1.put( "Group", groupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        syncNum = LocationUtils.countRecordSyncNum( csName, clName1, recordNum,
                primaryLocationNodes );
        Assert.assertTrue( syncNum >= 1, "syncNum is " + syncNum + " < 1" );

        // 优先同步到PrimaryLocation中的一个备节点
        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 2 );
        option2.put( "ConsistencyStrategy", 3 );
        option2.put( "Group", groupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );

        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        syncNum = LocationUtils.countRecordSyncNum( csName, clName2, recordNum,
                primaryLocationNodes );
        Assert.assertTrue( syncNum >= 1, "syncNum is " + syncNum + " < 1" );

        // 优先同步到PrimaryLocation中的备节点
        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "ReplSize", 3 );
        option3.put( "ConsistencyStrategy", 2 );
        option3.put( "Group", groupName );
        dbcl3 = dbcs.createCollection( clName3, option3 );

        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3,
                recordNum );
        syncNum = LocationUtils.countRecordSyncNum( csName, clName3, recordNum,
                primaryLocationNodes );
        Assert.assertTrue( syncNum >= 2, "syncNum is " + syncNum + " < 2" );

        // 优先同步到PrimaryLocation中的备节点
        BasicBSONObject option4 = new BasicBSONObject();
        option4.put( "ReplSize", 3 );
        option4.put( "ConsistencyStrategy", 3 );
        option4.put( "Group", groupName );
        dbcl4 = dbcs.createCollection( clName4, option4 );

        List< BSONObject > batchRecords4 = CommLib.insertData( dbcl4,
                recordNum );
        syncNum = LocationUtils.countRecordSyncNum( csName, clName4, recordNum,
                primaryLocationNodes );
        Assert.assertTrue( syncNum >= 2, "syncNum is " + syncNum + " < 2" );

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );
        CommLib.checkRecords( dbcl3, batchRecords3, orderBy );
        CommLib.checkRecords( dbcl4, batchRecords4, orderBy );

        // 集群环境恢复
        group.stopCriticalMode();
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
