package com.sequoiadb.transaction.session;

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

/**
 * 
 * @description seqDB-19182:TransAutoCommit属性支持会话级别
 * @author yinzhen
 * @date 2019年9月18日
 */
public class Transaction19182 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19182";

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

        try {

            // 创建一个连接db1
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // 创建一个连接db2，并设置TransAutoCommit属性为true，查询TransAutoCommit属性，插入记录R1
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{TransAutoCommit:true}" ) );
            BSONObject attr = db2.getSessionAttr();
            Assert.assertEquals( true, attr.get( "TransAutoCommit" ) );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl2.insert( "{_id:1, a:1, b:1}" );

            // 在连接db1上，插入记录R2
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl1.insert( "{_id:2, a:2, b:2}" );

            // 创建一个连接db3，插入记录R3
            db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl3 = db3.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl3.insert( "{_id:3, a:3, b:3}" );

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
