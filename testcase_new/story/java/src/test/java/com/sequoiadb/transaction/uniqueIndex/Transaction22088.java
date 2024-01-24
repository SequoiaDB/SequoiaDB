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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @testcase seqDB-22088:_id字段插入/更新为数组/regex类型报错
 * @date 2020-4-16
 * @author zhaoyu
 *
 */
public class Transaction22088 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl22088";
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
        cl.insert( ( BSONObject ) JSON.parse( "{a:1,b:1}" ) );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        // 插入_id字段为数组类型
        try {
            cl.insert( "{_id:[1,2,3]}" );
            Assert.fail( "Need throw error -6." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        // 插入_id字段为正则表达式类型
        try {
            cl.insert( "{_id:{$regex:'a'}}" );
            Assert.fail( "Need throw error -6." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        // 更新记录为_id字段为数组类型
        try {
            cl.update( "", "{$set:{_id:[1,2,3]}}", "" );
            Assert.fail( "Need throw error -358" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        // 更新记录为_id字段为正则表达式类型
        try {
            cl.update( "", "{$set:{_id:{$regex:'a'}}}", "" );
            Assert.fail( "Need throw error -6." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode() );
        }

    }
}
