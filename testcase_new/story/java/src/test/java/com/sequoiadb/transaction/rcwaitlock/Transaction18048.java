package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-18048:集合空间下存在多个集合，读写并发，事务回滚
 * @author xiaoni Zhao
 * @date 2019-3-28
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction18048 extends SdbTestBase {
    private List< BSONObject > insertR1s = new ArrayList< BSONObject >();

    @DataProvider(name = "provider_18048", parallel = true)
    public Object[][] dateProvider() {
        return new Object[][] { { "cl_18048A" }, { "cl_18048B" },
                { "cl_18048C" }, { "cl_18048D" }, { "cl_18048E" } };
    }

    @BeforeClass
    public void setUp() {
    }

    @Test(dataProvider = "provider_18048", invocationCount = 5)
    public void test( String clName ) throws InterruptedException {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .createCollection( clName );
            cl.createIndex( "a", "{a:1}", false, false );
            insertR1s = TransUtils.insertRandomDatas( cl, 0, 100 );

            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            TransUtils.beginTransaction( db1 );
            TransUtils.insertRandomDatas( cl1, 100, 200 );
            cl1.delete( null, "{'':'a'}" );

            Read read1 = new Read( clName, "{'':null}" );
            read1.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    read1.getTransactionID() ) );

            Read read2 = new Read( clName, "{'':'a'}" );
            read2.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    read2.getTransactionID() ) );

            db1.rollback();
            if ( !read1.isSuccess() || !read2.isSuccess() ) {
                Assert.fail( read1.getErrorMsg() + read2.getErrorMsg() );
            }

        } finally {
            db1.commit();
            if ( !db1.isClosed() ) {
                db1.close();
            }
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( !sdb.isClosed() ) {
                sdb.close();
            }
        }
    }

    private class Read extends SdbThreadBase {
        private Sequoiadb db = null;
        private String clName = null;
        private String hint = null;
        private DBCollection cl = null;
        private DBCursor cursor = null;

        public Read( String clName, String hint ) {
            this.clName = clName;
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                TransUtils.beginTransaction( db );
                cl = db.getCollectionSpace( csName ).getCollection( clName );

                // 判断事务阻塞需先获取事务id
                setTransactionID( db );

                cursor = cl.query( null, null, "{a : 1}", hint );
                List< BSONObject > records = TransUtils
                        .getReadActList( cursor );
                Assert.assertEquals( records, insertR1s );
                db.rollback();
            } finally {
                db.commit();
                cursor.close();
                db.close();
            }
        }
    }
}
