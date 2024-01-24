package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;

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
 * @Description seqDB-31800::灾备中心均异常，未超过MinKeepTime，手动停止Critical模式
 * @Author TangTao
 * @Date 2023.05.24
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.24
 */
@Test(groups = "location")
public class Critical31800 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31800";
    private String clName1 = "cl_31800_1";
    private String clName2 = "cl_31800_2";
    private String clName3 = "cl_31800_3";
    private String primaryLocation = "guangzhou.nansha_31800";
    private String sameCityLocation = "guangzhou.panyu_31800";
    private String offsiteLocation = "shenzhen.nanshan_31800";
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
        option1.put( "ReplSize", 0 );
        option1.put( "ConsistencyStrategy", 2 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 2 );
        option2.put( "ConsistencyStrategy", 2 );
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );

        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "ReplSize", 3 );
        option3.put( "ConsistencyStrategy", 2 );
        option3.put( "Group", expandGroupName );
        dbcl3 = dbcs.createCollection( clName3, option3 );
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
                        .append( "MinKeepTime", 10 )
                        .append( "MaxKeepTime", 20 ) );
        Date startTime = new Date();
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 插入数据并校验
        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );
        CommLib.checkRecords( dbcl3, batchRecords3, orderBy );

        // 集群环境恢复后校验数据
        group.start();
        LocationUtils.validateWaitTime( startTime, 3 );
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );
        group.stopCriticalMode();

        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );

        // 插入Lob并校验
        int lobtimes = 1;
        LinkedBlockingQueue< LocationUtils.SaveOidAndMd5 > id2md5 = LocationUtils
                .writeLobAndGetMd5( dbcl1, lobtimes );
        List< LinkedBlockingQueue< LocationUtils.SaveOidAndMd5 > > id2md5List = new ArrayList<>();
        id2md5List.add( id2md5 );
        LocationUtils.ReadLob( dbcl1, id2md5List.get( 0 ) );
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
