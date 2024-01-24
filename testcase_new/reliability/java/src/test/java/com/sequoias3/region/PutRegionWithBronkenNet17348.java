package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.RegionUtils;
import com.sequoias3.commlibs3.s3utils.bean.GetRegionResult;
import com.sequoias3.commlibs3.s3utils.bean.Region;
import org.springframework.web.client.ResourceAccessException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-17348 :: 创建区域过程中SequoiaS3和sdb节点网络异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.01.29
 */

public class PutRegionWithBronkenNet17348 extends S3TestBase {
    private boolean runSuccess = false;
    private int regionNum = 20;
    private String regionNameBase = "region17348a";
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

    @Test(enabled = false)
    public void test() throws Exception {
        // put region when bronken
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( SdbTestBase.hostName, 2, 120 );
        TaskMgr mgr = new TaskMgr( faultTask );
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
}
