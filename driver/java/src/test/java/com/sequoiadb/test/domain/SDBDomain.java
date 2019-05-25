package com.sequoiadb.test.domain;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.*;

import static org.junit.Assert.assertTrue;


public class SDBDomain {
    private static Sequoiadb sdb;
    private static Domain dm;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;
    private static long i = 0;
    private static boolean isCluster = true;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
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
        BSONObject options = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", Constants.GROUPNAME);
        options.put("Groups", arr);
        if (sdb.isDomainExist(Constants.TEST_DOMAIN_NAME)) {
            sdb.dropDomain(Constants.TEST_DOMAIN_NAME);
            dm = sdb.createDomain(Constants.TEST_DOMAIN_NAME, options);
        } else
            dm = sdb.createDomain(Constants.TEST_DOMAIN_NAME, options);
    }

    @After
    public void tearDown() throws Exception {
        if (!isCluster)
            return;
        sdb.dropDomain(Constants.TEST_DOMAIN_NAME);
    }

    @Test
    public void Sdb_DomainGlobal() {
        if (!isCluster)
            return;
        String dmName = "test_domain_name";
        Domain domain1 = sdb.createDomain(dmName, null);
        Domain domain2 = sdb.getDomain(dmName);
        String name = domain2.getName();
        assertTrue(name.equals(dmName));
        try {
            sdb.dropDomain(dmName);
        } catch (BaseException e) {
            assertTrue(false);
        }
    }

}
