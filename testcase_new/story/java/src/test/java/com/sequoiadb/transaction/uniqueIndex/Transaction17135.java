package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17135:回滚的记录与本事务中的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17135 extends SdbTestBase {
    private String clName = "cl17135";
    private String idxName = "idx17135";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
    }

    @Test
    public void testCommonUniqueIdx1() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17135_1'}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:'id17135_1'}" );
        cl.insert( data1 );
        cl.insert( data2 );

        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{_id:1}" );

            // 本事务中创建索引
            try {
                cl.createIndex( idxName, "{a:1}", true, true );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            }

            // 非事务中创建索引
            try {
                cl2.createIndex( idxName, "{a:1}", true, true );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            }

            // 事务2中创建索引
            try {
                TransUtils.beginTransaction( sdb2 );
                cl2.createIndex( idxName, "{a:1}", true, true );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38 );
            } finally {
                sdb2.commit();
            }

            expDataList.clear();
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
