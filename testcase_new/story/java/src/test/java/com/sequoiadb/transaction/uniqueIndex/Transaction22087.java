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
 * @testcase seqDB-22087:事务中不存在_id索引，执行事务写操作
 * @date 2020-4-16
 * @author zhaoyu
 *
 */
public class Transaction22087 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl22087";
    private DBCollection cl = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{AutoIndexId:false}" ) );
        cl.insert( ( BSONObject ) JSON.parse( "{a:1,b:1}" ) );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        try {
            TransUtils.beginTransaction( sdb );
            // 插入
            try {
                cl.insert( ( BSONObject ) JSON.parse( "{a:1,b:1}" ) );
                Assert.fail( "Need throw error -279." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -279 );
            }

            // 更新
            try {
                cl.update( "", "{$inc:{a:1}}", "" );
                Assert.fail( "Need throw error -279." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -279 );
            }

            // 删除
            try {
                cl.delete( "" );
                Assert.fail( "Need throw error -279." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -279 );
            }

        } finally {
            sdb.commit();
        }

    }
}
