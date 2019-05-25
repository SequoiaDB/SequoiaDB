package com.sequoiadb.test.common;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.TestConfig;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class Constants {
    public final static int PORT = Integer.valueOf(TestConfig.getSinglePort());
    public final static String HOST = TestConfig.getSingleHost();
    public final static int NODE_PORT = TestConfig.getNodePort();
    public final static String NODE_HOST = TestConfig.getNodeHost();
    public final static String BACKUPPATH = "/opt/sequoiadb/database/test/backup";
    public final static String DATAPATH4 = "/opt/sequoiadb/database/test/data4";

    public final static String COOR_NODE_CONN = HOST + ":" + PORT;

    public final static String TEST_CS_NAME_1 = "testfoo";
    public final static String TEST_CS_NAME_2 = "testCS2";

    public final static String TEST_CL_NAME_1 = "testbar";
    public final static String TEST_CL_NAME_2 = "testCL2";
    public final static String TEST_CL_FULL_NAME1 = "testfoo.testbar";
    public final static int SDB_PAGESIZE_4K = 4 * 1024;

    public final static String GROUPNAME = TestConfig.getSingleGroup();
    public final static String TEST_RG_NAME_SRC = "testRGSrc";
    public final static String TEST_RG_NAME_DEST = "testRGDest";
    public final static String CATALOGRGNAME = "SYSCatalogGroup";
    public final static int GROUPID = 1000;

    public final static String TEST_RN_HOSTNAME_SPLIT = "vmsvr2-ubun-x64-2";
    public final static String TEST_RN_PORT_SPLIT = "50100";
    public final static String TEST_INDEX_NAME = "testIndexName";

    public final static String BACKUPNAME = "backup_in_java_test";
    public final static String BACKUPGROUPNAME = GROUPNAME;

    public final static String TEST_DOMAIN_NAME = "domain_java";

    public final static String IXM_INDEXDEF = "IndexDef";
    public final static String IXM_NAME = "name";
    public final static String OID = "_id";

    public static boolean isCluster() {
        Sequoiadb sdb = new Sequoiadb(HOST + ":" + PORT, "", "");
        try {
            BSONObject empty = new BasicBSONObject();
            sdb.getList(7, empty, empty, empty);
        } catch (BaseException e) {
            if (e.getErrorType().equals("SDB_RTN_COORD_ONLY")) ;
            return false;
        }
        return true;
    }


}
