package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-17081:  多个原子操作与读并发，事务回滚
 * @author Zhao Xiaoni
 * @date 2019-1-21
 */
import java.util.ArrayList;
import java.util.List;

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

@Test(groups = "rc")
public class Transaction17081 extends SdbTestBase {
    private String clName = "cl_17081";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb dbT = null;
    private Sequoiadb dbI = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > insertR1s = new ArrayList< BSONObject >();
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'a'}";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() {
        insertR1s = TransUtils.insertRandomDatas( cl, 0, 10 );

        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1执行多个原子操作
        Operation operation = new Operation();
        operation.start();

        // 事务2并发表扫描
        dbT = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        TransUtils.beginTransaction( dbT );
        Read read1 = new Read( dbT, hintTbScan );
        read1.start();

        // 事务2并发索引扫描
        dbI = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        TransUtils.beginTransaction( dbI );
        Read read2 = new Read( dbI, hintIxScan );
        read2.start();

        if ( !read1.isSuccess() || !read2.isSuccess()
                || !operation.isSuccess() ) {
            Assert.fail( read1.getErrorMsg() + read2.getErrorMsg()
                    + operation.getErrorMsg() );
        }

        // 非事务表扫描
        TransUtils.queryAndCheck( cl, "{a:1}", hintTbScan, expList );

        // 非事务索引扫描
        TransUtils.queryAndCheck( cl, "{a:1}", hintIxScan, expList );

        db1.rollback();

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", hintTbScan, insertR1s );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", hintIxScan, insertR1s );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", hintTbScan, insertR1s );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", hintIxScan, insertR1s );
        db2.rollback();
    }

    private class Operation extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            BSONObject insertR = ( BSONObject ) JSON
                    .parse( "{_id:20000,a:20000,b:20000}" );
            for ( int i = 0; i < 100; i++ ) {
                cl1.insert( insertR );
                cl1.update( "{a:20000}", "{$set:{a:20001}}", "{'':'a'}" );
                cl1.delete( "{a:20001}", "{'':'a'}" );

                cl1.delete( "{a:" + i + "}", "{'':'a'}" );
                cl1.insert( ( BSONObject ) JSON.parse( "{_id:" + ( 10000 + i )
                        + ", a:" + i + ",b:" + i + "}" ) );
                cl1.update( "{a:" + i + "}", "{$set:{a:" + ( i + 10000 ) + "}}",
                        "{'':'a'}" );
                expList.add( ( BSONObject ) JSON.parse( "{_id:" + ( 10000 + i )
                        + ", a:" + ( i + 10000 ) + ",b:" + i + "}" ) );
            }
        }
    }

    private class Read extends SdbThreadBase {
        private Sequoiadb db2 = null;
        private DBCollection cl2 = null;
        private String hint = null;

        public Read( Sequoiadb db2, String hint ) {
            this.db2 = db2;
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            // 事务2扫描记录
            for ( int i = 0; i < 25; i++ ) {
                TransUtils.queryAndCheck( cl2, "{a:1}", hint, insertR1s );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        dbT.commit();
        dbI.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !dbT.isClosed() ) {
            db1.close();
        }
        if ( !dbI.isClosed() ) {
            db2.close();
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
