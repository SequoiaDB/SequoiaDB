package com.sequoias3.region;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.RegionUtils;
import com.sequoias3.commlibs3.s3utils.bean.Region;

/**
 * @Description seqDB-17344:删除区域过程中db端节点异常
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */
public class DeleteRegionWithKillData17344 extends S3TestBase {
    private String regionName = "beijing17344";
    private List< String > regionNames = new ArrayList< String >();
    private List< String > deletedRegionNameList = new CopyOnWriteArrayList< String >();
    private int threadNum = 10;

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < threadNum; i++ ) {
            String currRegionName = regionName + "-" + i;
            RegionUtils.clearRegion( currRegionName );

            Region region = new Region();
            region.withName( currRegionName );
            RegionUtils.putRegion( region );
            regionNames.add( currRegionName );
        }
    }

    // 可能遇到已知问题 SEQUOIADBMAINSTREAM-5197
    @Test(enabled = false)
    public void testDeleteRegion() throws Exception {
        TaskMgr mgr = new TaskMgr();
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();

        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < regionNames.size(); i++ ) {
            DeleteRegionTask dTask = new DeleteRegionTask(
                    regionNames.get( i ) );
            mgr.addTask( dTask );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the longest
        // waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        // delete again
        deleteAgainAndCheck();
    }

    @AfterClass
    private void tearDown() throws Exception {
    }

    private class DeleteRegionTask extends OperateTask {
        private String regionName = "";

        public DeleteRegionTask( String regionName ) {
            this.regionName = regionName;
        }

        @Override
        public void exec() throws Exception {
            try {
                RegionUtils.deleteRegion( regionName );
                deletedRegionNameList.add( regionName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void deleteAgainAndCheck() throws Exception {
        List< String > tobeDeleteRegions = new ArrayList< String >();
        tobeDeleteRegions.addAll( regionNames );
        tobeDeleteRegions.removeAll( deletedRegionNameList );
        for ( String regionName : tobeDeleteRegions ) {
            try {
                RegionUtils.deleteRegion( regionName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 404 ) {
                    throw e;
                }
            }
        }
        for ( int i = 0; i < regionNames.size(); i++ ) {
            Assert.assertFalse( RegionUtils.headRegion( regionNames.get( i ) ),
                    "current region name is : " + regionNames );
        }
    }
}