package com.sequoias3.region;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.RegionUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.Region;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * test content: 获取区域列表过程中sequoiaS3端节点异常 testlink-case: seqDB-17347
 *
 * @author wangkexin
 * @Date 2019.05.10
 * @version 1.00
 */

public class GetRegionListWithReStartS3N17347 extends S3TestBase {
    private String regionName = "Beijing17347";
    private String bucketName = "bucket17347";
    private String metaCSName = "metaCS17347";
    private String dataCSName = "dataCS17347";
    private String[] metaClNames = { "metaCL17347", "metaHistoryCL17347" };
    private String[] dataClName = { "dataCL17347" };
    private List< String > regionNames = new ArrayList<>();
    private int regionNum = 10000;
    private int bucketNum = 80;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLibS3.buildS3Client();
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );
        clearBuckets();
        // create regions
        for ( int i = 0; i < regionNum; i++ ) {
            String currRegionName = regionName + "-" + i;
            regionNames.add( currRegionName.toLowerCase() );
            RegionUtils.clearRegion( currRegionName );

            Region region = new Region();
            region.withName( regionNames.get( i ) )
                    .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                    .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                    .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
            RegionUtils.putRegion( region );
            if ( i < bucketNum ) {
                s3Client.createBucket( new CreateBucketRequest( bucketName + i,
                        regionNames.get( i ).toLowerCase() ) );
            }
        }
    }

    @Test
    public void testCreateRegion() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new GetRegionList() );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // list again
        List< String > actRegions = RegionUtils.listRegions();
        checkListResult( actRegions );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                clearBuckets();
                RegionUtils.dropCS( metaCSName );
                RegionUtils.dropCS( dataCSName );
                deleteRegions( regionNames );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class GetRegionList extends OperateTask {
        @Override
        public void exec() throws Exception {
            try {
                List< String > actRegions = RegionUtils.listRegions();
                checkListResult( actRegions );
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

    private void checkListResult( List< String > actRegions ) {
        Collections.sort( regionNames );
        Assert.assertEquals( actRegions.size(), regionNames.size() );
        Assert.assertEquals( actRegions, regionNames );
    }

    private void clearBuckets() {
        for ( int i = 0; i < bucketNum; i++ ) {
            CommLibS3.clearBucket( s3Client, bucketName + i );
        }
    }

    private void deleteRegions( List< String > regions ) throws Exception {
        for ( int i = 0; i < regions.size(); i++ ) {
            if ( RegionUtils.headRegion( regions.get( i ) ) ) {
                RegionUtils.deleteRegion( regions.get( i ) );
            }
        }
    }
}
