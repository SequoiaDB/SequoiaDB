package com.sequoiadb.transaction.lockEscalation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @author yaokang
 * @description seqDB-24841:CL 层面 S 与 S(DDL) 的兼容性
 * @date 2021/12/14
 * @updateUser yaokang
 * @updateDate 2021/12/14
 * @updateRemark
 */
@Test(groups = "lockEscalation")
public class Transaction24841 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24841";
    private final String newClName = "cl_24841_new";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1.getCollectionSpace( csName ).createCollection( clName );
        LockEscalationUtil.insertData( db1, csName, clName, 30 );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db1.getCollectionSpace( csName ).dropCollection( newClName );
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

            db2.getCollectionSpace( csName ).renameCollection( clName,
                    newClName );

            LockEscalationUtil.queryData( db1, csName, newClName, 20, 10 );
        } finally {
            db1.commit();
            db2.commit();
        }
    }

}
