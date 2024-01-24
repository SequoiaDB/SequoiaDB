package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-17116:  更新复合索引，同时与读并发 
 * @author xiaoni Zhao
 * @date 2019-1-21
 */
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rc")
public class Transaction17116 extends SdbTestBase {
    private String clName = "cl_17116";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
    }

    @DataProvider(name = "transaction17116")
    public Object[][] createIndex() {
        List< BSONObject > updateR1s = new ArrayList< BSONObject >();
        List< BSONObject > updateR2s = new ArrayList< BSONObject >();

        // indexKey为"{a:1, b:1}"时，读更新后记录的预期结果
        for ( int i = 1; i < 11; i++ ) {
            updateR2s.add( ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", b:" + ( i - 1 ) + "}" ) );
        }
        updateR1s.add( ( BSONObject ) JSON.parse( "{_id:0, a:6, b:-1}" ) );
        updateR1s.add( ( BSONObject ) JSON.parse( "{_id:11, a:4, b:2}" ) );
        updateR1s.addAll( TransUtils.getCompositeRecords( 3, 2000, 0, 10 ) );
        for ( int i = 11000; i < 12000; i++ ) {
            updateR1s.add( ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:5}" ) );
        }
        TransUtils.sortCompositeRecords( updateR1s, true );
        List< BSONObject > newExp1 = new ArrayList< BSONObject >();
        newExp1.addAll( updateR2s );
        newExp1.addAll( updateR1s );

        // indexKey为"{a:-1, b:1}"时，读更新后记录的预期结果
        List< BSONObject > newExp2 = new ArrayList< BSONObject >();
        TransUtils.sortCompositeRecords( updateR1s, false );
        Collections.reverse( updateR1s );
        newExp2.addAll( updateR1s );
        newExp2.addAll( updateR2s );

        // indexKey为"{a:-1, b:-1}"时，读更新后记录的预期结果
        TransUtils.sortCompositeRecords( updateR1s, true );
        Collections.reverse( updateR1s );
        Collections.reverse( updateR2s );
        List< BSONObject > newExp3 = new ArrayList< BSONObject >();
        newExp3.addAll( updateR1s );
        newExp3.addAll( updateR2s );

        // indexKey为"{a:1, b:-1}"时，读更新后记录的预期结果
        List< BSONObject > newExp4 = new ArrayList< BSONObject >();
        TransUtils.sortCompositeRecords( updateR1s, false );
        newExp4.addAll( updateR2s );
        newExp4.addAll( updateR1s );

        return new Object[][] { { "{a:1, b:1}", newExp1 },
                { "{a:-1, b:1}", newExp2 }, { "{a:-1, b:-1}", newExp3 },
                { "{a:1, b:-1}", newExp4 } };
    }

    @Test(dataProvider = "transaction17116")
    public void test( String indexKey, List< BSONObject > newExp ) {
        try {
            cl.createIndex( "a", indexKey, false, false );
            List< BSONObject > insertRs = getData();
            cl.insert( insertRs );
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );

            sortOldExp( indexKey, insertRs );
            ReadThread readThread = new ReadThread( indexKey, insertRs );
            readThread.start();

            UpdateThread updateThread = new UpdateThread( indexKey, newExp );
            updateThread.start();

            if ( !readThread.isSuccess() || !updateThread.isSuccess() ) {
                Assert.fail(
                        readThread.getErrorMsg() + updateThread.getErrorMsg() );
            } else {
                db1.commit();
            }

            // 非事务表扫描记录
            TransUtils.queryAndCheck( cl, indexKey, "{'':null}", newExp );

            // 非事务索引扫描记录
            TransUtils.queryAndCheck( cl, indexKey, "{'':'a'}", newExp );

            db2.commit();

            // 非事务表扫描记录
            TransUtils.queryAndCheck( cl, indexKey, "{'':null}", newExp );

            // 非事务索引扫描记录
            TransUtils.queryAndCheck( cl, indexKey, "{'':'a'}", newExp );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            db1.commit();
            db2.commit();
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            cl.truncate();
        }
    }

    private class UpdateThread extends SdbThreadBase {
        private DBCollection cl1 = null;
        private String indexKey = null;
        private List< BSONObject > newExp = null;

        public UpdateThread( String indexKey, List< BSONObject > newExp ) {
            this.indexKey = indexKey;
            this.newExp = newExp;
        }

        @Override
        public void exec() throws Exception {
            try {
                cl1 = db1.getCollectionSpace( csName ).getCollection( clName );

                cl1.update( "{a:0}", "{$inc:{a:6, b:-1}}", "{'':'a'}" );
                cl1.update( "{a:1}", "{$unset:{a:1}}", "{'':'a'}" );
                cl1.update( "{a:2}", "{$set:{a:4}}", "{'':'a'}" );
                cl1.update( "{b:{$isnull:1}, a:{$gte: 9000, $lt: 12000}}",
                        "{$set:{b:5}}", null );

                // 事务1表扫描记录
                TransUtils.queryAndCheck( cl1, indexKey, "{'':null}", newExp );

                // 事务1索引扫描记录
                TransUtils.queryAndCheck( cl1, indexKey, "{'':'a'}", newExp );
            } catch ( BaseException e ) {
                Assert.fail( e.getMessage() );
            }
        }
    }

    private class ReadThread extends SdbThreadBase {
        private String indexKey = null;
        private List< BSONObject > oldExp = null;

        public ReadThread( String indexKey, List< BSONObject > oldExp ) {
            this.indexKey = indexKey;
            this.oldExp = oldExp;
        }

        @Override
        public void exec() throws Exception {
            // 事务2表扫描记录
            TransUtils.queryAndCheck( cl2, indexKey, "{'':null}", oldExp );

            // 事务2走索引扫描记录
            TransUtils.queryAndCheck( cl2, indexKey, "{'':'a'}", oldExp );
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
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

    private void sortOldExp( String indexKey, List< BSONObject > oldExp ) {
        BSONObject object = ( BSONObject ) JSON.parse( indexKey );
        int a = ( int ) object.get( "a" );
        int b = ( int ) object.get( "b" );
        if ( a == 1 && b == 1 ) {
            TransUtils.sortCompositeRecords( oldExp, true );
        } else if ( a == 1 && b == -1 ) {
            TransUtils.sortCompositeRecords( oldExp, false );
        } else if ( a == -1 && b == 1 ) {
            TransUtils.sortCompositeRecords( oldExp, false );
            Collections.reverse( oldExp );
        } else if ( a == -1 && b == -1 ) {
            TransUtils.sortCompositeRecords( oldExp, true );
            Collections.reverse( oldExp );
        }
    }

    private List< BSONObject > getData() {
        List< BSONObject > insertRs = TransUtils.getCompositeRecords( 0, 2000,
                0, 10 );
        for ( int i = 11000; i < 12000; i++ ) {
            insertRs.add( ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + "}" ) );
        }
        return insertRs;
    }
}
