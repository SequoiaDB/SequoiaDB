package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18331:插入记录与其他事务多次更新的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction18334 extends SdbTestBase {
    private String clName = "cl18334";
    private String idxName = "idx18334";
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
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 10; i++ ) {
                cl.update( "", "{$inc:{_id:1}}", "" );
            }

            // 其他事务插入记录
            TransUtils.beginTransaction( sdb2 );
            expDataList.clear();
            expDataList.add( data1 );
            for ( int i = 2; i < 11; i++ ) {
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:" + i + "}" );
                cl2.insert( data2 );
                expDataList.add( data2 );
            }
            sdb2.commit();
            sdb.rollback();

            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            sdb2.commit();
            cl.delete( "" );
        }

    }

    @Test(priority = 2)
    public void testIdIndex2() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 10; i++ ) {
                cl.update( "", "{$inc:{_id:1}}", "" );
            }

            // 本事务插入记录
            expDataList.clear();
            for ( int i = 2; i < 11; i++ ) {
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:" + i + "}" );
                expDataList.add( data2 );
            }
            cl.insert( expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }

    }

    @Test(priority = 3)
    public void testIdIndex3() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 10; i++ ) {
                cl.update( "", "{$inc:{_id:1}}", "" );
            }

            // 非事务插入记录
            expDataList.clear();
            expDataList.add( data1 );
            for ( int i = 2; i < 11; i++ ) {
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:" + i + "}" );
                cl2.insert( data2 );
                expDataList.add( data2 );
            }

            sdb.rollback();
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'$id'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }

    }

    @Test(priority = 4)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 10; i++ ) {
                cl.update( "", "{$inc:{a:1}}", "" );
            }

            // 其他事务插入记录
            TransUtils.beginTransaction( sdb2 );
            expDataList.clear();
            expDataList.add( data1 );
            for ( int i = 2; i < 11; i++ ) {
                BSONObject data2 = ( BSONObject ) JSON.parse( "{a:" + i + "}" );
                cl2.insert( data2 );
                expDataList.add( data2 );
            }

            sdb2.commit();
            sdb.rollback();

            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

        } finally {
            sdb.rollback();
            sdb2.commit();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 5)
    public void testCommonUniqueIdx2() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 10; i++ ) {
                cl.update( "", "{$inc:{a:1}}", "" );
            }

            // 本事务插入记录
            expDataList.clear();
            for ( int i = 2; i < 11; i++ ) {
                BSONObject data2 = ( BSONObject ) JSON.parse( "{a:" + i + "}" );
                expDataList.add( data2 );
            }
            cl.insert( expDataList );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @Test(priority = 6)
    public void testCommonUniqueIdx3() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{a:1}" );
        cl.insert( data1 );

        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 10; i++ ) {
                cl.update( "", "{$inc:{a:1}}", "" );
            }

            // 非事务插入记录
            expDataList.clear();
            expDataList.add( data1 );
            for ( int i = 2; i < 11; i++ ) {
                BSONObject data2 = ( BSONObject ) JSON.parse( "{a:" + i + "}" );
                cl2.insert( data2 );
                expDataList.add( data2 );
            }

            sdb.rollback();
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'" + idxName + "'}",
                    expDataList );

        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }

    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
