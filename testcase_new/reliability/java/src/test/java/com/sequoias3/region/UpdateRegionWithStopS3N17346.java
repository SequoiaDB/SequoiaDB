package com.sequoias3.region;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.RegionUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.GetRegionResult;
import com.sequoias3.commlibs3.s3utils.bean.Region;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-17346:更新区域过程中sequoiaS3端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.01.29
 */

public class UpdateRegionWithStopS3N17346 extends S3TestBase {
    private boolean runSuccess = false;
    private int regionNum = 20;
    private String regionNameBase = "region17346";
    private String dataCSShardingType = "year";
    private String dataCLShardingType = "year";
    private String updateDataCSShardingType = "month";
    private String updateDataCLShardingType = "month";
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket17346";
    private String objectName = "object17436";
    private List< String > regionNames = new ArrayList< String >();
    private List< String > regionNameList = new CopyOnWriteArrayList< String >();

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        for ( int i = 0; i < regionNum; i++ ) {
            RegionUtils.clearRegion( regionNameBase + i );
            Region region = new Region()
                    .withDataCSShardingType( dataCSShardingType )
                    .withDataCLShardingType( dataCLShardingType )
                    .withName( regionNameBase + i );
            RegionUtils.putRegion( region );
            regionNames.add( regionNameBase + i );
        }
    }

    @Test
    public void test() throws Exception {
        // put region when stop s3
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < regionNum; i++ ) {
            mgr.addTask( new PutRegion( regionNames.get( i ),
                    updateDataCSShardingType, updateDataCLShardingType ) );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // get region that has not been updated and check
        regionNames.removeAll( regionNameList );
        if ( regionNames.size() > 0 ) {
            int index = new Random().nextInt( regionNames.size() );
            String regionName = regionNames.get( index );
            GetRegionResult result = RegionUtils.getRegion( regionName );
            Assert.assertEquals( result.getBuckets().size(), 0,
                    result.getBuckets().toString() );
            Region region = result.getRegion();
            Assert.assertEquals( region.getDataCSShardingType(),
                    dataCLShardingType );
            Assert.assertEquals( region.getDataCLShardingType(),
                    dataCLShardingType );
        }
        // get region that has been updated and check
        if ( regionNameList.size() > 0 ) {
            int index1 = new Random().nextInt( regionNameList.size() );
            String regionName1 = regionNameList.get( index1 );
            s3Client.createBucket(
                    new CreateBucketRequest( bucketName, regionName1 ) );
            s3Client.putObject( bucketName, objectName,
                    String.valueOf( UUID.randomUUID() ) );
            // check region information
            GetRegionResult result1 = RegionUtils.getRegion( regionName1 );
            Assert.assertEquals( result1.getBuckets().size(), 1,
                    result1.getBuckets().toString() );
            Assert.assertEquals( result1.getBuckets().get( 0 ).getName(),
                    bucketName );
            Region region1 = result1.getRegion();
            Assert.assertEquals( region1.getDataCSShardingType(),
                    updateDataCSShardingType );
            Assert.assertEquals( region1.getDataCLShardingType(),
                    updateDataCLShardingType );
            // check object information
            S3Object object = s3Client.getObject( bucketName, objectName );
            Assert.assertEquals( object.getKey(), objectName );
            Assert.assertEquals( object.getBucketName(), bucketName );
            ObjectMetadata objectMetadata = object.getObjectMetadata();
            Assert.assertEquals( objectMetadata.getVersionId(), "null" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            CommLibS3.clearBucket( s3Client, bucketName );
            for ( int i = 0; i < regionNum; i++ ) {
                RegionUtils.deleteRegion( regionNameBase + i );
            }
        }
    }

    private class PutRegion extends OperateTask {
        private String regionName = null;
        private String dataCSShardingType = null;
        private String dataCLShardingType = null;

        public PutRegion( String regionName, String dataCSShardingType,
                String dataCLShardingType ) {
            this.regionName = regionName;
            this.dataCSShardingType = dataCSShardingType;
            this.dataCLShardingType = dataCLShardingType;
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
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw e;
                }
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "I/O error on POST request" ) ) {
                    throw e;
                }
            }
        }
    }
}
