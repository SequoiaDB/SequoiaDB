package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @author yaokang
 * @description CL 层面 IX 与 S 的兼容性
 * @date 2021/12/14
 * @updateUser yaokang
 * @updateDate 2021/12/14
 * @updateRemark
 */
@Test(groups = "lockEscalation")
public class Transaction24834 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24834";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1.getCollectionSpace( csName ).createCollection( clName );
        LockEscalationUtil.insertData( db1, csName, clName, 20 );
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
            LockEscalationUtil.insertData( db1, csName, clName, 10 );
            LockEscalationUtil.checkCLLockType( db1,
                    LockEscalationUtil.LOCK_IX );

            LockEscalationUtil.queryData( db2, csName, clName, 0, 10 );
            try {
                LockEscalationUtil.queryData( db2, csName, clName, 10, 10 );
                Assert.fail( "Timeout out exception expected!" );
            } catch ( BaseException e ) {
                if ( SDBError.SDB_TIMEOUT.getErrorCode() != e.getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            db1.commit();
            db2.commit();
        }
    }

}
