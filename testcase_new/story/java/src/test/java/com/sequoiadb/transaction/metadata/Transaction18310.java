package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18310:集合上存在事务操作，并发创建/删除集合
 * @date 2019-4-25
 * @author yinzhen
 *
 */
public class Transaction18310 extends SdbTestBase {
    private CountDownLatch latch;

    @BeforeClass
    public void setUp() {
    }

    @AfterClass
    public void tearDown() {
    }

    @Test
    public void test() {
        latch = new CountDownLatch( 5 );

        List< CreateAndDropCLTh > thList = new ArrayList< >();
        for ( int i = 0; i < 5; i++ ) {
            CreateAndDropCLTh th = new CreateAndDropCLTh( "cl18310_" + i );
            thList.add( th );
            th.start();
        }
        for ( CreateAndDropCLTh createAndDropCLTh : thList ) {
            Assert.assertTrue( createAndDropCLTh.isSuccess(),
                    createAndDropCLTh.getErrorMsg() );
        }

        try {
            latch.await();
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }

    class CreateAndDropCLTh extends SdbThreadBase {
        private String clName;

        private CreateAndDropCLTh( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            CollectionSpace cs = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cs = db.getCollectionSpace( csName );
                int doTimes = 0;
                while ( true ) {
                    DBCollection cl = cs.createCollection( clName );
                    cl.createIndex( "idx", "{a:1}", false, false );
                    cl.insert(
                            ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );

                    // 开启事务，对该记录进行更新后删除，提交事务
                    TransUtils.beginTransaction( db );
                    cl.update( "{a:1}", "{$set:{a:10}}", "{'':'idx'}" );
                    cl.delete( "{a:10}", "{'':'idx'}" );
                    db.commit();

                    // 删除集合
                    cs.dropCollection( clName );
                    if ( ++doTimes == 30 ) {
                        break;
                    }
                }
            } finally {
                db.commit();
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
                }
                db.close();
                latch.countDown();
            }
        }
    }
}
