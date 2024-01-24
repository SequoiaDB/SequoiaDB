package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;


public class CLGetQueryMeta {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;
    private static DBCursor datacursor;
    private static int RECORDNUM = 10000;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // gen record
        ConstantsInsert.insertVastRecord(Constants.HOST, Constants.PORT,
            Constants.TEST_CS_NAME_1, Constants.TEST_CL_NAME_1,
            Constants.SDB_PAGESIZE_4K, RECORDNUM);
        cs = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        cl = cs.getCollection(Constants.TEST_CL_NAME_1);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void getQueryMeta() {
        try {
            // query data from master
            BSONObject sessionAttrObj = new BasicBSONObject();
            sessionAttrObj.put("PreferedInstance", "M");
            sdb.setSessionAttr(sessionAttrObj);
            // condition
            BSONObject empty = new BasicBSONObject();
            BSONObject subobj = new BasicBSONObject();
            subobj.put("$gt", 1);
            subobj.put("$lt", 99);
            BSONObject condition = new BasicBSONObject();
            condition.put("age", subobj);
            System.out.println("condition is: " + condition.toString());
            // select
            BSONObject select = new BasicBSONObject();
            select.put("", "ageIndex");
            // orderBy
            BSONObject orderBy = new BasicBSONObject();
            orderBy.put("Indexblocks", 1);
            cursor = cl.getQueryMeta(condition, orderBy, empty, 0, -1, 0);
//			cursor = cl.getQueryMeta(empty, empty, empty, 0, 0, 0);
            long i = 0;
            while (cursor.hasNext()) {
                System.out.println(cursor.getNext().toString());
                BSONObject temp = cursor.getCurrent();
                BSONObject hint = new BasicBSONObject();
                hint.put("Indexblocks", temp.get("Indexblocks"));
                System.out.println("hint is: " + hint.toString());
                datacursor = cl.query(null, null, null, hint, 0, -1, 0);
                while (datacursor.hasNext()) {
                    i++;
                    datacursor.getNext();
                }
            }
            System.out.println("total record is: " + i);
            assertEquals(RECORDNUM, i);
        } catch (BaseException e) {
            e.printStackTrace();
        }
    }
}
