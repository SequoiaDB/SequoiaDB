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
 * @Description seqDB-20013:回滚多条记录，回滚第一条更新的记录与过程中插入的记录重复
 * @author zhaoyu
 * @date 2019年1月15日
 */
public class Transaction20013 extends SdbTestBase {
    private String clName = "cl20013";
    private String idxName = "idx20013";
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

        try {
            TransUtils.beginTransaction( sdb );
            BSONObject data1 = ( BSONObject ) JSON.parse( "{_id:'id20013_1'}" );
            BSONObject data2 = ( BSONObject ) JSON.parse( "{_id:'id20013_2'}" );
            cl.insert( data1 );
            cl.insert( data2 );
            cl.update( "{_id:'id20013_1'}", "{$set:{_id:'id20013_3'}}",
                    "{'':'$id'}" );
            TransUtils.beginTransaction( sdb2 );
            cl2.insert( data1 );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data1 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );

            sdb2.rollback();

            expDataList.clear();
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'$id'}", expDataList );
        } finally {
            sdb2.rollback();
            sdb.rollback();
            cl.delete( "" );
        }
    }

    @Test(priority = 2)
    public void testCommonUniqueIdx1() {
        cl.createIndex( idxName, "{a:1}", true, true );

        try {
            TransUtils.beginTransaction( sdb );
            BSONObject data1 = ( BSONObject ) JSON
                    .parse( "{_id:1,a:'id20013_1'}" );
            BSONObject data2 = ( BSONObject ) JSON
                    .parse( "{_id:2,a:'id20013_2'}" );
            cl.insert( data1 );
            cl.insert( data2 );
            cl.update( "{a:'id20013_1'}", "{$set:{a:'id20013_3'}}",
                    "{'':'" + idxName + "'}" );

            TransUtils.beginTransaction( sdb2 );

            BSONObject data3 = ( BSONObject ) JSON
                    .parse( "{_id:3,a:'id20013_1'}" );
            cl2.insert( data3 );

            sdb.rollback();

            expDataList.clear();
            expDataList.add( data3 );
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                    expDataList );

            sdb2.rollback();

            expDataList.clear();
            TransUtils.queryAndCheck( cl, "{'':null}", expDataList );
            TransUtils.queryAndCheck( cl, "{'':'" + idxName + "'}",
                    expDataList );
        } finally {
            sdb2.rollback();
            sdb.rollback();
            cl.delete( "" );
            cl.dropIndex( idxName );
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
        sdb2.close();
    }

}
