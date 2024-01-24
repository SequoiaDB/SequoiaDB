package com.sequoiadb.test.rg;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class ActiveLocationTest {
    private static Sequoiadb sdb = null;
    private static ReplicaGroup rg = null;
    private static final String GZ = "GuangZhou";
    private static final String ACTIVE_LOCATION = "ActiveLocation";


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
    public void setUp() {
        Node master = rg.getMaster();
        master.setLocation(GZ);
    }

    @After
    public void dropDown() {
        Node master = rg.getMaster();
        master.setLocation("");
        rg.setActiveLocation("");
    }

    @Test
    public void testActiveLocation() {
        // test active location
        String activeInfo = "";
        rg.setActiveLocation(GZ);
        activeInfo = getActiveInfo();
        Assert.assertEquals(activeInfo, GZ);

        // test clean active location
        rg.setActiveLocation("");
        activeInfo = getActiveInfo();
        Assert.assertEquals(activeInfo, "");

        // test active location with null
        try {
            rg.setActiveLocation(null);
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }
    }

    private String getActiveInfo() {
        BSONObject match = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        match.put("GroupID", rg.getId());
        selector.put(ACTIVE_LOCATION, "");
        try (DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPS, match, selector, null)) {
            while (cursor.hasNext()) {
                return (String) cursor.getNext().get(ACTIVE_LOCATION);
            }
        }
        return null;
    }
}
