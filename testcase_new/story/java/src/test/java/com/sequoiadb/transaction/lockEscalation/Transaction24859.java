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
 * @Description: seqDB-24859:锁升级为 X 后，RC 不等锁的行为变为等锁
 * @Author Yang Qincheng
 * @Date 2021.12.16
 */
@Test( groups = "lockEscalation" )
public class Transaction24859 extends SdbTestBase {
    private Sequoiadb db1;
    private Sequoiadb db2;
    private final String clName = "cl_24859";

    @BeforeClass()
    public void setUp() {
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", 1 );
        sessionAttr.put( "TransLockWait", false );
        db1.setSessionAttr( sessionAttr );
        db2.setSessionAttr( sessionAttr );

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
    public void test(){
        db1.beginTransaction();
        db2.beginTransaction();
        try {
            LockEscalationUtil.insertData( db1, csName, clName, 20 );
            LockEscalationUtil.checkCLLockType( db1, LockEscalationUtil.LOCK_X );

            try {
                LockEscalationUtil.queryData( db2, csName, clName, 0, 10 );
                Assert.fail( "Timeout out exception expected!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            db1.commit();
            db2.commit();
        }
    }
}
