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
 * @Description Transaction17122.java 插入记录与本事务中删除的记录重复
 * @author luweikang
 * @date 2019年1月15日
 */
public class Transaction17122 extends SdbTestBase {

    private String clName = "cl17122";
    private String idxName = "idx17122";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
    }

    @Test(priority = 1)
    public void testIdIndex1() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17122'}" );
        cl.insert( data1 );
        expDataList.clear();
        expDataList.add( data1 );
        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{'_id': 'id17122'}" );
            cl.insert( data1 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );

            sdb.rollback();
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );
        } finally {
            sdb.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 2)
    public void testIdIndex2() {
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id17122'}" );
        cl.insert( data1 );
        expDataList.clear();
        expDataList.add( data1 );
        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{'_id': 'id17122'}" );
            cl.insert( data1 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );

            sdb.commit();
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );
        } finally {
            sdb.commit();
            cl.delete( "" );
        }

    }

    @Test(priority = 3)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17122'}" );
        cl.insert( data1 );
        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{'a': 'id17122'}" );
            BSONObject data2 = ( BSONObject ) JSON
                    .parse( "{_id:2,a:'id17122'}" );
            cl.insert( data2 );
            expDataList.clear();
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                    expDataList );

            sdb.rollback();
            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                    expDataList );
        } finally {
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }
    }

    @Test(priority = 4)
    public void testCommonUniqueIdx2() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id17122'}" );
        cl.insert( data1 );
        try {
            TransUtils.beginTransaction( sdb );
            cl.delete( "{'a': 'id17122'}" );
            BSONObject data2 = ( BSONObject ) JSON
                    .parse( "{_id:2,a:'id17122'}" );
            cl.insert( data2 );
            expDataList.clear();
            expDataList.add( data2 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                    expDataList );

            sdb.commit();
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                    expDataList );
        } finally {
            sdb.commit();
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
