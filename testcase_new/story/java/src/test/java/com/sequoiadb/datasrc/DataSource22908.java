package com.sequoiadb.datasrc;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-22908:并发查看和删除数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22908 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName = "datasource22908";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22908", dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DeleteDataSrc deleteDataSrc = new DeleteDataSrc();
        GetDataSrc getDataSrc = new GetDataSrc();
        es.addWorker( deleteDataSrc );
        es.addWorker( getDataSrc );
        es.run();

        if ( getDataSrc.getRetCode() != 0 ) {
            Assert.assertEquals( deleteDataSrc.getRetCode(), 0 );
            // 获取数据源失败,删除数据源成功
            Assert.assertEquals( getDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            Assert.assertFalse( sdb.isDataSourceExist( dataSrcName ) );
        } else {
            // 可能先获取成功，后删除成功
            Assert.assertEquals( deleteDataSrc.getRetCode(), 0 );
            Assert.assertFalse( sdb.isDataSourceExist( dataSrcName ) );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isDataSourceExist( dataSrcName ) ) {
                sdb.dropDataSource( dataSrcName );
            }

        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DeleteDataSrc extends ResultStore {
        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropDataSource( dataSrcName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class GetDataSrc extends ResultStore {
        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getDataSource( dataSrcName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
