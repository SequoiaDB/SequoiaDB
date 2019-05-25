package com.sequoiadb.test.rg;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Node.NodeStatus;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.junit.*;

import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class ShardTest {

    private static Sequoiadb sdb;
    private static ReplicaGroup rg = null;
    private static DBCursor cursor;
    private static final int PORT = 54300;

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
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
    }

    @After
    public void tearDown() throws Exception {
    }

    @Ignore
    @Test
    public void traverseClassShard() {
        boolean cata = rg.isCatalog();
        assertFalse(cata);
        Sequoiadb s = rg.getSequoiadb();
        assertTrue(s.equals(sdb));
        int id = 0;
        id = rg.getId();
        assertTrue(id != 0);
        String name = "";
        name = rg.getGroupName();
        assertTrue(name.equals(Constants.GROUPNAME));
        int num = 0;
        num = rg.getNodeNum(NodeStatus.SDB_NODE_ALL);
        assertTrue(num != 0);
        BSONObject detail = null;
        detail = rg.getDetail();
        assertTrue(detail != null);
        Node master = null;
        master = rg.getMaster();
        assertTrue(master != null);
        Node slave = null;
        slave = rg.getSlave();
        assertTrue(slave != null);
        Node node1 = null;
        node1 = rg.getNode(master.getNodeName());
        assertTrue(node1 != null);
        Node node2 = null;
        node2 = rg.getNode(slave.getHostName(), slave.getPort());
        assertTrue(node2 != null);
        Map<String, String> conf = new HashMap<String, String>();
        conf.put("logfilesz", "32");
        Node node = rg.createNode(Constants.HOST, PORT, Constants.DATAPATH4, conf);
        assertTrue(node != null);
        node.start();
        try {
            Thread.currentThread().sleep(15000);
        } catch (InterruptedException e) {
        }
        Sequoiadb ddb = null;
        ddb = new Sequoiadb(Constants.HOST, PORT, "", "");
        assertTrue(ddb != null);
        node.stop();
        try {
            Thread.currentThread().sleep(3000);
        } catch (InterruptedException e) {
        }
        ddb = null;
        try {
            ddb = new Sequoiadb(Constants.HOST, 54300, "", "");
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_NETWORK"));
        }
        assertTrue(ddb == null);
        rg.removeNode(Constants.HOST, PORT, null);
        node = null;
        node = rg.getNode(Constants.HOST, PORT);
        assertTrue(node == null);
    }

}
