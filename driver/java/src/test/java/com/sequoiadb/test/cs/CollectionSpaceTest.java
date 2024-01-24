package com.sequoiadb.test.cs;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;

public class CollectionSpaceTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs1;
    private static CollectionSpace cs2;
    private static String domainName = "CollectionSpaceTest_domain";

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, null, null);
        List<String> rgList = new ArrayList<>();
        rgList.add(Constants.GROUPNAME);
        BSONObject options = new BasicBSONObject("Groups", rgList);
        sdb.createDomain( domainName, options);
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        try {
            sdb.dropDomain(domainName);
        }catch (BaseException e){
            sdb.close();
        }
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_2)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_2);
        }

        BSONObject option = new BasicBSONObject("Domain", domainName);
        cs1 = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1, option);
        cs2 = sdb.createCollectionSpace(Constants.TEST_CS_NAME_2);

        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cs1.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        if ( sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1))
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_2);
    }

    @Test
    public void testCreateCL1() {
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        if (!cs1.isCollectionExist(Constants.TEST_CL_NAME_2)) {
            cs1.createCollection(Constants.TEST_CL_NAME_2, conf);
        }

        DBCollection dbc = cs1.getCollection(Constants.TEST_CL_NAME_2);
        assertNotNull(dbc);
        assertTrue(cs1.isCollectionExist(Constants.TEST_CL_NAME_2));
    }

    @Test
    public void testDropCL() throws Exception {
        if (cs1.isCollectionExist(Constants.TEST_CL_NAME_1))
            cs1.dropCollection(Constants.TEST_CL_NAME_1);
        assertFalse(cs1.isCollectionExist(Constants.TEST_CL_NAME_1));
    }

    @Test
    public void testGetCL() {
        DBCollection dbc = cs1.getCollection(Constants.TEST_CL_NAME_1);
        assertNotNull(dbc);
    }

    @Test
    public void testGetCLNames() {
        List<String> list = cs1.getCollectionNames();
        assertNotNull(list);
        assertTrue(1 == list.size());
        assertTrue(list.get(0).compareTo(Constants.TEST_CS_NAME_1 +
            "." + Constants.TEST_CL_NAME_1) == 0);
    }

    @Test
    public void testIsCLExist() {
        boolean result = cs1.isCollectionExist(Constants.TEST_CL_NAME_1);
        assertTrue(result);
    }

    @Test
    public void testDrop() throws Exception {
        sdb.dropCollectionSpace(cs1.getName());
        Thread.sleep(1000);
        assertFalse(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1));
    }

    @Test
    public void testAlter () throws Exception {
        CollectionSpace alterCS = sdb.createCollectionSpace("TestAlterCS");

        BSONObject options = new BasicBSONObject();
        options.put("PageSize", 8192);
        alterCS.setAttributes(options);

        sdb.dropCollectionSpace("TestAlterCS");
    }

    @Test
    public void testNullCollection(){
        String collectionName = null;
        try {
            DBCollection collection = cs1.createCollection(collectionName);
            System.out.println(collection.getFullName());
            System.out.println(cs1.getCollection(collectionName).getFullName());
            cs1.dropCollection(collectionName);
        }catch (BaseException e){
            Assert.assertEquals(e.getErrorCode(),SDBError.SDB_INVALIDARG.getErrorCode());
        }
    }

    @Test
    public void testGetDomainName(){
        String result1 = cs1.getDomainName();
        Assert.assertEquals(domainName, result1);

        String result2 = cs2.getDomainName();
        Assert.assertEquals("", result2);
    }
}
