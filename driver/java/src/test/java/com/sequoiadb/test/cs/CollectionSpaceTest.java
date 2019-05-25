package com.sequoiadb.test.cs;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

import static org.junit.Assert.*;

public class CollectionSpaceTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, null, null);
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }

        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);

        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testCreateCL1() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        if (!cs.isCollectionExist(Constants.TEST_CL_NAME_2)) {
            cs.createCollection(Constants.TEST_CL_NAME_2, conf);
        }

        DBCollection dbc = cs.getCollection(Constants.TEST_CL_NAME_2);
        assertNotNull(dbc);
        assertTrue(cs.isCollectionExist(Constants.TEST_CL_NAME_2));
    }

    @Test
    public void testDropCL() throws Exception {
        if (cs.isCollectionExist(Constants.TEST_CL_NAME_1))
            cs.dropCollection(Constants.TEST_CL_NAME_1);
        assertFalse(cs.isCollectionExist(Constants.TEST_CL_NAME_1));

    }

    @Test
    public void testGetCL() {
        DBCollection dbc = cs.getCollection(Constants.TEST_CL_NAME_1);
        assertNotNull(dbc);
    }

    @Test
    public void testGetCLNames() {
        List<String> list = cs.getCollectionNames();
        assertNotNull(list);
        assertTrue(1 == list.size());
        assertTrue(list.get(0).compareTo(Constants.TEST_CS_NAME_1 +
            "." + Constants.TEST_CL_NAME_1) == 0);
    }

    @Test
    public void testIsCLExist() {
        boolean result = cs.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(result);
    }

    @Test
    public void testDrop() throws Exception {
        sdb.dropCollectionSpace(cs.getName());
        Thread.sleep(1000);
        assertFalse(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1));
    }

}
