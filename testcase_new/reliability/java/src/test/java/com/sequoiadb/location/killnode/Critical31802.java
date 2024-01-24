package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-31802:灾备中心均异常，超过MinKeepTime后节点恢复，自动停止 Critical 模式
 * @Author liuli
 * @Date 2023.05.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.30
 * @version 1.10
 */
@Test(groups = "location")
public class Critical31802 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31802";
    private String clName = "cl_31802";
    private String primaryLocation = "guangzhou.nansha_31802";
    private String sameCityLocation = "guangzhou.panyu_31802";
    private String offsiteLocation = "shenzhan.nanshan_31802";
    private int recordNum = 100000;
    private String groupName = null;
    private Integer[] replSizes = { 0, 2, 3 };

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
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        sdb.getReplicaGroup( groupName ).setActiveLocation( primaryLocation );

        CommLib.isLSNConsistency( sdb, groupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        dbcs = sdb.createCollectionSpace( csName );

        // 创建多个集合
        for ( int i = 0; i < replSizes.length; i++ ) {
            BasicBSONObject option = new BasicBSONObject();
            option.put( "Group", groupName );
            option.put( "ReplSize", replSizes[ i ] );
            option.put( "ConsistencyStrategy", 3 );
            dbcs.createCollection( clName + i, option );
        }
    }

    @Test
    public void test() throws ReliabilityException {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > slaveLocationNodes = new ArrayList<>();
        ArrayList< BasicBSONObject > offsiteLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );
        slaveLocationNodes.addAll( offsiteLocationNodes );
        slaveLocationNodes.addAll( sameCityLocationNodes );

        // 灾备中心节点全部异常停止
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject slaveLocationNode : slaveLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    slaveLocationNode.getString( "hostName" ),
                    slaveLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject slaveLocationNode : slaveLocationNodes ) {
            String nodeName = slaveLocationNode.getString( "hostName" ) + ":"
                    + slaveLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 主中心启动Critical模式
        BasicBSONObject options = new BasicBSONObject();
        int mixKeepTime = 1;
        options.put( "MinKeepTime", mixKeepTime );
        options.put( "MaxKeepTime", 20 );
        options.put( "Location", primaryLocation );
        group.startCriticalMode( options );
        Date beginTime = new Date( System.currentTimeMillis() );

        // 插入数据并校验
        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clName + i );
            List< BSONObject > batchRecords = CommLib.insertData( dbcl,
                    recordNum );
            BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
            CommLib.checkRecords( dbcl, batchRecords, orderBy );
        }

        // 等待时间超过MinKeepTime
        LocationUtils.validateWaitTime( beginTime, mixKeepTime + 1 );

        // 启动所有节点
        group.start();

        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clName + i );
            // 超过半数节点恢复后解除Critical模式，ReplSize为0的集合插入数据可能会失败
            if ( i != 0 ) {
                dbcl.truncate();
                List< BSONObject > batchRecords = CommLib.insertData( dbcl,
                        recordNum );
                BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
                CommLib.checkRecords( dbcl, batchRecords, orderBy );
            }
        }

        // 等待集群恢复正常
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );

        // 校验Critical模式已经停止
        group.stopCriticalMode();
        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clName + i );
            dbcl.truncate();
            List< BSONObject > batchRecords = CommLib.insertData( dbcl,
                    recordNum );
            BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
            CommLib.checkRecords( dbcl, batchRecords, orderBy );
        }
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
