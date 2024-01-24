package com.sequoiadb.transaction.unlock;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18421:事务加s锁后，事务2加u锁返回，事务3加x锁
 * @author yinzhen
 * @date 2019-6-12
 *
 */
@Test(groups = { "rc", "rs" })
public class Transaction18421 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private String clName = "cl18421";
    private String idxName = "textIndex18421";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        db2.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        db3.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName );
        cl.createIndex( idxName, "{a:1}", false, false );
        cl.insert( "{_id:1, a:1, b:1}" );
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
        if ( db3 != null ) {
            db3.commit();
            db3.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( clName );

        // 开启事务1，查询记录R1
        TransUtils.beginTransaction( db1 );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        DBCursor cursor = cl1.query( "{a:1}", "", "",
                "{'':'" + idxName + "'}" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertTrue(
                actList.size() == 1 && record.equals( actList.get( 0 ) ),
                "actList: " + actList );

        // 开启事务2，select for update R1
        TransUtils.beginTransaction( db2 );
        cursor = cl2.query( "{a:1}", "", "", "{'':'" + idxName + "'}",
                DBQuery.FLG_QUERY_FOR_UPDATE );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertTrue(
                actList.size() == 1 && record.equals( actList.get( 0 ) ),
                "actList: " + actList );

        // 开启事务3，删除记录R1
        TransUtils.beginTransaction( db3 );
        CL3Delete th3 = new CL3Delete();
        th3.start();
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, th3.getTransactionID() ) );

        // 待事务3等锁超时后，提交所有事务，再次开启事务，执行查询，检查结果
        Assert.assertFalse(
                th3.isSuccess() || ( int ) th3.getExecResult() != -13,
                th3.getErrorMsg() );
        db1.commit();
        db2.commit();
        db3.commit();
        TransUtils.beginTransaction( db1 );
        cursor = cl1.query();
        actList = TransUtils.getReadActList( cursor );
        Assert.assertTrue(
                actList.size() == 1 && record.equals( actList.get( 0 ) ),
                "actList: " + actList );
    }

    private class CL3Delete extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                // 判断事务阻塞需先获取事务id
                setTransactionID( db3 );

                DBCollection cl3 = db3.getCollectionSpace( csName )
                        .getCollection( clName );
                cl3.delete( "{a:1}", "{'':'" + idxName + "'}" );
            } catch ( BaseException e ) {
                setExecResult( e.getErrorCode() );
                throw e;
            }
        }
    }
}