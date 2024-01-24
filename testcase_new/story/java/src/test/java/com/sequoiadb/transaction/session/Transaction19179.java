package com.sequoiadb.transaction.session;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19179:TransTimeout属性支持会话级别
 * @author yinzhen
 * @date 2019年9月17日
 */
public class Transaction19179 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19179";

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
        Sequoiadb db2 = null;
        Sequoiadb db3 = null;
        Sequoiadb db4 = null;

        try {

            // 创建一个连接db1，开启事务，并插入1条记录R1
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            TransUtils.beginTransaction( db1 );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject obj = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl1.insert( obj );
            List< BSONObject > expList = new ArrayList<>();
            expList.add( obj );

            // 创建一个连接db2
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // 创建一个连接db3，并设置TransTimeout属性为30s，查询TransTimeout属性，开启事务，更新R1为R2
            db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db3.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransTimeout:30}" ) );
            BSONObject attr = db3.getSessionAttr();
            Assert.assertEquals( 30, attr.get( "TransTimeout" ) );
            TransUtils.beginTransaction( db3 );
            DBCollection cl3 = db3.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            long start = System.currentTimeMillis();
            try {
                cl3.update( "{a:1}", "{$set:{b:2}}", null );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -13 );
            }
            long end = System.currentTimeMillis();
            int useTime = ( int ) ( ( end - start ) / 1000 );
            if ( useTime > 31 || useTime < 29 ) {
                Assert.fail( "" + useTime );
            }

            // 在连接db2上，开启事务，更新R1为R2
            TransUtils.beginTransaction( db2 );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            start = System.currentTimeMillis();
            try {
                cl2.update( "{a:1}", "{$set:{b:2}}", null );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -13 );
            }
            end = System.currentTimeMillis();
            useTime = ( int ) ( ( end - start ) / 1000 );
            if ( useTime > 61 || useTime < 59 ) {
                Assert.fail( "" + useTime );
            }

            // 创建一个连接db4，开启事务，更新R1为R2
            db4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            TransUtils.beginTransaction( db4 );
            DBCollection cl4 = db4.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            start = System.currentTimeMillis();
            try {
                cl4.update( "{a:1}", "{$set:{b:2}}", null );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -13 );
            }
            end = System.currentTimeMillis();
            useTime = ( int ) ( ( end - start ) / 1000 );
            if ( useTime > 61 || useTime < 59 ) {
                Assert.fail( "" + useTime );
            }

            // 提交步骤1中的事务
            db1.commit();
            TransUtils.queryAndCheck( cl1, "{_id:1}", expList );

        } finally {
            if ( null != db1 ) {
                db1.commit();
                db1.close();
            }
            if ( null != db2 ) {
                db2.commit();
                db2.close();
            }
            if ( null != db3 ) {
                db3.commit();
                db3.close();
            }
            if ( null != db4 ) {
                db4.commit();
                db4.close();
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
