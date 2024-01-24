package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17110:读并发与更新并发
 * @date 2019-1-18
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction17110 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17110";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'a'}";

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}" }, { "{'a': -1, 'b': 1}" }, };
    }

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
    }

    @AfterClass
    public void tearDown() {
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) {
        try {
            cl.createIndex( "a", indexKey, false, false );
            BSONObject R1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl.insert( R1 );

            // 1 开启并发事务
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );

            // 2 事务1读记录走表扫描
            expList.clear();
            expList.add( R1 );
            TransUtils.queryAndCheck( cl1, hintTbScan, expList );

            // 事务1读记录走索引扫描
            TransUtils.queryAndCheck( cl1, hintIxScan, expList );

            // 2 事务1逆序读记录走表扫描
            TransUtils.queryAndCheck( cl1, "{'a': -1}", hintTbScan, expList );

            // 事务1逆序读记录走索引扫描
            TransUtils.queryAndCheck( cl1, "{'a': -1}", hintIxScan, expList );

            // 3 事务2读记录走表扫描
            TransUtils.queryAndCheck( cl2, hintTbScan, expList );

            // 事务2读记录走索引扫描
            TransUtils.queryAndCheck( cl2, hintIxScan, expList );

            // 4 事务2逆序读记录走表扫描
            TransUtils.queryAndCheck( cl2, "{'a': -1}", hintTbScan, expList );

            // 事务2逆序读记录走索引扫描
            TransUtils.queryAndCheck( cl2, "{'a': -1}", hintIxScan, expList );

            // 5 事务3更新记录
            cl3.update( "{a:1}", "{$set:{a:4}}", hintIxScan );
            expList.clear();
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:1, a:4, b:1}" );
            expList.add( record );

            // 6 事务3读记录走表扫描
            TransUtils.queryAndCheck( cl3, hintTbScan, expList );

            // 事务3读记录走索引扫描
            TransUtils.queryAndCheck( cl3, hintIxScan, expList );

            // 6 事务3逆序读记录走表扫描
            TransUtils.queryAndCheck( cl3, "{'a': -1}", hintTbScan, expList );

            // 事务3逆序读记录走索引扫描
            TransUtils.queryAndCheck( cl3, "{'a': -1}", hintIxScan, expList );

            // 7 提交事务1和事务2
            db1.commit();
            db2.commit();

            // 7 非事务表扫描
            TransUtils.queryAndCheck( cl, hintTbScan, expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl, hintIxScan, expList );

            // 7 非事务表扫描逆序读
            TransUtils.queryAndCheck( cl, "{'a': -1}", hintTbScan, expList );

            // 非事务索引扫描逆序读
            TransUtils.queryAndCheck( cl, "{'a': -1}", hintIxScan, expList );

            // 8 事务3提交
            db3.commit();

            // 8 非事务表扫描
            TransUtils.queryAndCheck( cl, hintTbScan, expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl, hintIxScan, expList );

            // 8 非事务表扫描逆序读
            TransUtils.queryAndCheck( cl, "{'a': -1}", hintTbScan, expList );

            // 非事务索引扫描逆序读
            TransUtils.queryAndCheck( cl, "{'a': -1}", hintIxScan, expList );

            // 删除记录
            cl.delete( null, hintIxScan );
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            cl.truncate();
        }
    }
}
