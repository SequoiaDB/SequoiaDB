package com.sequoiadb.test.db;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class SdbAnalyze {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void anzyzeAll()
    {
        sdb.analyze();
    }

    @Test
    public void anzyzeCS()
    {
        BSONObject options = new BasicBSONObject();
        options.put( "CollectionSpace", Constants.TEST_CS_NAME_1 ) ;
        sdb.analyze(options);
    }

    @Test
    public void anzyzeCL()
    {
        BSONObject options = new BasicBSONObject();
        options.put( "Collection", Constants.TEST_CS_NAME_1 + "." + Constants.TEST_CL_NAME_1 ) ;
        sdb.analyze(options);
    }
}
