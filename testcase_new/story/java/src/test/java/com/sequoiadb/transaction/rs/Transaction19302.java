package com.sequoiadb.transaction.rs;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-19302:并发事务，s锁升级为u锁，加入锁升级队列
 * @date 2019-9-9
 * @author zhaoyu
 *
 */
@Test(groups = { "rs" })
public class Transaction19302 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private DBCollection cl1;
    private DBCollection cl2;
    private String clName = "cl19302";
    private String idxName = "idx19302";
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'" + idxName + "'}";
    private DBCollection cl;
    private ArrayList< BSONObject > expList = new ArrayList<>();
    private String recordR1 = "{_id:'cl19302', a:1}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        // 创建索引，插入记录
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( idxName, "{a:1}", true, true );
    }

    @AfterClass
    public void tearDown() {
        if ( db1 != null ) {
            db1.commit();
            db1.close();
        }
        if ( db2 != null ) {
            db2.commit();
            db2.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
            sdb.close();
        }
    }

    @DataProvider(name = "hint")
    public Object[][] HintIndex() {
        return new Object[][] { { hintTbScan }, { hintIxScan } };

    }

    @Test(dataProvider = "hint")
    public void test( String hint ) throws Exception {
        try {
            // 插入记录R1
            cl.insert( recordR1 );
            expList.clear();
            expList.add( ( BSONObject ) JSON.parse( recordR1 ) );

            // 开启两个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );

            // 事务1查询R1
            TransUtils.queryAndCheck( cl1, hint, expList );

            // 事务2查询R1
            TransUtils.queryAndCheck( cl2, hint, expList );

            // 事务1升级s锁为u锁，升级成功
            DBCursor cursor = cl1.query( "", "", "", hint,
                    DBQuery.FLG_QUERY_FOR_UPDATE );
            ArrayList< BSONObject > actualList = TransUtils
                    .getReadActList( cursor );
            Assert.assertEquals( actualList, expList );

            // 事务2升级s锁为u锁，阻塞等锁
            Cl2Query cl2Query = new Cl2Query( hint );
            cl2Query.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    cl2Query.getTransactionID() ) );

            // 事务1u锁升级为x锁，升级失败，事务回滚
            try {
                cl1.update( "", "{$set:{a:2}}", hint );
                throw new Exception( "need error" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
            Assert.assertTrue( cl2Query.isSuccess(), cl2Query.getErrorMsg() );

            // 事务1提交
            db2.commit();

            // 查询记录
            TransUtils.queryAndCheck( cl, hint, expList );
        } finally {
            db2.commit();
            cl.truncate();
        }

    }

    private class Cl2Query extends SdbThreadBase {
        private String hint;

        private Cl2Query( String hint ) {
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            DBCursor cursor = cl2.query( "", "", "", hint,
                    DBQuery.FLG_QUERY_FOR_UPDATE );
            ArrayList< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject record = cursor.getNext();
                actualList.add( record );
            }
            cursor.close();
            Assert.assertEquals( actualList, expList );
        }
    }
}
