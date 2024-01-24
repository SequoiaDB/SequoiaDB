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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19178:TransIsolation属性支持会话级别
 * @author yinzhen
 * @date 2019年9月17日
 */
@Test(groups = "ru")
public class Transaction19178 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19178";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName,
                ( BSONObject ) JSON
                        .parse( "{ShardingKey:{_id:1}, AutoSplit:true}" ) );
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

            // 创建一个连接db3，并设置TransIsolation属性为1，查询TransIsolation属性，开启事务，执行查询
            db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db3.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransIsolation:1}" ) );
            BSONObject attr = db3.getSessionAttr();
            Assert.assertEquals( 1, attr.get( "TransIsolation" ) );
            TransUtils.beginTransaction( db3 );
            DBCollection cl3 = db3.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            TransUtils.queryAndCheck( cl3, "{_id:1}",
                    new ArrayList< BSONObject >() );

            // 在连接db2上，开启事务，执行查询
            TransUtils.beginTransaction( db2 );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            TransUtils.queryAndCheck( cl2, "{_id:1}", expList );

            // 创建一个连接db4，开启事务，执行查询
            db4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            TransUtils.beginTransaction( db4 );
            DBCollection cl4 = db4.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            TransUtils.queryAndCheck( cl4, "{_id:1}", expList );

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
