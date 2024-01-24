package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24832:CL 层面 IS 与 X 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.10
 */
@Test(groups = "lockEscalation")
public class Transaction24832 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24832";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        db1.getCollectionSpace( csName ).createCollection( clName );
        LockEscalationUtil.insertData( db1, csName, clName, 10 );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db1.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            db1.close();
            db2.close();
        }
    }

    @Test
    public void test() {
        db1.beginTransaction();
        db2.beginTransaction();
        try {
            LockEscalationUtil.queryData( db1, csName, clName, 0, 10 );
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_IS );

            LockEscalationUtil.insertData( db2, csName, clName, 10 );
            try {
                LockEscalationUtil.insertData( db2, csName, clName, 10 );  // X lock
                Assert.fail( "The IS lock should conflict with the X lock, but it does not!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE.getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            db1.commit();
            db2.commit();
        }
    }
}
