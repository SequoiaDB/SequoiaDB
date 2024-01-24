package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24852:CL 层面 Z 与 Z 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.13
 */
@Test(groups = "lockEscalation")
public class Transaction24852 extends SdbTestBase {
    private Sequoiadb db;
    private final String clName = "cl_24852";
    private final int cycleNum = 1000;

    @BeforeClass()
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db.getCollectionSpace( csName ).createCollection( clName );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            db.close();
        }
    }

    @Test
    public void test() {
        ZLockThread zLock = new ZLockThread();
        zLock.start( 2 );
        Assert.assertTrue( zLock.isSuccess(), zLock.getErrorMsg() );
    }

    class ZLockThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < cycleNum; i++ ) {
                    try {
                        cl.truncate();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                                || e.getErrorCode() == SDBError.SDB_LOCK_FAILED
                                        .getErrorCode() ) {
                            break;
                        } else if ( e
                                .getErrorCode() == SDBError.SDB_DMS_TRUNCATED
                                        .getErrorCode() ) {
                            continue;
                        }
                        throw e;
                    }
                }
            }
        }
    }
}
