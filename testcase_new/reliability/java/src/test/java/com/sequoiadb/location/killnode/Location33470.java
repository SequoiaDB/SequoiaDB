package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-33470:同城主备data节点异常，启动运维模式，到达运行窗口时间后停止运维模式
 * @Author liuli
 * @Date 2023.09.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.09.22
 * @version 1.10
 */
@Test(groups = "location")
public class Location33470 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private CollectionSpace dbcs = null;
    private String csName = "cs_33470";
    private String clName = "cl_33470";
    private String primaryLocation = "guangzhou.nansha_33470";
    private String sameCityLocation = "guangzhou.panyu_33470";
    private String offsiteLocation = "shenzhan.nanshan_33470";
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
        // 同城主备中心节点异常
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        ArrayList< BasicBSONObject > cityLocationNodes = new ArrayList<>();
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, expandGroupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, expandGroupName,
                        sameCityLocation );
        cityLocationNodes.addAll( primaryLocationNodes );
        cityLocationNodes.addAll( sameCityLocationNodes );

        // 同城主备中心节点异常停止
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject cityLocationNode : cityLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cityLocationNode.getString( "hostName" ),
                    cityLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject cityLocationNode : cityLocationNodes ) {
            String nodeName = cityLocationNode.getString( "hostName" ) + ":"
                    + cityLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 指定同城主备中启动运维模式
        int minKeepTime = 1;
        int maxKeepTime = 2;
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", minKeepTime );
        options.put( "MaxKeepTime", maxKeepTime );
        options.put( "Location", primaryLocation );
        group.startMaintenanceMode( options );

        options.clear();
        options.put( "MinKeepTime", minKeepTime );
        options.put( "MaxKeepTime", maxKeepTime );
        options.put( "Location", sameCityLocation );
        group.startMaintenanceMode( options );

        Date beginTime = new Date( System.currentTimeMillis() );

        // 创建集合并插入数据
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        option.put( "ReplSize", 0 );
        DBCollection dbcl = dbcs.createCollection( clName, option );

        // 集合插入数据
        List< BSONObject > batchRecords = CommLib.insertData( dbcl, recordNum );

        // 同城主中心节点恢复，同城备中节点未恢复
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            String nodeName = primaryLocationNode.getString( "hostName" ) + ":"
                    + primaryLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.start();
        }

        // 等待时间超过MaxKeepTime
        LocationUtils.validateWaitTime( beginTime, maxKeepTime + 1 );

        try {
            dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 解除运维模式，节点恢复正常
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
