package com.sequoiadb.crud;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-11427:插入与查询并发
 * @Author wangkexin
 * @Date 2019.03.15
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @version 1.00
 */
public class CRUD11427 extends SdbTestBase {
    private String clName = "cl_11427";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private static ArrayList< BSONObject > expectData = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > clRecords = new ArrayList< BSONObject >();
    private int beginNo = 0;
    private int endNo = 20000;
    private BSONObject lastDataId = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        expectData = CRUDUitls.insertData( cl, beginNo, endNo );
        // 最后一条数据的_id,用作并发查询的查询条件
        lastDataId = expectData.get( expectData.size() - 1 );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        threadInsert insertData = new threadInsert();
        threadQuery queryData = new threadQuery();
        threadQuery queryData1 = new threadQuery();
        es.addWorker( insertData );
        es.addWorker( queryData );
        es.addWorker( queryData1 );
        es.run();
        Assert.assertEquals( insertData.getRetCode(), 0 );
        Assert.assertEquals( queryData.getRetCode(), 0 );
        Assert.assertEquals( queryData1.getRetCode(), 0 );

        expectData.addAll( clRecords);
        DBCursor clAllRecords = cl.query( "", "", "{_id:1}", "" );
        // 校验所有数据
        CRUDUitls.checkRecords( expectData, clAllRecords );
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

    private class threadInsert extends ResultStore {
        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                clRecords = CRUDUitls.insertData( cl, beginNo, endNo );
            }
        }
    }

    private class threadQuery extends ResultStore {
        @ExecuteOrder(step = 1)
        private void query() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject matcher = new BasicBSONObject();
                matcher.put( "_id", new BasicBSONObject( "$lte",
                        lastDataId.get( "_id" ) ) );
                DBCursor queryCursor = cl.query( matcher, null, null, null );
                CRUDUitls.checkRecords( expectData, queryCursor );
            }
        }
    }

}
