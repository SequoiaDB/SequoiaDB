package com.sequoiadb.test.bug;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;
import java.util.Arrays;

/**
 * Created by tanzhaobo on 2018/1/9.
 */
public class Bug_JIRA_3191 {
    private static SequoiadbDatasource ds;
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() {
        ConfigOptions configOptions = new ConfigOptions();
        configOptions.setSocketTimeout(5000);
        DatasourceOptions datasourceOptions = new DatasourceOptions();
        ds = new SequoiadbDatasource(Arrays.asList(Constants.COOR_NODE_CONN), "", "",
                configOptions, datasourceOptions);
        try {
            sdb = ds.getConnection();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (Exception e) {
            e.printStackTrace();
        }
        ds.releaseConnection(sdb);
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
        try {
            cl.delete("");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    @Ignore
    public void jira_3191() {
        cl.insert(new BasicBSONObject("a", 1));
        System.out.println("begin to stop coord node");
        try {
            Thread.sleep(15000);
        } catch (InterruptedException e1) {
        }
        System.out.println("finish sleeping");
        try {
            DBCursor cursor1 = cl.query();
        }catch (Exception e) {
            int i = 15;
            while(i-- != 0) {
                System.out.println("i is: " + i);
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e1) {
                }
            }
            e.printStackTrace();
        }
        try {
            DBCursor cursor2 = sdb.listCollections();
            Assert.assertFalse(true);
        } catch (BaseException e) {
            e.printStackTrace();
            Assert.assertEquals(SDBError.SDB_NOT_CONNECTED.getErrorCode(), e.getErrorCode());
        }
    }
}
