package com.sequoiadb.test.db;

import com.sequoiadb.base.*;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertTrue;

public class SdbTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void getHostNameAndPortTest() {
        String hostName = sdb.getHost();
        int port = sdb.getPort();
        assertTrue(hostName != null && !hostName.isEmpty());
        assertTrue(port != 0);
    }

    @Test
    public void infoEncryptionTest() {
        ClientOptions options = new ClientOptions();

        // case 1: check default value
        assertTrue(options.getInfoEncryption());

        // case 2: check set
        options.setInfoEncryption(false);
        Assert.assertFalse(options.getInfoEncryption());
    }
}
