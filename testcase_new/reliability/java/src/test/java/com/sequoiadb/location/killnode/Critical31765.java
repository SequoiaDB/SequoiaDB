package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.types.ObjectId;

/**
 * @version 1.10
 * @Description seqDB-31765:备中心异常停止，主中心正常提供服务
 * @Author huanghaimei
 * @Date 2023.05.29
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.05.29
 */
@Test(groups = "location")
public class Critical31765 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31765";
    private String clName = "cl_31765";
    private String indexName = "index_31765";
    private String primaryLocation = "guangzhou.nansha_31765";
    private String sameCityLocation = "guangzhou.panyu_31765";
    private int recordNum = 10000;
    private String groupName = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupName = SdbTestBase.expandGroupNames.get( 0 );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        LocationUtils.setTwoLocationInSameCity( sdb, groupName, primaryLocation,
                sameCityLocation );
        sdb.getReplicaGroup( groupName ).setActiveLocation( primaryLocation );

        CommLib.isLSNConsistency( sdb, groupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName );

    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停备中心节点
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    sameCityLocationNode.getString( "hostName" ),
                    sameCityLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 检查主节点是否变化
        LocationUtils.checkPrimaryNodeInLocation( sdb, groupName,
                primaryLocationNodes, 30 );

        // 创建集合、创建索引、插入数据、查询计划
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        dbcl = sdb.getCollectionSpace( csName ).createCollection( clName,
                option );

        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), false,
                false );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        dbcl.explain( null, null, null, new BasicBSONObject( "", indexName ), 0,
                -1, 0, null );

        // 插入Lob并校验
        int lobtimes = 1;
        LinkedBlockingQueue< LocationUtils.SaveOidAndMd5 > id2md5 = LocationUtils
                .writeLobAndGetMd5( dbcl, lobtimes );
        List< LinkedBlockingQueue< LocationUtils.SaveOidAndMd5 > > id2md5List = new ArrayList<>();
        id2md5List.add( id2md5 );

        LocationUtils.ReadLob( dbcl, id2md5List.get( 0 ) );
        sdb.getReplicaGroup( groupName ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb, expandGroupNames.get( 0 ) );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

}