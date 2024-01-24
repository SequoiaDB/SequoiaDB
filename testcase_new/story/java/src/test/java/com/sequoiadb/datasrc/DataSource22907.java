package com.sequoiadb.datasrc;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DataSource;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-22907:并发修改和删除数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22907 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName = "datasource22907";
    private String newDataSrcName = "updateDatasource22907";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22907", dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DeleteDataSrc deleteDataSrc = new DeleteDataSrc();
        AlterDataSrc alterDataSrc = new AlterDataSrc();
        es.addWorker( deleteDataSrc );
        es.addWorker( alterDataSrc );
        es.run();

        if ( alterDataSrc.getRetCode() != 0 ) {
            Assert.assertEquals( deleteDataSrc.getRetCode(), 0 );
            // 修改数据源失败,删除数据源成功
            Assert.assertEquals( alterDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            Assert.assertFalse( sdb.isDataSourceExist( dataSrcName ) );
            Assert.assertFalse( sdb.isDataSourceExist( newDataSrcName ) );
        } else {
            // 可能先修改成功，后删除失败
            Assert.assertEquals( deleteDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            Assert.assertTrue( sdb.isDataSourceExist( newDataSrcName ) );
            Assert.assertFalse( sdb.isDataSourceExist( dataSrcName ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isDataSourceExist( dataSrcName ) ) {
                sdb.dropDataSource( dataSrcName );
            }
            if ( sdb.isDataSourceExist( newDataSrcName ) ) {
                sdb.dropDataSource( newDataSrcName );
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

    private class AlterDataSrc extends ResultStore {
        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                obj.put( "Name", newDataSrcName );
                DataSource ds = db.getDataSource( dataSrcName );
                ds.alterDataSource( obj );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
