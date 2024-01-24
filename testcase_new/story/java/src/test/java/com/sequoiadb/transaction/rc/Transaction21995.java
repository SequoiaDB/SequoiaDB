package com.sequoiadb.transaction.rc;

import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;
/**
 * @testcase seqDB-21995:事务中使用count查询记录数，检查此事务不持有事务锁
 * @date 2020-03-31
 * @author zhaoxiaoni
 *
 */

@Test(groups = "rc")
public class Transaction21995 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private String clName = "cl21995";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.insert( "{_id: 1, a: 1, b: 1 }" );
    }

    @Test
    public void test() throws InterruptedException {
        TransUtils.beginTransaction( sdb );
        cl.getCount();
        DBCursor cursor = sdb.getSnapshot(
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", null, null );
        while ( cursor.hasNext() ) {
            BasicBSONList gotLocks = ( BasicBSONList ) cursor.getNext()
                    .get( "GotLocks" );
            Assert.assertEquals( gotLocks.size(), 0 );
        }
        sdb.commit();
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.commit();
            sdb.getCollectionSpace( csName ).dropCollection( clName );
            sdb.close();
        }
    }
}
