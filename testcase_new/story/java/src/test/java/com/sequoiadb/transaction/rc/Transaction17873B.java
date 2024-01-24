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
 * @testcase seqDB-17873:更新记录与$id索引扫描并发
 * @date 2019-3-1
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction17873B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17873B";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
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

        // 事务1更新记录更新_id字段
        cl1.update( "{_id:1}", "{$set:{_id:10}}", "{'':'$id'}" );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:10, a:1, b:1}" );
        List< BSONObject > expRecords = new ArrayList<>();
        expRecords.add( record );

        // 事务2读记录走表扫描
        TransUtils.queryAndCheck( cl2, "{'':null}", expList );

        // 事务2读记录走$id索引扫描
        TransUtils.queryAndCheck( cl2, "{a:{$exists:1}}", null, null,
                "{'':'$id'}", expList );

        // 事务1、2提交
        db1.commit();
        db2.commit();

        // 非事务表扫描
        TransUtils.queryAndCheck( cl, "{'':null}", expRecords );

        // 非事务索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null,
                "{'':'$id'}", expRecords );
    }
}
