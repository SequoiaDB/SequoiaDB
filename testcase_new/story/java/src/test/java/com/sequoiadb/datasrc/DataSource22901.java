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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-22901:并发修改数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22901 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName = "datasource22901";
    private String newDataSrcName1 = "datasource22901a";
    private String newDataSrcName2 = "datasource22901b";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22901", dataSrcName );
        DataSrcUtils.clearDataSource( sdb, "cs22901", newDataSrcName1 );
        DataSrcUtils.clearDataSource( sdb, "cs22901", newDataSrcName2 );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new AlterDataSource( newDataSrcName1 ) );
        es.addWorker( new AlterDataSource( newDataSrcName2 ) );
        es.addWorker( new AlterDataSourceb() );
        es.run();

        checkResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            DataSrcUtils.clearDataSource( sdb, "cs22901", newDataSrcName1 );
            DataSrcUtils.clearDataSource( sdb, "cs22901", newDataSrcName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class AlterDataSource {
        private String name;

        private AlterDataSource( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        private void alter() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                obj.put( "Name", name );
                DataSource ds = db.getDataSource( dataSrcName );
                ds.alterDataSource( obj );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_CAT_DATASOURCE_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class AlterDataSourceb {
        @ExecuteOrder(step = 1)
        private void alter() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                obj.put( "ErrorFilterMask", "WRITE" );
                DataSource ds = db.getDataSource( dataSrcName );
                ds.alterDataSource( obj );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_CAT_DATASOURCE_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void checkResult() {
        if ( sdb.isDataSourceExist( newDataSrcName1 ) ) {
            DataSource dataSrcInfo = sdb.getDataSource( newDataSrcName1 );
            Assert.assertEquals( dataSrcInfo.getName(), newDataSrcName1 );
        } else {
            DataSource dataSrcInfo = sdb.getDataSource( newDataSrcName2 );
            Assert.assertEquals( dataSrcInfo.getName(), newDataSrcName2 );
        }
    }
}
