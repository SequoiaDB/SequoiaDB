package com.sequoiadb.test.common;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.TestConfig;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class Constants {
    public final static String HOST = TestConfig.getSingleHost();
    public final static int PORT = Integer.valueOf(TestConfig.getSinglePort());
    //public final static String HOST = TestConfig.getSingleHost();
    public final static String DATA_HOST = TestConfig.getDataHost();
    public final static int DATA_PORT = TestConfig.getDataPort();
    public final static String DB_PATH = TestConfig.getDBPath();
    public final static String BACKUPPATH = "/opt/sequoiadb/database/test/backup";
    public final static String DATAPATH4 = "/opt/sequoiadb/database/test/data4";

    public final static String COOR_NODE_CONN = HOST + ":" + PORT;
    public final static String DATA_NODE_CONN = DATA_HOST + ":" + DATA_PORT;

    // cs
    public final static String TEST_CS_NAME_1 = "testfoo";
    public final static String TEST_CS_NAME_2 = "testCS2";
    public final static String TEST_CS_NAME_3 = "测试cs";
    // cl
    public final static String TEST_CL_NAME_1 = "testbar";
    public final static String TEST_CL_NAME_2 = "testCL2";
    public final static String TEST_CL_NAME_3 = "测试cl";
    public final static String TEST_CL_MAIN_NAME = "testMainCL";
    public final static String TEST_CL_SUB_NAME_1 = "testSubCL1";
    public final static String TEST_CL_SUB_NAME_2 = "testSubCL2";
    public final static String SHARDING_KEY = "testKey";

    public final static String TEST_CL_FULL_NAME1 = "testfoo.testbar";
    public final static int SDB_PAGESIZE_4K = 4 * 1024;

    // replicaGroup
    public final static String GROUPNAME = TestConfig.getDataGroupName();
    public final static String TEST_RG_NAME_SRC = "testRGSrc";
    public final static String TEST_RG_NAME_DEST = "testRGDest";
    public final static String CATALOGRGNAME = "SYSCatalogGroup";
    public final static int GROUPID = 1000;

    // node
    public final static String TEST_RN_HOSTNAME_SPLIT = "vmsvr2-ubun-x64-2";
    public final static String TEST_RN_PORT_SPLIT = "50100";
    public final static String TEST_INDEX_NAME = "testIndexName";

    // backup
    public final static String BACKUPNAME = "backup_in_java_test";
    public final static String BACKUPGROUPNAME = GROUPNAME;

    // domain
    public final static String TEST_DOMAIN_NAME = "domain_java";

    // data source
    public final static String DATASOURCE_ADDRESS = TestConfig.getDatasourceAddress();
    public final static String DATASOURCE_URLS = TestConfig.getDatasourceUrls();
    public final static String DATASOURCE_NAME = "datasource_java";
    public final static String DATASOURCE_TYPE = "SequoiaDB";

    public final static String IXM_INDEXDEF = "IndexDef";
    public final static String IXM_NAME = "name";
    public final static String OID = "_id";

    // user information
    public final static String TEST_USER_NAME = "test_user";
    public final static String TEST_USER_PASSWORD = "123";
    public final static String TEST_USER_TOKEN = "123";
    public final static String TEST_USER_CIPHER_FILE = "./src/test/java/com/sequoiadb/testdata/cipher.txt";

    public static String data4B = "0123";
    public static String data10B = "0123456789";
    public static String data20B = data10B + data10B;
    public static String data40B = data20B + data20B;
    public static String data100B = data40B + data40B + data20B;
    public static String data200B = data100B + data100B;
    public static String data500B = data200B + data200B + data100B;

    public static String data1000B = data500B + data500B;
    public static String data1kb = data1000B;
    public static String data2kb = data1000B + data1000B;
    public static String data4kb = data2kb + data2kb;
    public static String data8kb = data4kb + data4kb;
    public static String data10kb = data4kb + data4kb + data2kb;
    public static String data20kb = data10kb + data10kb;
    public static String data50kb = data20kb + data20kb + data10kb;
    public static String data100kb = data50kb + data50kb;
    public static String data200kb = data100kb + data100kb;
    public static String data500kb = data200kb + data200kb + data100kb;
    public static String data1mb = data500kb + data500kb;
    public static String data2mb = data1mb + data1mb;

    public static String data1KB = data1000B + data20B + data4B;
    public static String data2KB = data1KB + data1KB;
    public static String data4KB = data2KB + data2KB;
    public static String data8KB = data4KB + data4KB;
    public static String data16KB = data8KB + data8KB;
    public static String data32KB = data16KB + data16KB;
    public static String data64KB = data32KB + data32KB;
    public static String data128KB = data64KB + data64KB;
    public static String data256KB = data128KB + data128KB;
    public static String data512KB = data256KB + data256KB;
    public static String data1MB = data512KB + data512KB;
    public static String data2MB = data1MB + data1MB;

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
