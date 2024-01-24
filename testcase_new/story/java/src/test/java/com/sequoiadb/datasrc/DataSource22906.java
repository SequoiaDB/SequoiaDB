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
 * @Description seqDB-22906:并发创建和修改数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22906 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName = "datasource22906";
    private String newDataSrcName = "updateDatasource22906";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22906", dataSrcName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        CreateDataSrc createDataSrc = new CreateDataSrc();
        AlterDataSrc alterDataSrc = new AlterDataSrc();
        es.addWorker( createDataSrc );
        es.addWorker( alterDataSrc );
        es.run();

        Assert.assertEquals( createDataSrc.getRetCode(), 0 );
        if ( alterDataSrc.getRetCode() != 0 ) {
            // 先修改数据源失败,后创建数据源成功
            Assert.assertEquals( alterDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            Assert.assertTrue( sdb.isDataSourceExist( dataSrcName ) );
        } else {
            // 可能先创建成功，后修改成功
            Assert.assertEquals( alterDataSrc.getRetCode(), 0 );
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

    private class CreateDataSrc extends ResultStore {
        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                db.createDataSource( dataSrcName, DataSrcUtils.getSrcUrl(),
                        DataSrcUtils.getSrcUrl(), DataSrcUtils.getPasswd(), "",
                        obj );
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
