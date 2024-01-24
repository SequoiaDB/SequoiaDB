package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-18047:集合空间下存在多个集合，读写并发，事务回滚
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

@Test(groups = "rc")
public class Transaction18047 extends SdbTestBase {
    private List< BSONObject > insertR1s = new ArrayList< BSONObject >();

    @DataProvider(name = "provider_18047", parallel = true)
    public Object[][] dateProvider() {
        return new Object[][] { { "cl_18047A" }, { "cl_18047B" },
                { "cl_18047C" }, { "cl_18047D" }, { "cl_18047E" } };
    }

    @BeforeClass
    public void setUp() {
    }

    @Test(dataProvider = "provider_18047", invocationCount = 5)
    public void test( String clName ) throws InterruptedException {
        Sequoiadb sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        Sequoiadb db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        Sequoiadb db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        try {

            DBCollection cl = sdb.getCollectionSpace( csName )
                    .createCollection( clName );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            cl.createIndex( "a", "{a:1}", false, false );
            insertR1s = TransUtils.insertRandomDatas( cl, 0, 100 );

            TransUtils.beginTransaction( db1 );
            cl1.delete( null, "{'':'a'}" );

            TransUtils.beginTransaction( db2 );

            Operation operation2 = new Operation( cl2 );
            operation2.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    operation2.getTransactionID() ) );

            db1.rollback();
            if ( !operation2.isSuccess() ) {
                Assert.fail( operation2.getErrorMsg() );
            }

            db2.rollback();
            Read read1 = new Read( clName, "{'':null}" );
            read1.start();

            Read read2 = new Read( clName, "{'':'a'}" );
            read2.start();

            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

        } finally {
            db1.commit();
            db2.commit();
            if ( !db1.isClosed() ) {
                db1.close();
            }
            if ( !db2.isClosed() ) {
                db2.close();
            }
            if ( !sdb.isClosed() ) {
                sdb.close();
            }
        }
    }

    private class Operation extends SdbThreadBase {
        private DBCollection cl = null;

        public Operation( DBCollection cl ) {
            this.cl = cl;
        }

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            cl.delete( null, "{'':'a'}" );
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
            db = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            try {
                TransUtils.beginTransaction( db );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                cursor = cl.query( null, null, "{a : 1}", hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        insertR1s );
                db.rollback();
                cursor.close();
            } finally {
                db.commit();
                db.close();
            }
        }
    }
}
