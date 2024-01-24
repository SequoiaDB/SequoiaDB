package com.sequoias3.region;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.bean.GetRegionResult;
import com.sequoias3.commlibs3.s3utils.bean.Region;
import com.sequoias3.commlibs3.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 获取区域过程中db端节点异常 testlink-case: seqDB-17343
 * 
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */
public class GetRegionWithKillCoord17343 extends S3TestBase {
    private GroupMgr groupMgr = null;
    private String bucketName = "bucket17343";
    private String regionName = "Beijing17343";
    private String dataDomain = "dataDomain17343";
    private String metaDomain = "metaDomain17343";
    private String dataCSShardingType = "quarter";
    private String dataCLShardingType = "month";
    private int threadNum = 100;
    private AmazonS3 s3Client = null;
    private GroupWrapper coordGroup = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        s3Client = CommLibS3.buildS3Client();

        CommLibS3.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
        RegionUtils.dropDomain( metaDomain );
        RegionUtils.dropDomain( dataDomain );

        RegionUtils.createDomain( dataDomain );
        RegionUtils.createDomain( metaDomain );

        Region region = new Region();
        region.withName( regionName ).withDataDomain( dataDomain )
                .withMetaDomain( metaDomain )
                .withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType );
        RegionUtils.putRegion( region );
        s3Client.createBucket( new CreateBucketRequest( bucketName,
                regionName.toLowerCase() ) );
    }

    @Test
    public void testGetRegion() throws Exception {
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < threadNum; i++ ) {
            GetRegionTask gTask = new GetRegionTask();
            mgr.addTask( gTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        GetRegionResult result = RegionUtils.getRegion( regionName );
        checkRegion( result );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
                RegionUtils.deleteRegion( regionName );
                RegionUtils.dropDomain( metaDomain );
                RegionUtils.dropDomain( dataDomain );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class GetRegionTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try {
                GetRegionResult result = RegionUtils.getRegion( regionName );
                checkRegion( result );
            } catch ( AmazonServiceException e ) {
                if ( e.getStatusCode() != 500
                        && !e.getErrorCode().equals( "GetDBConnectFail" ) ) {
                    throw e;
                }
            }
        }
    }

    private void checkRegion( GetRegionResult result ) {
        Region region = result.getRegion();
        String actBucket = result.getBuckets().get( 0 ).getName();
        Assert.assertEquals( actBucket, bucketName );
        Assert.assertEquals( region.getName(), regionName.toLowerCase() );
        Assert.assertEquals( region.getDataCSShardingType(),
                dataCSShardingType );
        Assert.assertEquals( region.getDataCLShardingType(),
                dataCLShardingType );
        Assert.assertEquals( region.getDataDomain(), dataDomain );
        Assert.assertEquals( region.getMetaDomain(), metaDomain );
    }
}