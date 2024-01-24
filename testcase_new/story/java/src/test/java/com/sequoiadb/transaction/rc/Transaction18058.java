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
 * @testcase seqDB-18058:同一个事务中对同一条记录进行多次修改
 * @date 2019-3-28
 * @author zhaoyu
 *
 */
@Test(groups = "rc")
public class Transaction18058 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_18058";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private DBCollection cl1 = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        if ( !db1.isClosed() ) {
            db1.close();
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
        // 开启事务
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        TransUtils.beginTransaction( db1 );

        // 插入记录R1及R2
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        BSONObject insertR2 = ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" );
        cl.insert( insertR1 );
        cl.insert( insertR2 );

        // 事务中更新记录R1为R3
        cl1.update( "{a:1}", "{$inc:{a:2}}", "{\"\":\"a\"}" );

        // 事务中更新记录R1为R4
        cl1.update( "{a:1}", "{$inc:{a:2}}", "{\"\":\"a\"}" );

        // 事务中更新记录R3为R5
        cl1.update( "{a:3}", "{$inc:{a:2}}", "{\"\":\"a\"}" );

        // 事务中更新记录R3为R5
        cl1.update( "{a:3}", "{$inc:{a:2}}", "{\"\":\"a\"}" );

        // 事务中表扫描查询
        expList.add( insertR2 );
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, a:5, b:1}" );
        expList.add( updateR1 );
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':null}", expList );

        // 事务中索引查询
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':'a'}", expList );

        // 事务1、2提交
        db1.commit();

        // 表扫描查询
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList );

        // 索引查询
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );
    }
}
