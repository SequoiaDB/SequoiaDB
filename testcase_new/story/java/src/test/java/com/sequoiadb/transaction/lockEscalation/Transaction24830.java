package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24830:CL 层面 IS 与 SIX 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.10
 */
@Test(groups = "lockEscalation")
public class Transaction24830 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24830";

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
            LockEscalationUtil.queryData( db1, csName, clName, 0, 10 );
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_IS );

            LockEscalationUtil.insertData( db2, csName, clName, 10 );
            LockEscalationUtil.queryData( db2, csName, clName, 10, 10 );
            LockEscalationUtil.checkCLLockType( db2, LockEscalationUtil.LOCK_SIX );
        } finally {
            db1.commit();
            db2.commit();
        }
    }
}
