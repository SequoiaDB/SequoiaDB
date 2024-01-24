package com.sequoias3.region;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
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
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-17345 :: 创建区域过程中sequoiaS3端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.01.29
 */

public class PutRegionWithStopS3N17345 extends S3TestBase {
    private boolean runSuccess = false;
    private int regionNum = 10;
    private String regionNameBase = "region17345a";
    private String dataCSShardingType = "year";
    private String dataCLShardingType = "month";
    private List< String > regionNames = new ArrayList< String >();
    private List< String > regionNameList = new CopyOnWriteArrayList< String >();

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < regionNum; i++ ) {
            RegionUtils.clearRegion( regionNameBase + i );
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
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( "regionName = " + regionName, e );
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
