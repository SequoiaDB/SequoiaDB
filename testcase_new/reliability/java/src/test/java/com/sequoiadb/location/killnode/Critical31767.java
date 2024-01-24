package com.sequoiadb.location.killnode;

import java.util.ArrayList;
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
 * @Description seqDB-31767:主中心异常停止，备中心单节点启动Critical模式
 *              seqDB-32334:Java驱动startCriticalMode参数校验
 * @Author TangTao
 * @Date 2023.05.26
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.26
 */
@Test(groups = "location")
public class Critical31767 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31767";
    private String clName = "cl_31767";
    private String primaryLocation = "guangzhou.nansha_31767";
    private String sameCityLocation = "guangzhou.panyu_31767";
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
        LocationUtils.setTwoLocationInSameCity( sdb, expandGroupName,
                primaryLocation, sameCityLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

    }

    @Test
    public void test() throws ReliabilityException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // seqDB-32334:Java驱动startCriticalMode参数校验，使用空bson
        try {
            group.startCriticalMode( new BasicBSONObject() );
            Assert.fail(
                    "startCriticalMode with null options should throw error" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_OUT_OF_BOUND
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 停止主位置节点
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );

        // 同城备中心单节点Critical模式
        String nodeName1 = sameCityLocationNodes.get( 0 )
                .getString( "hostName" ) + ":"
                + sameCityLocationNodes.get( 0 ).getString( "svcName" );
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 5 );
        options1.put( "MaxKeepTime", 10 );
        options1.put( "NodeName", nodeName1 );
        group.startCriticalMode( options1 );
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 再次指定另一个节点启动critical模式
        String nodeName2 = sameCityLocationNodes.get( 1 )
                .getString( "hostName" ) + ":"
                + sameCityLocationNodes.get( 1 ).getString( "svcName" );
        BasicBSONObject options2 = new BasicBSONObject();
        options2.put( "MinKeepTime", 10 );
        options2.put( "MaxKeepTime", 20 );
        options2.put( "NodeName", nodeName2 );
        group.startCriticalMode( options2 );

        LocationUtils.checkGroupCriticalModeStatus( sdb, groupName,
                sameCityLocationNodes.get( 1 ).getInt( "nodeID" ) );

        // 创建集合、插入数据并校验
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        dbcl = sdb.getCollectionSpace( csName ).createCollection( clName,
                option );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        // 集群环境恢复后校验数据
        group.start();
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
