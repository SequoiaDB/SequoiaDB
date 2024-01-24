package com.sequoiadb.test.node;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Node.NodeStatus;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.*;

import static org.junit.Assert.assertTrue;

public class NoteTest {

    private static Sequoiadb sdb;
    private static ReplicaGroup rg = null;
    private static Node node = null;
    private static final String nodeHost = Constants.DATA_HOST;
    private static final int nodePort = Constants.DATA_PORT;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        isCluster = Constants.isCluster();
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (!isCluster)
            return;
        // rg
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
        // node
        node = rg.getNode(nodeHost, nodePort);
    }

    @After
    public void tearDown() throws Exception {
        if (!isCluster)
            return;
    }

    @Test
    public void test() {
        if (!isCluster)
            return;
        assertTrue(0 == 0);
    }

//    @Ignore
    @Test
    public void traverseClassNode() {
        if (!isCluster)
            return;
        // getNodeId
        int id = 0;
        id = node.getNodeId();
        assertTrue(id != 0);
        // getShard
        ReplicaGroup s = null;
        s = node.getReplicaGroup();
        assertTrue(s != null);
        // connect
        Sequoiadb connect = null;
        DBCursor cursor = null;
        connect = node.connect();
        cursor = connect.getList(4, null, null, null);
        assertTrue(connect != null);
        assertTrue(cursor != null);
        // disconnect
        connect.disconnect();
        try {
            cursor = connect.getList(4, null, null, null);
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_NOT_CONNECTED"));
        }
        // getSdb
        Sequoiadb ddb = null;
        ddb = node.getSdb();
        assertTrue(ddb != null);
        // getHostName
        String hostName = null;
        hostName = node.getHostName();
        assertTrue(hostName != null);
        // getHost
        int port = 0;
        port = node.getPort();
        assertTrue(port == nodePort);
        // getNodeName
        String nodeName = null;
        nodeName = node.getNodeName();
        System.out.println(nodeName);
        // getStatus
        NodeStatus status = NodeStatus.SDB_NODE_UNKNOWN;
        status = node.getStatus();
        assertTrue(status != NodeStatus.SDB_NODE_UNKNOWN);
    }

    @Test
    public void locationTest() {
        try {
            node.setLocation(null);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        setAndCheckLocation(node, "shanghai");

        setAndCheckLocation(node, "guangzhou.nansha");

        setAndCheckLocation(node, "");
    }

    private void setAndCheckLocation(Node node, String location) {
        // 1. set location
        node.setLocation(location);

        // 2. query location
        String actualLocation = "";
        BSONObject matcher = new BasicBSONObject();
        matcher.put("GroupName", node.getReplicaGroup().getGroupName());
        matcher.put("Group.NodeID", node.getNodeId());

        BSONObject selector = new BasicBSONObject();
        selector.put("Group.Location", "");

        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPS, matcher, selector, null)){
            BSONObject obj = cursor.getNext();
            BasicBSONList nodeList = (BasicBSONList) obj.get("Group");
            Assert.assertNotNull(nodeList);
            Assert.assertEquals(1, nodeList.size());
            BSONObject nodeObj = (BasicBSONObject)nodeList.get(0);

            if (nodeObj.containsField("Location")) {
                actualLocation = (String)nodeObj.get("Location");
            }
        }

        // 3. check location
        Assert.assertEquals(location, actualLocation);
    }
}
