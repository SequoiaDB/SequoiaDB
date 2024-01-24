package com.sequoiadb.transaction.session;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19185:事务中设置事务属性
 * @author yinzhen
 * @date 2019年9月18日
 */
@Test(groups = "ru")
public class Transaction19185 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19185";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;

        try {

            // 创建一个连接db1，开启事务，设置事务属性，覆盖：Timeout、TransIsolation、TransTimeout、
            // TransUseRBS、TransLockWait、TransAutoCommit、TransAutoRollback、TransRCCount，查询事务属性
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            TransUtils.beginTransaction( db1 );
            db1.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransTimeout:120}" ) );
            try {
                db1.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransIsolation:1}" ) );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6 );
            }
            try {
                db1.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransUseRBS:false}" ) );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6 );
            }
            try {
                db1.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransLockWait:true}" ) );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6 );
            }
            try {
                db1.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransAutoCommit:true}" ) );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6 );
            }
            try {
                db1.setSessionAttr( ( BSONObject ) JSON
                        .parse( "{TransAutoRollback:false}" ) );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6 );
            }
            try {
                db1.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransRCCount:false}" ) );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6 );
            }
            BSONObject attr = db1.getSessionAttr();
            BSONObject expAttr = ( BSONObject ) JSON.parse(
                    "{TransTimeout:120, TransIsolation:0, TransUseRBS:true, TransLockWait:false, TransAutoCommit:false, TransAutoRollback:true, TransRCCount: true}" );
            for ( String key : expAttr.keySet() ) {
                Assert.assertEquals( attr.get( key ), expAttr.get( key ) );
            }
        } finally {
            if ( null != db1 ) {
                db1.commit();
                db1.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( null != sdb ) {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
            sdb.close();
        }
    }
}
