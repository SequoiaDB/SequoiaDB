package com.sequoias3.region;

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
import com.sequoias3.commlibs3.s3utils.bean.GetRegionResult;
import com.sequoias3.commlibs3.s3utils.bean.Region;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-17341:创建区域过程中db端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.01.29
 */

public class PutRegionWithKillCoord17341 extends S3TestBase {
    private boolean runSuccess = false;
    private int regionNum = 20;
    private String regionNameBase = "region17341a";
    private String dataCSShardingType = "year";
    private String dataCLShardingType = "month";
    private List< String > regionNames = new ArrayList< String >();
    private List< String > regionNameList = new CopyOnWriteArrayList< String >();
    private GroupMgr groupMgr = null;
    private GroupWrapper coordGroup = null;

    @BeforeClass
    private void setUp() throws Exception {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        for ( int i = 0; i < regionNum; i++ ) {
            RegionUtils.clearRegion( regionNameBase + i );
            regionNames.add( regionNameBase + i );
        }
    }

    // SEQUOIADBMAINSTREAM-4852
    @Test(enabled = false)
    public void test() throws Exception {
        // put region when kill coord
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 2 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < regionNum; i++ ) {
            mgr.addTask( new PutRegion( regionNames.get( i ) ) );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        // put region again
        regionNames.removeAll( regionNameList );
        for ( String regionName : regionNames ) {
            Region region = new Region()
                    .withDataCSShardingType( dataCSShardingType )
                    .withDataCLShardingType( dataCLShardingType )
                    .withName( regionName );
            RegionUtils.putRegion( region );
            regionNameList.add( regionName );
        }
        for ( String regionName : regionNameList ) {
            GetRegionResult result = RegionUtils.getRegion( regionName );
            Assert.assertEquals( result.getBuckets().size(), 0,
                    result.getBuckets().toString() );
            Region region = result.getRegion();
            Assert.assertEquals( region.getDataCSShardingType(),
                    dataCSShardingType );
            Assert.assertEquals( region.getDataCLShardingType(),
                    dataCLShardingType );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            for ( int i = 0; i < regionNum; i++ ) {
                RegionUtils.deleteRegion( regionNameBase + i );
            }
        }
    }

    private class PutRegion extends OperateTask {
        private String regionName = null;

        public PutRegion( String regionName ) {
            this.regionName = regionName;
        }

        @Override
        public void exec() throws Exception {
            Region region = new Region()
                    .withDataCSShardingType( dataCSShardingType )
                    .withDataCLShardingType( dataCLShardingType )
                    .withName( regionName );
            try {
                RegionUtils.putRegion( region );
                regionNameList.add( this.regionName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500
                        && !e.getErrorCode().contains( "GetDBConnectFail" ) ) {
                    throw e;
                }
            }
        }
    }
}
