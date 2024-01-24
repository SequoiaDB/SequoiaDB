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
 * @description seqDB-19183:TransAutoRollback属性支持会话级别
 * @author yinzhen
 * @date 2019年9月18日
 */
public class Transaction19183 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19183";
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
        cl.createIndex( "idx_19183", "{a:1}", true, false );
        cl.insert( "{_id:1, a:1, b:1}" );
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        Sequoiadb db3 = null;

        try {

            // 创建一个连接db1
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // 创建一个连接db2，并设置TransAutoRollback属性为false，查询TransAutoRollback属性，
            // 开启事务，插入记录R1与集合中已存在的记录唯一索引重复，再次插入记录R2成功，回滚事务
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransAutoRollback:false}" ) );
            BSONObject attr = db2.getSessionAttr();
            Assert.assertEquals( false, attr.get( "TransAutoRollback" ) );
            TransUtils.beginTransaction( db2 );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            try {
                cl2.insert( "{_id:2, a:1, b:2}" );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            }
            cl2.insert( "{_id:3, a:3, b:3}" );
            db2.rollback();
            TransUtils.queryAndCheck( cl2, "{_id:1}", expList );

            // 在连接db1上，开启事务，插入记录R3与集合中已存在的记录唯一索引重复，再次插入记录R4成功，回滚事务
            TransUtils.beginTransaction( db1 );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            try {
                cl1.insert( "{_id:2, a:1, b:2}" );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            }
            cl1.insert( "{_id:3, a:3, b:3}" );
            expList.add( ( BSONObject ) JSON.parse( "{_id:3, a:3, b:3}" ) );
            db1.rollback();
            TransUtils.queryAndCheck( cl1, "{_id:1}", expList );

            // 创建一个连接db3，开启事务，插入记录R3与集合中已存在的记录唯一索引重复，再次插入记录R4成功，回滚事务
            db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl3 = db3.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            try {
                cl3.insert( "{_id:2, a:1, b:2}" );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            }
            cl3.insert( "{_id:4, a:4, b:4}" );
            expList.add( ( BSONObject ) JSON.parse( "{_id:4, a:4, b:4}" ) );
            db3.rollback();
            TransUtils.queryAndCheck( cl3, "{_id:1}", expList );

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
