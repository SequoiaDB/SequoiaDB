package com.sequoiadb.transaction.uniqueIndex;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22085:删除_id索引，对集合加s锁，锁兼容/锁升级验证
 * @date 2020-4-16
 * @author zhaoyu
 *
 */
public class Transaction22085 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl22085";
    private String idxName = "idx22085";
    private DBCollection cl = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl.createIndex( idxName + "_a", "{a:1}", true, true );
        cl.createIndex( idxName + "_b", "{b:1}", true, true );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        // 开启事务，插入记录
        Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            TransUtils.beginTransaction( db1 );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            cl1.insert( ( BSONObject ) JSON.parse( "{a:1,b:1}" ) );

            // 删除_id索引拿s锁，锁升级验证
            try {
                cl1.dropIdIndex();
                Assert.fail( "Need throw error -336." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -336 );
            }

            // 删除_id索引拿s锁，锁兼容验证
            try {
                cl.dropIdIndex();
                Assert.fail( "Need throw error -190." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -190 );
            }

            try {
                TransUtils.beginTransaction( sdb );
                cl.dropIdIndex();
                Assert.fail( "Need throw error -190." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -190 );
            }

            // 本连接或者其他连接删除普通唯一索引成功
            cl1.dropIndex( idxName + "_a" );
            cl1.dropIndex( idxName + "_b" );

        } finally {
            sdb.commit();
            db1.commit();
            db1.close();
        }

    }
}
