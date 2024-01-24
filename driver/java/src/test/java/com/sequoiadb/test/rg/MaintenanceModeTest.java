package com.sequoiadb.test.rg;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class MaintenanceModeTest {
    private static Sequoiadb sdb = null;
    private static ReplicaGroup rg = null;
    private static final String GZ = "GuangZhou";

    @BeforeClass
    public static void setUpBeforeClass() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        rg = sdb.getReplicaGroup(Constants.GROUPNAME);
    }

    @AfterClass
    public static void tearDownAfterClass() {
        sdb.close();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
        // clear location
        Node master = rg.getMaster();
        master.setLocation("");
    }

    @Test
    public void testMaintenanceModeWithNodeName() {
        BSONObject options = new BasicBSONObject();
        Node master = rg.getMaster();
        options.put("NodeName", master.getNodeName());
        options.put("MinKeepTime", 10);
        options.put("MaxKeepTime", 100);

        rg.startMaintenanceMode(options);
        Assert.assertTrue(isMaintenanceMode());

        rg.stopMaintenanceMode(options);
        Assert.assertFalse(isMaintenanceMode());
    }

    @Test
    public void testMaintenanceModeLocation() {
        Node master = rg.getMaster();
        master.setLocation(GZ);

        BSONObject options = new BasicBSONObject();
        options.put("Location", GZ);
        options.put("MinKeepTime", 10);
        options.put("MaxKeepTime", 100);

        rg.startMaintenanceMode(options);
        Assert.assertTrue(isMaintenanceMode());

        rg.stopMaintenanceMode();
        Assert.assertFalse(isMaintenanceMode());
    }

    @Test
    public void testInvalidArg() {
        BSONObject options = new BasicBSONObject();
        Node master = rg.getMaster();
        options.put("NodeName", master.getNodeName());
        // min > max
        options.put("MinKeepTime", 1000);
        options.put("MaxKeepTime", 100);
        try {
            rg.startMaintenanceMode(options);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // error key
        options.put("MinKeepTime", 10);
        options.put("abc", "");
        try {
            rg.startMaintenanceMode(options);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // option is null
        try {
            rg.startMaintenanceMode(null);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            rg.stopMaintenanceMode(null);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    private boolean isMaintenanceMode() {
        BSONObject match = new BasicBSONObject();
        match.put("GroupID", rg.getId());
        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPMODES, match, null, null)) {
            while (cursor.hasNext()) {
                BSONObject o = cursor.getNext();
                String mode = (String) o.get("GroupMode");
                if (mode != null && mode.equals("maintenance")) {
                    return true;
                }
            }
        }
        return false;
    }
}
