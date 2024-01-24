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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-22902:并发删除数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22902 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataSrcName = "datasource22902";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, "cs22902", dataSrcName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DropDataSource( dataSrcName ) );
        es.addWorker( new DropDataSource( dataSrcName ) );
        es.addWorker( new DropDataSource( dataSrcName ) );
        es.run();

        checkResult( dataSrcName );
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class DropDataSource {
        private String name;

        private DropDataSource( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropDataSource( name );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_CAT_DATASOURCE_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void checkResult( String name ) {
        try {
            sdb.getDataSource( name );
            Assert.fail( "get datasource should be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CAT_DATASOURCE_NOTEXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }
}
