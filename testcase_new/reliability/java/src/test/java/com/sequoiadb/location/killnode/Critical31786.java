package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
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
 * @Description seqDB-31786:同城备中心异常停止
 * @Author TangTao
 * @Date 2023.06.02
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.02
 */
@Test(groups = "location")
public class Critical31786 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31786";
    private String clName = "cl_31786";
    private String idxName = "idx_31786";
    private String primaryLocation = "guangzhou.nansha_31786";
    private String sameCityLocation = "guangzhou.panyu_31786";
    private String offsiteLocation = "shenzhen.nanshan_31786";
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

    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 同城备中心异常停止，然后stop节点模拟故障无法启动
        LocationUtils.stopNodeAbnormal( sdb, groupName, sameCityLocationNodes );

        LocationUtils.checkPrimaryNodeInLocation( sdb, groupName,
                primaryLocationNodes, 30 );

        // 创建集合、创建索引、插入数据、查询计划
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        dbcl = sdb.getCollectionSpace( csName ).createCollection( clName,
                option );

        dbcl.createIndex( idxName, new BasicBSONObject( "a", 1 ), false,
                false );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        DBCursor cursor = dbcl.explain( null, null, null,
                new BasicBSONObject( "", idxName ), 0, -1, 0, null );
        BSONObject plan = cursor.getNext();
        cursor.close();
        Assert.assertEquals( plan.get( "ScanType" ), "ixscan",
                "query plan error:" + plan );
        Assert.assertEquals( plan.get( "IndexName" ), idxName,
                "query plan error:" + plan );

        // 插入Lob并校验
        int lobtimes = 1;
        LinkedBlockingQueue< LocationUtils.SaveOidAndMd5 > id2md5 = LocationUtils
                .writeLobAndGetMd5( dbcl, lobtimes );
        List< LinkedBlockingQueue< LocationUtils.SaveOidAndMd5 > > id2md5List = new ArrayList<>();
        id2md5List.add( id2md5 );
        LocationUtils.ReadLob( dbcl, id2md5List.get( 0 ) );

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

    private boolean isExplainUseIdx( DBCollection cl, String idxName ) {
        BSONObject hint = ( BSONObject ) JSON
                .parse( "{ '': '" + idxName + "' }" );
        BSONObject run = ( BSONObject ) JSON.parse( "{ Run: true }" );
        DBCursor cursor = cl.explain( null, null, null, hint, 0, -1,
                DBQuery.FLG_QUERY_FORCE_HINT, run );
        BSONObject plan = cursor.getNext();
        cursor.close();

        if ( !( plan.get( "ScanType" ) ).equals( "ixscan" )
                || !( plan.get( "IndexName" ) ).equals( idxName ) ) {
            System.out.println( "index: " + idxName );
            System.out.println( "explain:" + plan );
            return false;
        }
        return true;
    }

}
