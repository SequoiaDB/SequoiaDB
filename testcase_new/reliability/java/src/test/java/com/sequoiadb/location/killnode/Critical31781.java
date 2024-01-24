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
 * @Description seqDB-31781:主中心异常，超过MinKeepTime节点未恢复，手动停止Critical模式
 * @Author liuli
 * @Date 2023.05.29
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.29
 * @version 1.10
 */
@Test(groups = "location")
public class Critical31781 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31781";
    private String clName = "cl_31781";
    private String primaryLocation = "guangzhou.nansha_31781";
    private String sameCityLocation = "guangzhou.panyu_31781";
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
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        // 创建集合，指定ReplSize为0
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", groupName );
        option.put( "ReplSize", 0 );
        dbcl = dbcs.createCollection( clName, option );
    }

    @Test
    public void test() throws ReliabilityException {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 主中心节点异常停止
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    primaryLocationNode.getString( "hostName" ),
                    primaryLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            String nodeName = primaryLocationNode.getString( "hostName" ) + ":"
                    + primaryLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 备中心Location启动Critical模式
        BasicBSONObject options = new BasicBSONObject();
        int minKeepTime = 1;
        options.put( "MinKeepTime", minKeepTime );
        options.put( "MaxKeepTime", 20 );
        options.put( "Location", sameCityLocation );
        group.startCriticalMode( options );
        Date beginTime = new Date( System.currentTimeMillis() );

        // 创建集合插入数据并校验
        List< BSONObject > batchRecords = CommLib.insertData( dbcl, recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );

        // 等待时间超过MinKeepTime
        LocationUtils.validateWaitTime( beginTime, minKeepTime + 1 );

        // 手动停止Critical模式
        group.stopCriticalMode();

        // 集合执行truncate
        try {
            dbcl.truncate();
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            System.out.println( "e -- " + e );
            if ( e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY.getErrorCode()
                    && e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                            .getErrorCode() ) {
                throw e;
            }
        }

        // 主中心节点启动并恢复正常
        group.start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.checkGroupStopCriticalMode( sdb, groupName );
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
