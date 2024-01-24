package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.bean.Region;
import com.sequoias3.commlibs3.s3utils.RegionUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.springframework.web.client.ResourceAccessException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 删除区域过程中SequoiaS3和sdb节点网络异常 testlink-case: seqDB-17349
 * 
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */
public class DeleteRegionWithBrokenNet17349 extends S3TestBase {
    private String regionName = "beijing17349";
    private List< String > regionNames = new ArrayList< String >();
    private List< String > deletedRegionNameList = new CopyOnWriteArrayList< String >();

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < 10; i++ ) {
            String currRegionName = regionName + "-" + i;
            RegionUtils.clearRegion( currRegionName );

            Region region = new Region();
            region.withName( currRegionName );
            RegionUtils.putRegion( region );
            regionNames.add( currRegionName );
        }
    }

    @Test(enabled = false)
    public void testDeleteRegion() throws Exception {
        try {
            // delete region when network broken
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( SdbTestBase.hostName, 1, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );

            for ( int i = 0; i < regionNames.size(); i++ ) {
                DeleteRegionTask dTask = new DeleteRegionTask(
                        regionNames.get( i ) );
                mgr.addTask( dTask );
            }
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // delete again
            deleteAgainAndCheck();

        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
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
            } catch ( ResourceAccessException e ) {
                if ( !e.getMessage()
                        .contains( "I/O error on POST request " ) ) {
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
            RegionUtils.deleteRegion( regionName );
        }

        for ( int i = 0; i < regionNames.size(); i++ ) {
            Assert.assertFalse( RegionUtils.headRegion( regionNames.get( i ) ),
                    "current region name is : " + regionNames );
        }
    }
}