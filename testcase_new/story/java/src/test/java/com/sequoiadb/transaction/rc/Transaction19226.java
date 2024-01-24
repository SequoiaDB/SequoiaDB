package com.sequoiadb.transaction.rc;

/**
 * @Description  seqDB-19226:全表扫描删除不删除old version链
 * @author zhaoyu
 * @date 2019-8-27
 */
import java.util.ArrayList;

import org.bson.BSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rc")
public class Transaction19226 extends SdbTestBase {
    private String clName = "cl_19226";
    private String indexName = "index_19226";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( indexName, "{a:1}", false, false );
    }

    @Test
    public void test() {
        ArrayList< BSONObject > records = TransUtils.insertRandomDatas( cl, 0,
                100 );
        TransUtils.beginTransaction( sdb );
        cl.update( null, "{$set:{a:1024}}", null, 0 );

        cl1.dropIndex( indexName );

        cl.delete( "{a:{$gte:0}}" );

        cl1.createIndex( indexName, "{a:1}", false, false );

        TransUtils.beginTransaction( db1 );
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':'a'}", records );
        TransUtils.queryAndCheck( cl1, "{a:1}", "{'':null}", records );

        sdb.commit();
        db1.commit();
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        db1.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();
    }
}
