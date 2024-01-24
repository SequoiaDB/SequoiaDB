package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;

/**
 * @Description: seqDB-24848:CL 层面 X 与 X 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.13
 */
@Test(groups = "lockEscalation")
public class Transaction24848 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName1 = "cl_24848_A";
    private final String clName2 = "cl_24848_B";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( !CommLib.isStandAlone( db1 ) ) {
            ArrayList< String > groupList =  CommLib.getDataGroupNames( db1 );
            BSONObject option = new BasicBSONObject("Group", groupList.get( 0 ));
            db1.getCollectionSpace( csName ).createCollection( clName1, option );
            db1.getCollectionSpace( csName ).createCollection( clName2, option );
        }else {
            db1.getCollectionSpace( csName ).createCollection( clName1 );
            db1.getCollectionSpace( csName ).createCollection( clName2 );
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            db1.getCollectionSpace( csName ).dropCollection( clName1 );
            db1.getCollectionSpace( csName ).dropCollection( clName2 );
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
            LockEscalationUtil.insertData( db1, csName, clName1, 20 );
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_X );

            LockEscalationUtil.insertData( db2, csName, clName2, 20 );
            LockEscalationUtil.checkCLLockType( db2, LockEscalationUtil.LOCK_X );
            try {
                LockEscalationUtil.insertData( db2, csName, clName1, 1 );
                Assert.fail( "The X lock should conflict with the X lock, but it does not!" );
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
