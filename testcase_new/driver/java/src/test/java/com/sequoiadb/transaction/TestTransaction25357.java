package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-25357:事务中指定SDB_FLG_QUERY_FOR_SHARE查询
 * @Description seqDB-25358:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
 * @Author liuli
 * @Date 2022.02.12
 * @UpdateAuthor liuli
 * @UpdateDate 2022.02.12
 * @version 1.10
 */
public class TestTransaction25357 extends SdbTestBase {
    private Sequoiadb sdb;
    private DBCollection dbcl;
    private String csName = "cs_25357";
    private String clName = "cl_25357";
    private String indexName = "index_25357";
    private ArrayList< BSONObject > insertRecods = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace(csName);
        dbcl = dbcs.createCollection( clName );
        int recordNum = 20;
        insertRecods = insertData( dbcl, recordNum );
        BasicBSONObject indexKeys = new BasicBSONObject();
        indexKeys.put( "a", 1 );
        dbcl.createIndex( indexName, indexKeys, null );
    }

    @DataProvider(name = "sessionAttr")
    public Object[][] configs() {
        return new Object[][] { { 0 }, { 1 } };
    }

    @Test(dataProvider = "sessionAttr")
    public void test( int transIsolation ) {
        DBCursor cursor;
        int lockCount = 0;
        // 修改会话属性
        BasicBSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransIsolation", transIsolation );
        sessionAttr.put( "TransMaxLockNum", 10 );
        sdb.setSessionAttr( sessionAttr );

        // 开启事务后查询10条数据
        sdb.beginTransaction();
        BasicBSONObject hint = new BasicBSONObject();
        hint.put( "", indexName );
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "a", new BasicBSONObject( "$lt", 10 ) );
        cursor = dbcl.query( matcher, null, null, hint );
        while ( cursor.hasNext() ) {
            cursor.getNext();
        }
        cursor.close();
        // 校验没有记录锁为S锁
        lockCount = Util.getCLLockCount( sdb, Util.LOCK_S );
        Assert.assertEquals( lockCount, 0 );

        // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询10条数据
        cursor = dbcl.query( matcher, null, null, hint,
                DBQuery.FLG_QUERY_FOR_SHARE );
        while ( cursor.hasNext() ) {
            cursor.getNext();
        }
        cursor.close();
        // 校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
        lockCount = Util.getCLLockCount( sdb, Util.LOCK_S );
        Assert.assertEquals( lockCount, 10 );
        Util.checkIsLockEscalated( sdb, false );
        Util.checkCLLockType( sdb, Util.LOCK_IS );

        // 不指定flags查询后10条数据
        matcher.clear();
        matcher.put( "a", new BasicBSONObject( "$gte", 10 ) );
        cursor = dbcl.query( matcher, null, null, hint );
        while ( cursor.hasNext() ) {
            cursor.getNext();
        }
        cursor.close();
        // 记录锁数量不变，集合锁不变，没有发生锁升级
        lockCount = Util.getCLLockCount( sdb, Util.LOCK_S );
        Assert.assertEquals( lockCount, 10 );
        Util.checkIsLockEscalated( sdb, false );
        Util.checkCLLockType( sdb, Util.LOCK_IS );

        // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
        cursor = dbcl.query( matcher, null, null, hint,
                DBQuery.FLG_QUERY_FOR_SHARE );
        while ( cursor.hasNext() ) {
            cursor.getNext();
        }
        cursor.close();

        // 发生锁升级，集合锁为S锁
        Util.checkIsLockEscalated( sdb, true );
        Util.checkCLLockType( sdb, Util.LOCK_S );

        // 事务中指定flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验
        cursor = dbcl.query( null, null, new BasicBSONObject( "a", 1 ), null,
                DBQuery.FLG_QUERY_FOR_SHARE );
        Util.checkRecords( insertRecods, cursor );

        // 提交事务
        sdb.commit();

        // seqDB-25358:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
        cursor = dbcl.query( null, null, new BasicBSONObject( "a", 1 ), null,
                DBQuery.FLG_QUERY_FOR_SHARE );
        Util.checkRecords( insertRecods, cursor );
    }

    @AfterClass
    public void tearDown() {
        sdb.dropCollectionSpace( csName );
        sdb.close();
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        Random random = new Random();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "b", i );
            obj.put( "c", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }
}