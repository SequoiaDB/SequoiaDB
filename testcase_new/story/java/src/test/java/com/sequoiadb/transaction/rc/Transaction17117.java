package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-17117:   更新多个索引，同时与读并发   
 * @author Zhao Xiaoni
 * @date 2019-1-21
 */
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rc")
public class Transaction17117 extends SdbTestBase {
    private String clName = "cl_17117";
    private Sequoiadb sdb = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private List< BSONObject > posExpList = new ArrayList< BSONObject >();
    private List< BSONObject > invExpList = new ArrayList< BSONObject >();
    private List< BSONObject > posInsertR1s = new ArrayList< BSONObject >();
    private List< BSONObject > invInsertR1s = new ArrayList< BSONObject >();

    // no limit for the number of transaction locks
    private BSONObject sessionAttr = new BasicBSONObject( "transmaxlocknum", -1 );

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        cl.createIndex( "ab", "{a:-1, b:1}", false, false );

        sdb.setSessionAttr( sessionAttr );
        db2.setSessionAttr( sessionAttr );
    }

    @Test
    public void test() {
        // 获取更新前逆序数据invInsertR1s
        List< BSONObject > insertR1s = new ArrayList< BSONObject >();
        insertR1s = TransUtils.getCompositeRecords( 0, 2000, 0, 10 );
        cl.insert( insertR1s );
        TransUtils.sortCompositeRecords( insertR1s, true );
        posInsertR1s.addAll( insertR1s );

        TransUtils.sortCompositeRecords( insertR1s, false );
        Collections.reverse( insertR1s );
        invInsertR1s.addAll( insertR1s );

        // 获取更新后正序数据posExpList
        posExpList.addAll( posInsertR1s );
        for ( int i = 0; i < posExpList.size(); i++ ) {
            BSONObject record = posExpList.get( i );
            int id = ( int ) record.get( "_id" ) + 15000;
            int a = ( int ) record.get( "a" ) + 15001;
            int b = ( int ) record.get( "b" ) - 10;
            posExpList.set( i, ( BSONObject ) JSON
                    .parse( "{_id:" + id + ", a:" + a + ", b:" + b + "}" ) );
        }
        TransUtils.sortCompositeRecords( posExpList, true );

        // 获取更新后逆序数据invExpList
        invExpList.addAll( posExpList );
        TransUtils.sortCompositeRecords( invExpList, false );
        Collections.reverse( invExpList );

        TransUtils.beginTransaction( db2 );

        ReadThread readThread = new ReadThread();
        readThread.start();

        // 事务1更新全部索引字段
        UpdateThread updateThread = new UpdateThread( readThread );
        updateThread.start();

        if ( !updateThread.isSuccess() ) {
            Assert.fail( updateThread.getErrorMsg() );
        }

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", posExpList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1, b:1}", "{'':'a'}", posExpList );

        // 事务2表扫描记录、正序
        TransUtils.queryAndCheck( cl2, "{a:1, b:1}", "{'':null}", posExpList );

        // 事务2表扫描记录、逆序
        TransUtils.queryAndCheck( cl2, "{a:-1, b:1}", "{'':null}", invExpList );

        // 事务2索引扫描记录、正序
        TransUtils.queryAndCheck( cl2, "{a:1, b:1}", "{'':'a'}", posExpList );

        // 事务2索引扫描记录、逆序
        TransUtils.queryAndCheck( cl2, "{a:-1, b:1}", "{'':'ab'}", invExpList );

        db2.commit();

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{a:1, b:1}", "{'':null}", posExpList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1, b:1}", "{'':'a'}", posExpList );
    }

    private class UpdateThread extends SdbThreadBase {
        private Sequoiadb db1 = null;
        private DBCollection cl1 = null;
        private ReadThread readThread = null;

        public UpdateThread( ReadThread readThread ) {
            this.readThread = readThread;
        }

        @Override
        public void exec() throws Exception {
            try {
                db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
                db1.setSessionAttr( sessionAttr );
                cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
                TransUtils.beginTransaction( db1 );

                cl1.update( "{$and:[{a:{$gte:0}},{a:{$lt:2000}}]}",
                        "{$inc:{_id:15000, a:15001, b:-10}}", "{'':'a'}" );

                // 事务1表扫描记录、正序
                TransUtils.queryAndCheck( cl1, "{a:1, b:1}", "{'':null}",
                        posExpList );

                // 事务1表扫描记录、逆序
                TransUtils.queryAndCheck( cl1, "{a:-1, b:1}", "{'':null}",
                        invExpList );

                // 事务1走"a"索引扫描记录、正序
                TransUtils.queryAndCheck( cl1, "{a:1, b:1}", "{'':'a'}",
                        posExpList );

                // 事务1走"ab"索引扫描记录、逆序
                TransUtils.queryAndCheck( cl1, "{a:-1, b:1}", "{'':'ab'}",
                        invExpList );

                if ( !readThread.isSuccess() ) {
                    Assert.fail( readThread.getErrorMsg() );
                }
                db1.commit();
            } catch ( BaseException e ) {
                Assert.fail( e.getMessage() );
            } finally {
                db1.close();
            }
        }
    }

    private class ReadThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            // 事务2表扫描记录、正序
            TransUtils.queryAndCheck( cl2, "{a:1, b:1}", "{'':null}",
                    posInsertR1s );

            // 事务2表扫描记录、逆序
            TransUtils.queryAndCheck( cl2, "{a:-1, b:1}", "{'':null}",
                    invInsertR1s );

            // 事务2走"a"索引扫描记录、正序
            TransUtils.queryAndCheck( cl2, "{a:1, b:1}", "{'':'a'}",
                    posInsertR1s );

            // 事务2走"ab"索引扫描记录、逆序
            TransUtils.queryAndCheck( cl2, "{a:-1, b:1}", "{'':'ab'}",
                    invInsertR1s );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !db2.isClosed() ) {
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
