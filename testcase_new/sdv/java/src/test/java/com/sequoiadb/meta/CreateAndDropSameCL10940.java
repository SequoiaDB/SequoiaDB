package com.sequoiadb.meta;

import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: CreateAndDropCL10940.java test content:concurrent creation and
 * deletion of same cl testlink case:seqDB-10940
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class CreateAndDropSameCL10940 extends SdbTestBase {

    private String clName = "cl10940";
    private static Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void createAndDropCL10940() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateCL() );
        es.addWorker( new DropCL() );
        es.run();
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateCL {
        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection dbcl = dbcs.createCollection( clName );
                dbcl.insert( "{a:1}" );
                Assert.assertEquals( dbcl.getCount(), 1 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropCL {
        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                dbcs.dropCollection( clName );
                Assert.assertFalse( dbcs.isCollectionExist( clName ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
