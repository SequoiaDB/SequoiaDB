package com.sequoiadb.transaction.sessionserial;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19193:coord及数据节点均开启事务，TransAutoRollback属性不一致
 * @author yinzhen
 * @date 2019年9月18日
 */
@Test(groups = "ru")
public class Transaction19193 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19193";
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
        cl.createIndex( "idx_19193", "{a:1}", true, false );
        cl.insert( "{_id:1, a:1, b:1}" );
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );
        sdb.updateConfig(
                ( BSONObject ) JSON.parse( "{transautorollback:false}" ),
                ( BSONObject ) JSON.parse( "{Global:false}" ) );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;

        try {

            // 开启事务1，插入记录R1与集合中已存在的记录唯一索引重复，
            // 再次插入记录R2，回滚事务，检查结果
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
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

            db1.rollback();
            DBCursor cursor = cl1.query();
            List< BSONObject > actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
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
            sdb.deleteConfig(
                    ( BSONObject ) JSON.parse( "{transautorollback:''}" ),
                    ( BSONObject ) JSON.parse( "{Global:false}" ) );
            sdb.close();
        }
    }
}
