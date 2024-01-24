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
 * @Description seqDB-21879:插入记录与本事务中删除的记录索引重复
 * @author zhaoyu
 * @date 2019年1月15日
 */
public class Transaction21879 extends SdbTestBase {
    private String clName = "cl21879";
    private String idxName = "idx21879";
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
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id21879_1'}" );
        cl.insert( data1 );
        try {
            for ( int i = 0; i < 10; i++ ) {
                TransUtils.beginTransaction( sdb );
                cl.delete( "" );
                cl.insert( data1 );

                sdb.rollback();
                cl.delete( "" );
                expDataList.clear();
                TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
                TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );
            }

        } finally {
            sdb.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 1)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );
        BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:1,a:'id21879_1'}" );
        cl.insert( data1 );
        try {
            for ( int i = 0; i < 10; i++ ) {
                TransUtils.beginTransaction( sdb );
                cl.delete( "" );
                BSONObject data2 = ( BSONObject ) JSON
                        .parse( "{_id:2,a:'id21879_1'}" );
                cl.insert( data2 );

                sdb.rollback();
                cl.delete( "" );
                expDataList.clear();
                TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
                TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                        expDataList );
            }

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
