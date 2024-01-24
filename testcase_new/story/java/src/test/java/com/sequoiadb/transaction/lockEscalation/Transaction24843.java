package com.sequoiadb.transaction.lockEscalation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @author yaokang
 * @description seqDB-24843:CL 层面 S 与 Z 的兼容性
 * @date 2021/12/14
 * @updateUser yaokang
 * @updateDate 2021/12/14
 * @updateRemark
 */
@Test(groups = "lockEscalation")
public class Transaction24843 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24843";

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
            LockEscalationUtil.queryData( db1, csName, clName, 0, 20 );
            LockEscalationUtil.checkCLLockType( db1,
                    LockEscalationUtil.LOCK_S );

            try {
                db2.getCollectionSpace( csName ).getCollection( clName )
                        .truncate();
                Assert.fail( "Incompatible lock exception expected" );
            } catch ( BaseException e ) {
                if ( SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                        .getErrorCode() != e.getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            db1.commit();
            db2.commit();
        }
    }

}
