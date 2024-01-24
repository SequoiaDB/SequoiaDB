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
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
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
        // 4
        cursor = sdb.getSnapshot(4, matcher, selector, orderBy);
        assertTrue(cursor.hasNext());
//	    System.out.println(cursor.getNext().toString());
        // 0
        cursor = sdb.getSnapshot(0, "", "", "");
        assertTrue(cursor.hasNext());
        // 1
        cursor = sdb.getSnapshot(1, "", "", "");
        assertTrue(cursor.hasNext());
        // 2
        cursor = sdb.getSnapshot(2, "", "", "");
        assertTrue(cursor.hasNext());
        // 3
        cursor = sdb.getSnapshot(3, "", "", "");
        assertTrue(cursor.hasNext());
        // 5
        cursor = sdb.getSnapshot(5, "", "", "");
        assertTrue(cursor.hasNext());
        // 6
        cursor = sdb.getSnapshot(6, "", "", "");
        assertTrue(cursor.hasNext());
        // 7
        cursor = sdb.getSnapshot(7, "", "", "");
        assertTrue(cursor.hasNext());
        // 8
        try {
            cursor = sdb.getSnapshot(8, "", "", "");
            assertTrue(cursor.hasNext());
        } catch (BaseException e) {
            if (!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
                assertTrue(false);
        }

        // 9
        try {
            sdb.beginTransaction();
            cl.insert(new BasicBSONObject());
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", "", "");
            System.out.println("result of SDB_SNAP_TRANSACTIONS_CURRENT is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
            // 10
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_TRANSACTIONS, "", "", "");
            System.out.println("result of SDB_SNAP_TRANSACTIONS is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }

            // 14
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SVCTASKS, null, null, null, null, 0, -1);
            System.out.println("result of SDB_SNAP_SVCTASKS is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }

            // 15
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SEQUENCES, "", "", "");
            System.out.println("result of SDB_SNAP_TRANSACTIONS is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
        } catch(BaseException e) {
            e.printStackTrace();
            Assert.assertTrue(e.getErrorType().equals("SDB_DPS_TRANS_DIABLED"));
        }finally {
            sdb.commit();
        }

        // 11
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_ACCESSPLANS, "", "", "");
        System.out.println("result of SDB_SNAP_ACCESSPLANS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }
        
        // 12
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_HEALTH, "", "", "");
        System.out.println("result of SDB_SNAP_HEALTH is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }     
        
        // 15
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SEQUENCES, "", "", "");
        System.out.println("result of SDB_SNAP_SEQUENCES is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 18
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_QUERIES, "", "", "");
        System.out.println("result of SDB_SNAP_QUERIES is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 19
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_LATCHWAITS, "", "", "");
        System.out.println("result of SDB_SNAP_LATCHWAITS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 20
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_LOCKWAITS, "", "", "");
        System.out.println("result of SDB_SNAP_LOCKWAITS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 21
        sdb.analyze();
        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_INDEXSTATS, "", "", "");
        System.out.println("result of SDB_SNAP_INDEXSTATS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }
    }

    @Test
    public void getList() {
        BSONObject dump = new BasicBSONObject();
        BSONObject matcher = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();
        matcher.put("Status", "Running");
        selector.put("SessionID", 1);
        selector.put("TID", 1);
        selector.put("Status", 1);
        selector.put("Type", 1);
        orderBy.put("TID", 1);
        // 2
        cursor = sdb.getList(2, matcher, selector, orderBy);
        assertTrue(cursor.hasNext());
        // 0
        cursor = sdb.getList(0, null, null, null);
        assertTrue(cursor.hasNext());
        // 1
        cursor = sdb.getList(1, null, null, null);
        assertTrue(cursor.hasNext());
        // 3
        cursor = sdb.getList(3, null, null, null);
        assertTrue(cursor.hasNext());
        // 4
        cursor = sdb.getList(4, null, null, null);
        assertTrue(cursor.hasNext());
        // 5
        cursor = sdb.getList(5, null, null, null);
        assertTrue(cursor.hasNext());
        // 6 in cluster, we need to connect to data node to test,
        if (!isCluster) {
            cl.insert(new BasicBSONObject("a", 1));
            cursor = sdb.getList(6, null, null, null);
            assertTrue(cursor.hasNext());
        }
        // 7
        try {
            cursor = sdb.getList(7, null, null, null);
            assertTrue(cursor.hasNext());
        } catch (BaseException e) {
            if (!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
                assertTrue(false);
        }
        // 8
        if (isCluster) {
            cursor = null;
            cursor = sdb.getList(8, null, null, null);
            assertTrue(null != cursor);
        }

        // 9
        try {
            cursor = null;
            cursor = sdb.getList(9, null, null, null);
            assertTrue(null != cursor);
        } catch (BaseException e) {
            if (!e.getErrorType().equals("SDB_RTN_COORD_ONLY"))
                assertTrue(false);
        }
        // 10
        if (isCluster) {
            cursor = null;
            cursor = sdb.getList(10, null, null, null);
            assertTrue(null != cursor);
        }

        // 11
        try {
            sdb.beginTransaction();
            BSONObject obj = new BasicBSONObject();
            cl.insert(obj);
            cursor = sdb.getList(Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT, dump, dump, dump);
            System.out.println("result of SDB_LIST_TRANSACTIONS_CURRENT is: ");
            while(cursor.hasNext()){
                System.out.println(cursor.getNext());
            }
            // 12
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

        // 14
        cursor = sdb.getList(Sequoiadb.SDB_LIST_SVCTASKS, dump, dump, dump, dump, 0, -1);
        System.out.println("result of SDB_LIST_SVCTASKS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 15
        cursor = sdb.getList(Sequoiadb.SDB_LIST_SEQUENCES, dump, dump, dump);
        System.out.println("result of SDB_LIST_SEQUENCES is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 16
        cursor = sdb.getList(Sequoiadb.SDB_LIST_USERS, dump, dump, dump, dump, 0, -1);
        System.out.println("result of SDB_LIST_USERS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

        // 17
        cursor = sdb.getList(Sequoiadb.SDB_LIST_BACKUPS, dump, dump, dump, dump, 0, -1);
        System.out.println("result of SDB_LIST_BACKUPS is: ");
        while(cursor.hasNext()){
            System.out.println(cursor.getNext());
        }

    }
}
