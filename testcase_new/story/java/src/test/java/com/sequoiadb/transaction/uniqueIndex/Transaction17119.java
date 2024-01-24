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
 * @FileName seqDB-17119 : 事务中插入记录与已提交记录唯一索引重复
 * @Author luweikang
 * @Date 2019年1月15日
 */
public class Transaction17119 extends SdbTestBase {

    private String clName = "cl17119";
    private String idxName = "idx17119";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
    }

    @Test(priority = 1)
    public void testIdIndex() {
        BSONObject data = ( BSONObject ) JSON.parse( "{_id:1}" );
        cl.insert( data );
        expDataList.clear();
        expDataList.add( data );

        try {
            TransUtils.beginTransaction( sdb );
            cl.insert( data );
            Assert.fail( "Need throw error -38." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        } finally {
            sdb.commit();
        }

        TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
        TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );

        cl.delete( "" );
        Assert.assertEquals( cl.getCount(), 0 );
    }

    @Test(priority = 2)
    public void testCommonUniqueIdx() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:1}" );
        BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:2,a:1}" );
        cl.insert( data1 );
        expDataList.clear();
        expDataList.add( data1 );
        try {
            TransUtils.beginTransaction( sdb );
            cl.insert( data2 );
            Assert.fail( "Need throw error -38." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        } finally {
            sdb.commit();
        }

        TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
        TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}", expDataList );

        cl.delete( "" );
        Assert.assertEquals( cl.getCount(), 0 );
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

}
