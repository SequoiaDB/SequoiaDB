package com.sequoiadb.crud;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-1542:更新记录同时删记录
 * @Author laojingtang
 * @Date 2018.01.04
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @Version 1.10
 */
public class CRUD1542 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "curdcl_1542";
    private DBCollection cl = null;

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        BasicBSONObject obj = new BasicBSONObject( "a", 1 );
        cl.insert( obj );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();

        threadUpdate updateData = new threadUpdate();
        threadRemove removeData = new threadRemove();

        es.addWorker( updateData );
        es.addWorker( removeData );
        es.run();
        Assert.assertEquals( removeData.getRetCode(), 0 );
        Assert.assertEquals( updateData.getRetCode(), 0 );
        int actualCount = ( int ) cl.getCount();
        Assert.assertEquals( actualCount, 0 );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class threadRemove extends ResultStore {
        @ExecuteOrder(step = 1)
        private void remove() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                cl.delete( new BasicBSONObject() );
            }
        }
    }

    private class threadUpdate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void update() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                for ( int i = 0; i < 10000; i++ ) {
                    cl.update( new BasicBSONObject(),
                            ( BSONObject ) JSON.parse( "{$inc:{a:1}}" ),
                            new BasicBSONObject() );
                }
            }
        }
    }
}
