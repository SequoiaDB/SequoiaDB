package com.sequoiadb.transaction.ru;

/**
 * @Description seqDB-17193:增删改记录与读记录并发，事务回滚
 * @author xiaoni Zhao
 * @date 2019-1-15
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "ru")
public class Transaction17193 extends SdbTestBase {
    private String clName = "cl_17193";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hintIxScan = "{'':'a'}";
    private String hintTbScan = "{'':null}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() {

        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1插入记录R1
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1,a:1,b:1}" );
        cl1.insert( insertR1 );
        expList.add( insertR1 );

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        db1.rollback();

        // 事务2表扫描记录
        TransUtils.queryAndCheck( cl2, hintTbScan,
                new ArrayList< BSONObject >() );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan,
                new ArrayList< BSONObject >() );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan,
                new ArrayList< BSONObject >() );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan,
                new ArrayList< BSONObject >() );

        // 事务1更新记录R1
        cl1.insert( insertR1 );
        TransUtils.beginTransaction( db1 );
        cl1.update( "", "{$inc:{a:1}}", "{'':'a'}" );

        // 事务2表扫描记录
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1,a:2,b:1}" ) );
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        db1.rollback();

        // 事务2表扫描记录
        expList.clear();
        expList.add( insertR1 );
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        // 事务1删除记录R1
        TransUtils.beginTransaction( db1 );
        cl1.delete( "", "{'':'a'}" );

        // 事务2表扫描记录
        expList.clear();
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        db1.rollback();

        // 事务2表扫描记录
        expList.clear();
        expList.add( insertR1 );
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2索引扫描记录
        TransUtils.queryAndCheck( cl2, hintIxScan, expList );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        db2.rollback();
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
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
