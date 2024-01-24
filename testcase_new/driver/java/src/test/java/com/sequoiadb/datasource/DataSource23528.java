package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-23528 : MinIdleCount/MaxIdleCount参数校验验证
 * @Author Cheng Jingjing
 * @CreateDate 2023/06/06
 * @UpdateUser Cheng Jingjing
 * @UpdateDate 2023/06/06
 * @UpdateRemark
 * @Version v1.0
 */
public class DataSource23528 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private String addr = null;

    @BeforeClass
    public void setUp() {
        addr = SdbTestBase.coordUrl;
    }

    @Test
    public void testInitDS() throws Exception {
        DatasourceOptions dsOpt = new DatasourceOptions();

        // case 1: default value: min 0, max 10
        Assert.assertEquals( 0, dsOpt.getMinIdleCount() );
        Assert.assertEquals( 10, dsOpt.getMaxIdleCount() );
        checkInitDS( dsOpt.getMinIdleCount(), dsOpt.getMaxIdleCount() );

        // case 2: min 0, max 0
        checkInitDS( 0, 0 );

        // case 3: min = 5, max = 10
        checkInitDS( 5, 10 );

        // case 4: min = 10, max = 10;
        checkInitDS( 10, 10 );

        // case 5: min = 10, max = 5
        try {
            checkInitDS( 10, 5 );
            Assert.fail(
                    "exp fail but act success: minIdleCount should less than maxIdleCount" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
    }

    private void checkInitDS( int min, int max ) throws Exception {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount( 30 );
        dsOpt.setMinIdleCount( min );
        dsOpt.setMaxIdleCount( max );
        dsOpt.setCheckInterval( 500 ); // 500ms

        SequoiadbDatasource ds = new SequoiadbDatasource( addr, "", "", dsOpt );
        try {
            checkIdleCount( ds, dsOpt );
        } finally {
            ds.close();
        }
    }

    private void checkIdleCount( SequoiadbDatasource ds,
                                 DatasourceOptions dsOpt ) throws Exception {
        int avgCount = ( dsOpt.getMinIdleCount() + dsOpt.getMaxIdleCount() )
                / 2;
        List< Sequoiadb > connList = new ArrayList<>();

        Assert.assertEquals( dsOpt.getMinIdleCount(),
                ds.getDatasourceOptions().getMinIdleCount() );
        Assert.assertEquals( dsOpt.getMaxIdleCount(),
                ds.getDatasourceOptions().getMaxIdleCount() );

        connList.add( ds.getConnection() );
        // wait for CreateConnectionTask
        int loopTime = 10 ;
        while( ds.getIdleConnNum() != avgCount && loopTime-- > 0 ){
            Thread.sleep( dsOpt.getCheckInterval() );
        }
        Assert.assertEquals( ds.getIdleConnNum(), avgCount );
        // idleCount < avgCount, new connection will create by
        // CreateConnectionTask
        connList.add( ds.getConnection() );
        // wait for CheckConnectionTask and CreateConnectionTask
        loopTime = 10;
        while( ds.getIdleConnNum() != avgCount && loopTime-- > 0 ){
            Thread.sleep( dsOpt.getCheckInterval() );
        }
        Assert.assertEquals( ds.getIdleConnNum(), avgCount );

        for ( Sequoiadb db : connList ) {
            ds.releaseConnection( db );
        }
    }

    @Test
    public void testUpdateConf() throws Exception {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount( 30 );
        dsOpt.setCheckInterval( 500 );
        dsOpt.setMinIdleCount( 0 );
        dsOpt.setMaxIdleCount( 20 );
        SequoiadbDatasource ds = new SequoiadbDatasource( addr, "", "", dsOpt );
        try {
            // min = 0, max = 20
            checkIdleCount( ds, dsOpt );

            // case 1: update min = 0, max = 10
            checkUpdateConf( ds, 0, 10 );

            // case 2: update min = 5, max = 10
            checkUpdateConf( ds, 5, 10 );

            // case 3: update min = 10, max = 10
            checkUpdateConf( ds, 10, 10 );

            // case 4: update min = 20, max = 10
            try {
                checkUpdateConf( ds, 20, 10 );
                Assert.fail(
                        "exp fail but act success: minIdleCount should less than maxIdleCount" );
            } catch ( BaseException e ) {
                Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                        e.getErrorCode() );
            }

            // case 5: update min = 10, max = 5
            try {
                checkUpdateConf( ds, 10, 5 );
                Assert.fail(
                        "exp fail but act success: minIdleCount should less than maxIdleCount" );
            } catch ( BaseException e ) {
                Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                        e.getErrorCode() );
            }
        } finally {
            ds.close();
        }
    }

    private void checkUpdateConf( SequoiadbDatasource ds, int min, int max )
            throws Exception {
        DatasourceOptions dsOpt = ds.getDatasourceOptions();
        dsOpt.setMinIdleCount( min );
        dsOpt.setMaxIdleCount( max );

        ds.updateDatasourceOptions( dsOpt );
        Assert.assertEquals( dsOpt.getMinIdleCount(),
                ds.getDatasourceOptions().getMinIdleCount() );
        Assert.assertEquals( dsOpt.getMaxIdleCount(),
                ds.getDatasourceOptions().getMaxIdleCount() );
    }

    @AfterClass
    public void tearDown() {
        if ( ds != null ) {
            ds.close();
        }
    }
}