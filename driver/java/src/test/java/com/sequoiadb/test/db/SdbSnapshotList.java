package com.sequoiadb.test.db;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertTrue;

public class SdbSnapshotList {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        isCluster = Constants.isCluster();
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
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
    public void getSnapshot() {
        BSONObject matcher = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();
        matcher.put("Name", Constants.TEST_CS_NAME_1 + "." + Constants.TEST_CL_NAME_1);
        selector.put("Name", 1);
        selector.put("Details", 1);
        orderBy.put("Details", 1);
        cursor = sdb.getSnapshot(4, matcher, selector, orderBy);
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(0, "", "", "");
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(1, "", "", "");
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(2, "", "", "");
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(3, "", "", "");
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(5, "", "", "");
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(6, "", "", "");
        assertTrue(cursor.hasNext());
        cursor = sdb.getSnapshot(7, "", "", "");
        assertTrue(cursor.hasNext());
        try {
            cursor = sdb.getSnapshot(8, "", "", "");
            assertTrue(cursor.hasNext());
        } catch (BaseException e) {
            if (!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
                assertTrue(false);
        }

        try {
            sdb.beginTransaction();
            cl.insert(new BasicBSONObject());
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", "", "");
            System.out.println("result of SDB_SNAP_TRANSACTIONS_CURRENT is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_TRANSACTIONS, "", "", "");
            System.out.println("result of SDB_SNAP_TRANSACTIONS is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
        } catch(BaseException e) {
            Assert.assertTrue(e.getErrorType().equals("SDB_DPS_TRANS_DIABLED"));
        }finally {
            sdb.commit();
        }

        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_ACCESSPLANS, "", "", "");
        System.out.println("result of SDB_SNAP_ACCESSPLANS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }
        
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_HEALTH, "", "", "");
        System.out.println("result of SDB_SNAP_HEALTH is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }     

    }

    @Test
    public void getList() {
        BSONObject matcher = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();
        matcher.put("Status", "Running");
        selector.put("SessionID", 1);
        selector.put("TID", 1);
        selector.put("Status", 1);
        selector.put("Type", 1);
        orderBy.put("TID", 1);
        cursor = sdb.getList(2, matcher, selector, orderBy);
        assertTrue(cursor.hasNext());
        cursor = sdb.getList(0, null, null, null);
        assertTrue(cursor.hasNext());
        cursor = sdb.getList(1, null, null, null);
        assertTrue(cursor.hasNext());
        cursor = sdb.getList(3, null, null, null);
        assertTrue(cursor.hasNext());
        cursor = sdb.getList(4, null, null, null);
        assertTrue(cursor.hasNext());
        cursor = sdb.getList(5, null, null, null);
        assertTrue(cursor.hasNext());
        if (!isCluster) {
            cl.insert(new BasicBSONObject("a", 1));
            cursor = sdb.getList(6, null, null, null);
            assertTrue(cursor.hasNext());
        }
        try {
            cursor = sdb.getList(7, null, null, null);
            assertTrue(cursor.hasNext());
        } catch (BaseException e) {
            if (!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
                assertTrue(false);
        }
        if (isCluster) {
            cursor = null;
            cursor = sdb.getList(8, null, null, null);
            assertTrue(null != cursor);
        }

        try {
            cursor = null;
            cursor = sdb.getList(9, null, null, null);
            assertTrue(null != cursor);
        } catch (BaseException e) {
            if (!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
                assertTrue(false);
        }
        if (isCluster) {
            cursor = null;
            cursor = sdb.getList(10, null, null, null);
            assertTrue(null != cursor);
        }

        try {
            sdb.beginTransaction();
            BSONObject dump = new BasicBSONObject();
            BSONObject obj = new BasicBSONObject();
            cl.insert(obj);
            cursor = sdb.getList(Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT, dump, dump, dump);
            System.out.println("result of SDB_LIST_TRANSACTIONS_CURRENT is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
            cursor = sdb.getList(Sequoiadb.SDB_LIST_TRANSACTIONS, dump, dump, dump);
            System.out.println("result of SDB_LIST_TRANSACTIONS is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
        } catch(BaseException e) {
            Assert.assertTrue(e.getErrorType().equals("SDB_DPS_TRANS_DIABLED"));
        } finally {
            sdb.commit();
        }
    }
}
