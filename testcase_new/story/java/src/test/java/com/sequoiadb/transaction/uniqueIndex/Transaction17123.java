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
 * @Description Transaction17123.java 插入记录与其他事务中插入的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17123 extends SdbTestBase {

    private String clName = "cl17123";
    private String idxName = "idx17123";
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

    @Test(priority = 1)
    public void testIdIndex1() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17123'}" );
        try {
            TransUtils.beginTransaction( sdb );
            cl.insert( data1 );
            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.insert( data1 );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'$id'}", expDataList );

            sdb.rollback();

            TransUtils.queryAndCheck( cl2, "{'':null}",
                    new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( cl2, "{'':'$id'}",
                    new ArrayList< BSONObject >() );
        } finally {
            sdb.rollback();
            sdb2.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 2)
    public void testIdIndex2() {

        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17123'}" );
        try {
            TransUtils.beginTransaction( sdb );
            cl.insert( data1 );
            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.beginTransaction( sdb2 );
            try {
                cl2.insert( data1 );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'$id'}", expDataList );

            sdb.commit();
            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'$id'}", expDataList );
        } finally {
            sdb.commit();
            sdb2.commit();
            cl.delete( "" );
        }

    }

    @Test(priority = 3)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17123'}" );
        try {
            TransUtils.beginTransaction( sdb );
            cl.insert( data1 );
            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.beginTransaction( sdb2 );
            try {
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:2,a:'id17123'}" );
                cl2.insert( data2 );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'" + idxName + "'}",
                    expDataList );

            sdb.rollback();

            TransUtils.queryAndCheck( cl2, "{'':null}",
                    new ArrayList< BSONObject >() );
            TransUtils.queryAndCheck( cl2, "{'':'" + idxName + "'}",
                    new ArrayList< BSONObject >() );
        } finally {
            sdb.rollback();
            sdb2.rollback();
            cl.dropIndex( idxName );
            cl.delete( "" );
        }

    }

    @Test(priority = 4)
    public void testCommonUniqueIdx2() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17123'}" );
        try {
            TransUtils.beginTransaction( sdb );
            cl.insert( data1 );
            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.beginTransaction( sdb2 );
            try {
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:2,a:'id17123'}" );
                cl2.insert( data2 );
                Assert.fail( "Need throw error -38." );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }

            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'" + idxName + "'}",
                    expDataList );

            sdb.commit();

            TransUtils.queryAndCheck( cl2, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl2, "{'':'" + idxName + "'}",
                    expDataList );
        } finally {
            sdb.commit();
            sdb2.commit();
            cl.dropIndex( idxName );
            cl.delete( "" );
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        sdb2.close();
    }

}
