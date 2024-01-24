package com.sequoiadb.transaction.rc;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19184:TransRCCount属性支持会话级别
 * @author yinzhen
 * @date 2019年9月18日
 */
@Test(groups = "rc")
public class Transaction19184 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19184";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
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
            db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            TransUtils.beginTransaction( db1 );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject obj = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl1.insert( obj );
            List< BSONObject > expList = new ArrayList<>();
            expList.add( obj );

            // 创建一个连接db2
            db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );

            // 创建一个连接db3，并设置TransRCCount属性为false，查询TransRCCount属性，开启事务，执行count查询
            db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            db3.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransRCCount:false}" ) );
            BSONObject attr = db3.getSessionAttr();
            Assert.assertEquals( false, attr.get( "TransRCCount" ) );
            TransUtils.beginTransaction( db3 );
            DBCollection cl3 = db3.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            long count = cl3.getCount();
            Assert.assertEquals( count, 1 );

            // 在连接db2上，开启事务，执行count查询
            TransUtils.beginTransaction( db2 );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            count = cl2.getCount();
            Assert.assertEquals( count, 0 );

            // 创建一个连接db4，开启事务，执行count查询
            db4 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl4 = db4.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            TransUtils.beginTransaction( db4 );
            count = cl4.getCount();
            Assert.assertEquals( count, 0 );

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
