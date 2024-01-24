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
 * @Description seqDB-22905:并发创建和删除数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22905 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName = "datasource22905";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22905", dataSrcName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        CreateDataSrc createDataSrc = new CreateDataSrc();
        DeleteDataSrc deleteDataSrc = new DeleteDataSrc();
        es.addWorker( createDataSrc );
        es.addWorker( deleteDataSrc );
        es.run();

        if ( createDataSrc.getRetCode() != 0 ) {
            // 创建数据源失败,删除数据源成功
            Assert.assertEquals( deleteDataSrc.getRetCode(), 0 );
            // 未跑到这个分支，待确定？
            Assert.assertEquals( createDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            Assert.assertFalse( sdb.isDataSourceExist( dataSrcName ) );
        } else if ( deleteDataSrc.getRetCode() != 0 ) {
            // 创建数据源成功,删除数据源失败
            Assert.assertEquals( createDataSrc.getRetCode(), 0 );
            Assert.assertEquals( deleteDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            DataSource dataSrcInfo = sdb.getDataSource( dataSrcName );
            Assert.assertEquals( dataSrcInfo.getName(), dataSrcName );
        } else {
            // 可能先创建成功，后删除成功
            Assert.assertEquals( createDataSrc.getRetCode(), 0 );
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

    private class CreateDataSrc extends ResultStore {
        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                db.createDataSource( dataSrcName, DataSrcUtils.getSrcUrl(),
                        DataSrcUtils.getUser(), DataSrcUtils.getPasswd(), "",
                        obj );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
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
}
