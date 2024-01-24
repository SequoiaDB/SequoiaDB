package com.sequoiadb.transaction.rc;

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

/**
 * @testcase seqDB-17085:插入相同的记录与读并发,事务回滚
 * @date 2019-1-18
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction17085 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17085";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'textIndex17085'}";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17085", "{a:1}", false, false );
        BSONObject record = ( BSONObject ) JSON.parse( "{a:1, b:1}" );
        cl.insert( record );
        expList.add( record );
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

    @Test
    public void test() {
        // 开启2个并发事务
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1删除记录，并插入相同的记录
        cl1.delete( "", hintIxScan );
        BSONObject record = ( BSONObject ) JSON.parse( "{a:1, b:1}" );
        cl1.insert( record );
        List< BSONObject > expRecords = new ArrayList<>();
        expRecords.add( record );

        // 事务2读记录走表扫描
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2读记录走索引扫描
        TransUtils.queryAndCheck( cl2, "{a:{$exists:1}}", null, null,
                hintIxScan, expList );

        // 非事务表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expRecords );

        // 非事务索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expRecords );

        // 事务1回滚
        db1.rollback();

        // 事务2读记录走表扫描
        TransUtils.queryAndCheck( cl2, hintTbScan, expList );

        // 事务2读记录走索引扫描
        TransUtils.queryAndCheck( cl2, "{a:{$exists:1}}", null, null,
                hintIxScan, expList );

        // 非事务表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expList );

        // 事务2回滚
        db2.rollback();

        // 非事务表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 非事务索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expList );
    }
}
