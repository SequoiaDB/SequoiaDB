package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.MultiThreadInsert;
import com.sequoiadb.test.common.MultiThreadQuery;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;


public class testMultiThreadInsertAndQuery {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;


    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void testThreadInsertAndQuery() {
        final int THREAD_COUNT = 10;
        Thread[] insertThreadList = new Thread[THREAD_COUNT];
        Thread[] queryThreadList = new Thread[THREAD_COUNT];

        for (int i = 0; i < THREAD_COUNT; i++) {

            insertThreadList[i] = new Thread(new MultiThreadInsert());
            queryThreadList[i] = new Thread(new MultiThreadQuery());

            insertThreadList[i].start();
            queryThreadList[i].start();
        }
        for (int i = 0; i < THREAD_COUNT; i++) {
            try {
                insertThreadList[i].join();
                queryThreadList[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
