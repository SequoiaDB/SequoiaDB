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
 * @Description seqDB-22900:并发创建数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22900 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName1 = "datasource22900a";
    private String dataSrcName2 = "datasource22900b";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22900", dataSrcName1 );
        DataSrcUtils.clearDataSource( sdb, "cs22900", dataSrcName2 );

    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateDataSource( dataSrcName1 ) );
        es.addWorker( new CreateDataSource( dataSrcName1 ) );
        es.addWorker( new CreateDataSource( dataSrcName2 ) );
        es.run();

        checkResult( dataSrcName1 );
        checkResult( dataSrcName2 );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropDataSource( dataSrcName1 );
            sdb.dropDataSource( dataSrcName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateDataSource {
        private String name;

        private CreateDataSource( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                db.createDataSource( name, DataSrcUtils.getSrcUrl(),
                        DataSrcUtils.getUser(), DataSrcUtils.getPasswd(), "",
                        obj );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_CAT_DATASOURCE_EXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void checkResult( String name ) {
        DataSource dataSrcInfo = sdb.getDataSource( name );
        Assert.assertEquals( dataSrcInfo.getName(), name );
    }
}
