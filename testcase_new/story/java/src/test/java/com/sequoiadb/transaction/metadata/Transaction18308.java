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

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18308:集合空间上存在事务操作，并发创建/删除集合空间
 * @date 2019-4-25
 * @author yinzhen
 *
 */
public class Transaction18308 extends SdbTestBase {
    private String clName = "cl18308";
    private CountDownLatch latch;

    @BeforeClass
    public void setUp() {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        for ( int i = 0; i < 10; i++ ) {
            String csName = "cs18308_" + i;
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @AfterClass
    public void tearDown() {
    }

    @Test
    public void test() {
        latch = new CountDownLatch( 5 );

        List< CreateAndDropCSTh > thList = new ArrayList<>();
        for ( int i = 0; i < 5; i++ ) {
            CreateAndDropCSTh th = new CreateAndDropCSTh( "cs18308_" + i );
            thList.add( th );
            th.start();
        }

        for ( CreateAndDropCSTh createAndDropCSTh : thList ) {
            Assert.assertTrue( createAndDropCSTh.isSuccess(),
                    createAndDropCSTh.getErrorMsg() );
        }

        try {
            latch.await();
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }

    class CreateAndDropCSTh extends SdbThreadBase {
        private String csName;

        private CreateAndDropCSTh( String csName ) {
            this.csName = csName;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                int doTimes = 0;
                while ( true ) {
                    DBCollection cl = db.createCollectionSpace( csName )
                            .createCollection( clName );
                    cl.createIndex( "idx", "{a:1}", false, false );
                    cl.insert(
                            ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );

                    // 开启事务，对该记录进行更新后删除，提交事务
                    TransUtils.beginTransaction( db );
                    cl.update( "{a:1}", "{$set:{a:10}}", "{'':'idx'}" );
                    cl.delete( "{a:10}", "{'':'idx'}" );
                    db.commit();

                    // 删除集合空间,由于后台清理记录的线程会对集合空间加锁，且是异步的，需要规避该错误码，
                    TransUtils.dropCS( db, csName );

                    if ( ++doTimes == 30 ) {
                        System.out.println(
                                "CSNAME: " + csName + " doTimes: " + doTimes );
                        break;
                    }
                }
            } finally {
                // The db is closed when a network error occurs
                if ( db != null && !db.isClosed() ){
                    db.commit();
                    if ( db.isCollectionSpaceExist( csName ) ) {
                        db.dropCollectionSpace( csName );
                    }
                    db.close();
                }
                latch.countDown();
            }
        }
    }
}
