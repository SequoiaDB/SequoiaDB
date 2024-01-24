package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24831:CL 层面 IS 与 S(DDL) 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.10
 */
@Test(groups = "lockEscalation")
public class Transaction24831 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private CollectionSpace cs;
    private final String clName = "cl_24831";
    private final String clName_new = "cl_24831_new";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        cs = db1.getCollectionSpace( csName );
        cs.createCollection( clName );
        LockEscalationUtil.insertData( db1, csName, clName, 10 );
    }

    @AfterClass()
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( clName_new ) ) {
                cs.dropCollection( clName_new );
            }
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
            LockEscalationUtil.queryData( db1, csName, clName, 0, 5 );
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_IS );

            db2.getCollectionSpace( csName ).renameCollection( clName, clName_new );

            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_IS );
            LockEscalationUtil.queryData( db1, csName, clName_new, 5, 5 );
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_IS );
        } finally {
            db1.commit();
            db2.commit();
        }
    }
}
