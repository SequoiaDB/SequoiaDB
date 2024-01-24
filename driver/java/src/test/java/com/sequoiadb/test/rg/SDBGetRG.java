package com.sequoiadb.test.rg;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.junit.*;

import java.util.Collection;
import java.util.Random;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

public class SDBGetRG {
    private static Sequoiadb sdb;
    private static ReplicaGroup rg;
    private static Node node;
    private static int groupID;
    private static String groupName;
    private static String Name;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        isCluster = Constants.isCluster();
        if (!isCluster)
            return;
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        if (!isCluster)
            return;
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (!isCluster)
            return;
        rg = sdb.getReplicaGroup(1000);
        Name = rg.getGroupName();
        groupID = Constants.GROUPID;
        groupName = Name;
    }

    @After
    public void tearDown() throws Exception {
        if (!isCluster)
            return;
    }

    @Test
    public void getGroupTest() {
        if (!isCluster) {
            return;
        }
        try {
            groupName = "SYSCatalogGroup12345";
            rg = sdb.getReplicaGroup(groupName);
            Assert.fail("should get SDB_CLS_GRP_NOT_EXIST(-154) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_GRP_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
        try {
            rg = sdb.getReplicaGroup(0);
            Assert.fail("should get SDB_CLS_GRP_NOT_EXIST(-154) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_GRP_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void getNodeTest() {
        if (!isCluster) {
            return;
        }
        // case 1: normal case
        groupName = "SYSCatalogGroup";
        rg = sdb.getReplicaGroup(groupName);
        Node master = rg.getMaster();
//        System.out.println(String.format("group is: %s, master is: %s", groupName, master.getNodeName()));
        String hostName = master.getHostName();
        int hostPort = master.getPort();
        Node node1 = rg.getNode(hostName, hostPort);
//        System.out.println(String.format("group is: %s, node1 is: %s", groupName, node1.getNodeName()));
        Node node2 = rg.getNode(hostName + ":" + hostPort);
//        System.out.println(String.format("group is: %s, node2 is: %s", groupName, node2.getNodeName()));
        // case 2: get a node which is not exist
        Node node3 = null;
        try {
            node3 = rg.getNode("ubuntu", 30000);
            Assert.fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_NODE_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
        try {
            node3 = rg.getNode(hostName, 0);
            Assert.fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_CLS_NODE_NOT_EXIST.getErrorCode(), e.getErrorCode());
        }
        // case 3: get a node from empty group
        groupName = "groupNoteExist";
        rg = sdb.createReplicaGroup(groupName);
        try {
            node3 = rg.getNode(hostName, 0);
            Assert.fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
        } catch (BaseException e) {
            Assert.assertEquals("SDB_CLS_NODE_NOT_EXIST", e.getErrorType());
        } finally {
            sdb.removeReplicaGroup(groupName);
        }

    }

    @Test
    public void getReplicaGroupById() {
        if (!isCluster)
            return;
        rg = sdb.getReplicaGroup(groupID);
        int id = rg.getId();
        assertEquals(groupID, id);
    }

    @Test
    public void getReplicaGroupByName() {
        if (!isCluster)
            return;
        groupName = Constants.CATALOGRGNAME;
        rg = sdb.getReplicaGroup(groupName);
        String name = rg.getGroupName();
        assertEquals(groupName, name);
    }

    @Test
    public void getCataReplicaGroupById() {
        if (!isCluster)
            return;
        groupID = 1;
        rg = sdb.getReplicaGroup(groupID);
        int id = rg.getId();
        assertEquals(groupID, id);
        boolean f = rg.isCatalog();
        assertTrue(f);
    }

    @Test
    public void getCataReplicaGroupByName() {
        if (!isCluster)
            return;
        groupName = Constants.CATALOGRGNAME;
        rg = sdb.getReplicaGroup(groupName);
        String name = rg.getGroupName();
        assertEquals(groupName, name);
        boolean f = rg.isCatalog();
        assertTrue(f);
    }

    @Test
    public void getCataReplicaGroupByName1() {
        if (!isCluster)
            return;
        try {
            // no replica group can name start wtih "SYS"
            groupName = "SYSCatalogGroupForTest";
            rg = sdb.createReplicaGroup(groupName);
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(),
                e.getErrorCode());
            return;
        }
        assertTrue(false);
    }

    private int getMasterPosition(ReplicaGroup group)
    {
        int primaryNodePosition = 0;
        BSONObject groupInfoObj = group.getDetail();
        if (groupInfoObj == null) {
            throw new BaseException(SDBError.SDB_CLS_GRP_NOT_EXIST);
        }
        // check the nodes in current group
        Object nodesInfoArr = groupInfoObj.get("Group");
        if (nodesInfoArr == null || !(nodesInfoArr instanceof BasicBSONList)) {
            throw new BaseException(SDBError.SDB_SYS,
                    String.format("invalid content[%s] of field[%s]",
                            nodesInfoArr == null ? "null" : nodesInfoArr.toString(), "Group"));
        }
        BasicBSONList nodesInfoList = (BasicBSONList) nodesInfoArr;
        if (nodesInfoList.isEmpty()) {
            throw new BaseException(SDBError.SDB_CLS_EMPTY_GROUP);
        }
        // check whether there has primary or not
        Object primaryNodeId = groupInfoObj.get("PrimaryNode");
        boolean hasPrimary = true;
        if (primaryNodeId == null) {
            hasPrimary = false;
        } else if (!(primaryNodeId instanceof Number)) {
            throw new BaseException(SDBError.SDB_SYS, "invalid primary node's information: " + primaryNodeId.toString());
        } else if (primaryNodeId.equals(Integer.valueOf(-1))){
            hasPrimary = false;
        }
        // try to mark the position of primary node in the nodes list,
        // the value of position is [1, 7]
        for (int i = 0; i < nodesInfoList.size(); i++) {
            BSONObject nodeInfo = (BSONObject) nodesInfoList.get(i);
            Object nodeIdValue = nodeInfo.get("NodeID");
            if (nodeIdValue == null) {
                throw new BaseException(SDBError.SDB_SYS, "node id can not be null");
            }
            if (hasPrimary && nodeIdValue.equals(primaryNodeId)) {
                primaryNodePosition = i + 1;
            }
        }
        return primaryNodePosition;
    }

    @Test
    public void getMasterAndSlaveNodeTest() {
        if (!isCluster)
            return;
        groupName = "SYSCatalogGroup";
        rg = sdb.getReplicaGroup(groupName);
        BSONObject detail = rg.getDetail();
        BasicBSONList nodeList = (BasicBSONList)detail.get("Group");
        int nodeCount = nodeList.size();
        int primaryNodePosition = getMasterPosition(rg);
        assertTrue(nodeCount != 0);

        Node master = null;
        Node slave = null;

        // case 1
        master = rg.getMaster();
        slave = rg.getSlave();
        System.out.println(String.format("case1: group is: %s, master is: %s, slave is: %s", groupName,
                master == null ? null : master.getNodeName(),
                slave == null ? null : slave.getNodeName()));
        if (nodeCount == 1) {
            assertEquals(master.getNodeName(), slave.getNodeName());
        } else {
            assertNotEquals(master.getNodeName(), slave.getNodeName());
        }

        // case 2
        slave = rg.getSlave(1,2,3,4,5,6,7);
        System.out.println(String.format("case2: group is: %s, master is: %s, slave is: %s", groupName,
                master == null ? null : master.getNodeName(),
                slave == null ? null : slave.getNodeName()));
        if (nodeCount == 1) {
            assertEquals(master.getNodeName(), slave.getNodeName());
        } else {
            assertNotEquals(master.getNodeName(), slave.getNodeName());
        }

        // case 3
        Random random = new Random();
        int pos1 = random.nextInt(7) + 1;
        int pos2 = 0;
        while(true) {
            pos2 = random.nextInt(7) + 1;
            if (pos2 != pos1) {
                break;
            }
        }
        //pos1 = 4;pos2=7;
        slave = rg.getSlave(pos1, pos2);
        System.out.println(String.format("case3: group is: %s, master is: %s, slave is: %s", groupName,
                master == null ? null : master.getNodeName(),
                slave == null ? null : slave.getNodeName()));
        if (nodeCount == 1) {
            assertEquals(master.getNodeName(), slave.getNodeName());
        } else {
            if ((pos1 % nodeCount == pos2 % nodeCount) &&
                    (primaryNodePosition == (pos1 - 1) % nodeCount + 1)) {
                assertEquals(master.getNodeName(), slave.getNodeName());
            } else {
                assertNotEquals(master.getNodeName(), slave.getNodeName());
            }
        }

        // case 4
        int[] nullArray = null;
        slave= rg.getSlave(nullArray);
        int[] emptyArray = new int[0];
        slave= rg.getSlave(emptyArray);
        Collection<Integer> nullPoint = null;
        slave= rg.getSlave(nullPoint);
        System.out.println(String.format("case4: group is: %s, master is: %s, slave is: %s", groupName,
                master == null ? null : master.getNodeName(),
                slave == null ? null : slave.getNodeName()));
        if (nodeCount == 1) {
            assertEquals(master.getNodeName(), slave.getNodeName());
        } else {
            assertNotEquals(master.getNodeName(), slave.getNodeName());
        }
    }

    @Test
    @Ignore
    public void groupTmpTest() {
        if (!isCluster)
            return;
        groupName = "db2";
        rg = sdb.getReplicaGroup(groupName);
        BSONObject detail = rg.getDetail();
        BasicBSONList nodeList = (BasicBSONList) detail.get("Group");
        int nodeCount = nodeList.size();
        int primaryNodePosition = getMasterPosition(rg);
        assertTrue(nodeCount != 0);

        Node master = null;
        Node slave = null;

        // case 1
        //master = rg.getMaster();
        slave = rg.getSlave();
        System.out.println(String.format("case1: group is: %s, master is: %s, slave is: %s", groupName,
                master == null ? null : master.getNodeName(),
                slave == null ? null : slave.getNodeName()));
        int counter1 = 0, counter2 = 0;
        String str1 = "susetzb:40000", str2 = "susetzb:42000";
        for(int i = 0; i < 100; i++) {
            slave = rg.getSlave(1,2,3,4,5,6,7);
            if (str1.equals(slave.getNodeName())) {
                counter1++;
            } else if(str2.equals(slave.getNodeName())) {
                counter2++;
            }
        }
        System.out.println("counter1 is: " + counter1 + ", counter2 is: " + counter2);
    }

}
