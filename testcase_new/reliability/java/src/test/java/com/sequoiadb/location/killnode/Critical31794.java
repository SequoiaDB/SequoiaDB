package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
 * @Description seqDB-31794::同城主备中心同时异常，超过MinKeepTime后节点恢复，自动停止 Critical 模式
 * @Author TangTao
 * @Date 2023.05.23
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.23
 */
@Test(groups = "location")
public class Critical31794 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31794";
    private String clName1 = "cl_31794_1";
    private String clName2 = "cl_31794_2";
    private String clName3 = "cl_31794_3";
    private String clName4 = "cl_31794_4";
    private String primaryLocation = "guangzhou.nansha_31794";
    private String sameCityLocation = "guangzhou.panyu_31794";
    private String offsiteLocation = "shenzhen.nanshan_31794";
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
        option1.put( "ConsistencyStrategy", 3 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 2 );
        option2.put( "ConsistencyStrategy", 3 );
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );

        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "ReplSize", 3 );
        option3.put( "ConsistencyStrategy", 3 );
        option3.put( "Group", expandGroupName );
        dbcl3 = dbcs.createCollection( clName3, option3 );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );
        ArrayList< BasicBSONObject > offsiteLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation );

        // 主中心、同城备中心异常停止，然后stop节点模拟故障无法启动
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );
        LocationUtils.stopNodeAbnormal( sdb, groupName, sameCityLocationNodes );

        // 异地中心启动critical模式
        group.startCriticalMode(
                new BasicBSONObject( "Location", offsiteLocation )
                        .append( "MinKeepTime", 2 )
                        .append( "MaxKeepTime", 10 ) );
        Date startTime = new Date();

        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 插入数据并校验
        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        checkRecordSyncNum( csName, clName1, recordNum, offsiteLocationNodes,
                2 );
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        checkRecordSyncNum( csName, clName2, recordNum, offsiteLocationNodes,
                2 );
        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3,
                recordNum );
        checkRecordSyncNum( csName, clName3, recordNum, offsiteLocationNodes,
                2 );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );
        CommLib.checkRecords( dbcl3, batchRecords3, orderBy );

        // 集群环境恢复，超过MinKeepTime后节点恢复，自动停止 Critical 模式
        group.start();
        LocationUtils.validateWaitTime( startTime, 3 );
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

        BasicBSONObject option4 = new BasicBSONObject();
        option4.put( "ReplSize", 0 );
        option4.put( "ConsistencyStrategy", 3 );
        option4.put( "Group", expandGroupName );
        DBCollection dbcl4 = sdb.getCollectionSpace( csName )
                .createCollection( clName4, option4 );
        List< BSONObject > batchRecords4 = CommLib.insertData( dbcl4,
                recordNum );
        CommLib.checkRecords( dbcl4, batchRecords4, orderBy );
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

    private static void checkRecordSyncNum( String csName, String clName,
            int recordNum, ArrayList< BasicBSONObject > nodeAddrs, int num ) {
        DBCollection dbcl = null;
        int count = 0;
        for ( BasicBSONObject nodeAddr : nodeAddrs ) {
            String nodeName = nodeAddr.get( "hostName" ) + ":"
                    + nodeAddr.get( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeName, "", "" )) {
                dbcl = data.getCollectionSpace( csName )
                        .getCollection( clName );
                if ( ( int ) dbcl.getCount() == recordNum ) {
                    count++;
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }

        if ( count < num ) {
            Assert.fail( "expect at least " + num + " node to sync, act : "
                    + count );
        }
    }
}
