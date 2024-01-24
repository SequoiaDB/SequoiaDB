package com.sequoiadb.transaction.lockEscalation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

import java.util.ArrayList;

/**
 * @author yaokang
 * @description seqDB-24842:CL 层面 S 与 X 的兼容性
 * @date 2021/12/14
 * @updateUser yaokang
 * @updateDate 2021/12/14
 * @updateRemark
 */
@Test(groups = "lockEscalation")
public class Transaction24842 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String cl1Name = "cl_24842_1";
    private final String cl2Name = "cl_24842_2";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone
        if ( CommLib.isStandAlone( db1 ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        ArrayList< String > groupList = CommLib.getDataGroupNames( db1 );
        BSONObject option = new BasicBSONObject( "Group", groupList.get( 0 ) );
        db1.getCollectionSpace( csName ).createCollection( cl1Name, option );
        db1.getCollectionSpace( csName ).createCollection( cl2Name, option );
        LockEscalationUtil.insertData( db1, csName, cl1Name, 20 );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db1.getCollectionSpace( csName ).dropCollection( cl1Name );
            db1.getCollectionSpace( csName ).dropCollection( cl2Name );
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
            LockEscalationUtil.queryData( db1, csName, cl1Name, 0, 20 );
            LockEscalationUtil.checkCLLockType( db1,
                    LockEscalationUtil.LOCK_S );

            LockEscalationUtil.insertData( db2, csName, cl2Name, 20 );

            LockEscalationUtil.checkCLLockType( db2,
                    LockEscalationUtil.LOCK_X );
            try {
                LockEscalationUtil.insertData( db2, csName, cl1Name, 1 );
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
