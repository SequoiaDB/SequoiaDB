package com.sequoiadb.test.cs;


import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;


public class DropCreateGetCL {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCollection cl2;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Test
    public void creategetCL() {
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        assertTrue(cs.isCollectionExist(Constants.TEST_CL_NAME_1));
    }

    @Test
    public void testGetCL() {
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        assertTrue(cs.isCollectionExist(Constants.TEST_CL_NAME_1));
        cl = cs.getCollection(Constants.TEST_CL_NAME_1);
        assertNotNull(cl);
        String cName = cl.getName();
        assertEquals(Constants.TEST_CL_NAME_1, cName);
    }

    @Test
    public void testCreateSameCL() {
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        assertTrue(cs.isCollectionExist(Constants.TEST_CL_NAME_1));
        try {
            cl2 = cs.createCollection(Constants.TEST_CL_NAME_1);
        } catch (BaseException e) {
            int errono = e.getErrorCode();
            assertEquals(-22, errono);
            return;
        }
    }

    @Test
    public void testCreateOtherCL() {
        DBCollection cl2 = cs.createCollection(Constants.TEST_CL_NAME_2);
        assertNotNull(cl2);
        cs.dropCollection(Constants.TEST_CL_NAME_2);
    }

    @Test
    public void testListCL() {
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        assertTrue(cs.isCollectionExist(Constants.TEST_CL_NAME_1));
        List<String> listCL = new ArrayList<String>();
        listCL.addAll(cs.getCollectionNames());
        assertEquals(listCL.size(), 1);
        String cName = listCL.get(0);
        assertEquals(Constants.TEST_CL_FULL_NAME1, cName);
        cs.dropCollection(Constants.TEST_CL_NAME_1);
    }

    @Test
    public void testCreate_127_CL() {
        String cl_127 = "";
        for (int i = 0; i < 127; i++) {
            cl_127 += "a";
        }
        if (cs.isCollectionExist(cl_127))
            cs.dropCollection(cl_127);
        cs.createCollection(cl_127);
        assertTrue(cs.isCollectionExist(cl_127));
    }

    @Test(expected = BaseException.class)
    public void testCreate_128_CL() {
        String cl_128 = "";
        for (int i = 0; i < 128; i++) {
            cl_128 += "a";
        }
        cs.createCollection(cl_128);
        assertFalse(cs.isCollectionExist(cl_128));
    }

    @Test(expected = BaseException.class)
    public void testCreateIllegalCL() {
        String arr[] = {".", "SYS", "$"};
        String name = "bar";
        for (int i = 0; i < arr.length; i++) {
            String cl_name = arr[i] + name;
            cs.createCollection(cl_name);
            assertFalse(cs.isCollectionExist(cl_name));
        }
        cs.createCollection("");
        assertFalse(cs.isCollectionExist(""));
    }
}
