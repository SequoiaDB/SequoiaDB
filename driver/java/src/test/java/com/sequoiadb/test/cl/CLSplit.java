package com.sequoiadb.test.cl;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.Map;

import static org.junit.Assert.assertTrue;


public class CLSplit {

    private static Sequoiadb sdb;
    private static ReplicaGroup rg;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static String host = "ubuntu-dev1";
    private static int nodePort = 58100;
    private static String sourceRGName = "group1";
    private static String targetRGName = "shardingTargetGroup";
    private static String nodePath = "/home/users/tanzhaobo/data/database/data/58100";

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
        try {
            if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1))
                sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            sdb.removeReplicaGroup(targetRGName);
        } catch (BaseException ex) {
            int exCode = ex.getErrorCode();
        }
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        try {
            rg = sdb.createReplicaGroup(targetRGName);
            rg.createNode(host, nodePort,
                nodePath, (Map<String, String>) null);
            rg.start();
            Thread.sleep(5000);
        } catch (BaseException e) {
            int errno = e.getErrorCode();
            System.out.println("errno = " + errno);
            try {
                sdb.removeReplicaGroup(targetRGName);
            } catch (BaseException exp) {
                int errCode = exp.getErrorCode();
                System.out.println("Something wrong with remove rg. errCode = " + errCode);
            }
        }
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        try {
            sdb.removeReplicaGroup(targetRGName);
        } catch (BaseException exp) {
            int errCode = exp.getErrorCode();
            System.out.println("Something wrong with remove rg. errCode = " + errCode);
        }
    }

    @Test
    @Ignore
    public void hashSplitRange() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "hash");
        conf.put("Partition", 1024);
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        BSONObject cond = new BasicBSONObject();
        BSONObject endCond = new BasicBSONObject();
        cond.put("Partition", 512);
        endCond.put("partition", 1024);
        cl.split(sourceRGName, targetRGName, cond, endCond);
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            e.printStackTrace();
            assertTrue(false);
        }
        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing hashSplitRange()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void hashSplitPercent() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "hash");
        conf.put("Partition", 1024);
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        double percent = 50;
        cl.split(sourceRGName, targetRGName, percent);
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            e.printStackTrace();
            assertTrue(false);
        }
        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing hashSplitPercent()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void normalSplitRange() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "range");
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        BSONObject cond = new BasicBSONObject();
        BSONObject endCond = new BasicBSONObject();
        cond.put("Id", 512);
        endCond.put("Id", 1024);
        cl.split(sourceRGName, targetRGName, cond, endCond);
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            e.printStackTrace();
            assertTrue(false);
        }
        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing normalSplitRange()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void normalSplitPercent() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "range");
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        double percent = 50;
        cl.split(sourceRGName, targetRGName, percent);
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            e.printStackTrace();
            assertTrue(false);
        }
        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing hashSplitPercent()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void hashSplitRangeAsync() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "hash");
        conf.put("Partition", 1024);
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        BSONObject cond = new BasicBSONObject();
        BSONObject endCond = new BasicBSONObject();
        cond.put("Partition", 512);
        endCond.put("partition", 1024);
        long taskID = -1;
        taskID = cl.splitAsync(sourceRGName, targetRGName, cond, endCond);
        assertTrue(taskID != -1);
        long[] taskIDs = new long[1];
        taskIDs[0] = taskID;
        System.out.println("taskID = " + taskID);
        sdb.waitTasks(taskIDs);

        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing hashSplitRangeAsync()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void hashSplitPercentAsync() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "hash");
        conf.put("Partition", 1024);
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        double percent = 50;
        long taskID = -1;
        taskID = cl.splitAsync(sourceRGName, targetRGName, percent);
        assertTrue(taskID != -1);
        long[] taskIDs = new long[1];
        taskIDs[0] = taskID;
        System.out.println("taskID = " + taskID);
        sdb.waitTasks(taskIDs);

        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing hashSplitPercentAsync()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void normalSplitRangeAsync() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "range");
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        BSONObject cond = new BasicBSONObject();
        BSONObject endCond = new BasicBSONObject();
        cond.put("Id", 512);
        endCond.put("Id", 1024);
        long taskID = -1;
        taskID = cl.splitAsync(sourceRGName, targetRGName, cond, endCond);
        assertTrue(taskID != -1);
        long[] taskIDs = new long[1];
        taskIDs[0] = taskID;
        System.out.println("taskID = " + taskID);
        sdb.waitTasks(taskIDs);

        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing normalSplitRangeAsync()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void normalSplitPercentAsync() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "range");
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 1000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        double percent = 50;

        long taskID = -1;
        taskID = cl.splitAsync(sourceRGName, targetRGName, percent);
        assertTrue(taskID != -1);
        long[] taskIDs = new long[1];
        taskIDs[0] = taskID;
        System.out.println("taskID = " + taskID);
        sdb.waitTasks(taskIDs);

        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue(recordNum > 0);
        ddb.disconnect();
        System.out.println("Finish testing hashSplitPercentAsync()");
        System.out.println("");
    }

    @Test
    @Ignore
    public void splitAsyncTaskTest() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ShardingKey", new BasicBSONObject("Id", 1));
        conf.put("ShardingType", "range");
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        int num = 10000;
        ConstantsInsert.insertRecords(cl, num);
        long recordNum = cl.getCount();
        assertTrue(num == recordNum);

        BSONObject cond1 = new BasicBSONObject();
        BSONObject cond2 = new BasicBSONObject();
        BSONObject cond3 = new BasicBSONObject();
        BSONObject endCond1 = new BasicBSONObject();
        BSONObject endCond2 = new BasicBSONObject();
        BSONObject endCond3 = new BasicBSONObject();
        cond1.put("Id", 0);
        endCond1.put("Id", num / 3);
        cond2.put("Id", num / 3);
        endCond2.put("Id", 2 * (num / 3));
        cond3.put("Id", 2 * (num / 3));
        endCond3.put("Id", num);
        long taskID1 = -1;
        long taskID2 = -1;
        long taskID3 = -1;
        BSONObject condition = new BasicBSONObject("SplitValue", new BasicBSONObject("$gt", new BasicBSONObject("", 3000)));
        BSONObject selector = new BasicBSONObject("TaskID", "");
        BSONObject orderBy = new BasicBSONObject("TaskID", -1);
        BSONObject hint = new BasicBSONObject("", "$id");
        taskID1 = cl.splitAsync(sourceRGName, targetRGName, cond1, endCond1);
        taskID2 = cl.splitAsync(sourceRGName, targetRGName, cond2, endCond2);
        taskID3 = cl.splitAsync(sourceRGName, targetRGName, cond3, endCond3);
        assertTrue(taskID1 != -1);
        assertTrue(taskID2 != -1);
        assertTrue(taskID2 != -1);
        DBCursor cur = sdb.listTasks(condition, selector, orderBy, hint);
        assertTrue(cur != null);
        int count = 0;
        while (cur.hasNext()) {
            BSONObject obj = cur.getNext();
            System.out.println("obj is: " + obj.toString());
            count++;
        }
        assertTrue(count == 2);
        sdb.cancelTask(taskID2, true);
        sdb.cancelTask(taskID3, false);
        long[] taskIDs = new long[3];
        taskIDs[0] = taskID1;
        taskIDs[1] = taskID2;
        taskIDs[2] = taskID3;
        System.out.println("taskID1 = " + taskID1);
        System.out.println("taskID2 = " + taskID2);
        System.out.println("taskID2 = " + taskID3);
        sdb.waitTasks(taskIDs);

        Sequoiadb ddb = new Sequoiadb(host, nodePort, "", "");
        boolean flag = ddb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1);
        assertTrue(flag);
        CollectionSpace cs1 = ddb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        assertTrue(cs1 != null);
        flag = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(flag);
        DBCollection cl1 = cs1.getCollection(Constants.TEST_CL_NAME_1);
        recordNum = 0;
        recordNum = cl1.getCount();
        System.out.println("The record num in target group is " + recordNum);
        assertTrue((recordNum > 0) && (recordNum <= num / 3));
        ddb.disconnect();
        System.out.println("Finish testing splitAsyncTaskTest()");
        System.out.println("");
    }
}
