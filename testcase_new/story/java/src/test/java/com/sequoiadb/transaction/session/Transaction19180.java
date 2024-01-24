package com.sequoiadb.transaction.session;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19180:TransUseRBS属性支持会话级别
 * @author yinzhen
 * @date 2019年9月18日
 */
public class Transaction19180 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19180";
    private Boolean isTransisolationRR;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName );
        isTransisolationRR = TransUtils.isTransisolationRR( sdb );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;

        try {

            // 创建一个连接db1，并插入1条记录R1，设置TransUseRBS属性为false，查询TransUseRBS属性
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject obj = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl1.insert( obj );
            db1.setSessionAttr( ( BSONObject ) JSON
                    .parse( "{TransIsolation:1, TransUseRBS:false}" ) );
            BSONObject attr = db1.getSessionAttr();
            Assert.assertEquals( false, attr.get( "TransUseRBS" ) );

            // 创建一个连接db2，并设置TransUseRBS属性为false，查询TransUseRBS属性
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2.setSessionAttr( ( BSONObject ) JSON
                    .parse( "{TransIsolation:1, TransUseRBS:false}" ) );
            attr = db2.getSessionAttr();
            Assert.assertEquals( false, attr.get( "TransUseRBS" ) );

            // db1上开启事务，执行update操作
            TransUtils.beginTransaction( db1 );
            cl1.update( "{a:1}", "{$set:{b:2}}", null );

            // db2上开启事务，执行查询，检查结果
            TransUtils.beginTransaction( db2 );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            try {
                DBCursor cursor = cl2.query();
                TransUtils.getReadActList( cursor );
                if ( !isTransisolationRR ) {
                    Assert.fail();
                }
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -13 );
            }

        } finally

        {
            if ( null != db1 ) {
                db1.commit();
                db1.close();
            }
            if ( null != db2 ) {
                db2.commit();
                db2.close();
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
