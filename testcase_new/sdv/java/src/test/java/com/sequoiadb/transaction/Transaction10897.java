package com.sequoiadb.transaction;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * test content: 重复多次commit/rollback testlink-case: seqDB-10897
 * 
 * @author wangkexin
 * @Date 2019.04.08
 * @version 1.00
 */

public class Transaction10897 extends SdbTestBase {
    private String clName = "cl10897";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    @BeforeClass
    private void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // test commit
        try {
            sdb.beginTransaction();
            DBCollection cl1 = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject record = new BasicBSONObject();
            record.put( "a", 1 );
            cl1.insert( record );

            for ( int i = 0; i < 500; i++ ) {
                sdb.commit();
            }
            CheckResult( record );

            // test rollback
            sdb.beginTransaction();
            DBCollection cl2 = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject record2 = new BasicBSONObject();
            record2.put( "a", 2 );
            cl2.insert( record2 );

            for ( int i = 0; i < 500; i++ ) {
                sdb.rollback();
            }
            CheckResult( record );

        } finally {
            sdb.commit();
        }

    }

    @AfterClass
    private void teardown() {
        try {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    private void CheckResult( BSONObject record ) {
        long expCount = 1;
        long actCount = cl.getCount();
        Assert.assertEquals( actCount, expCount );
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ) {
            Assert.assertEquals( cursor.getNext(), record );
        }
    }
}