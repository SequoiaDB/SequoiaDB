package com.sequoiadb.test.domain;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.junit.*;

import java.util.ArrayList;
import java.util.Random;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class DomainTest {
    private static Sequoiadb sdb = null;
    private static ArrayList<String> groupList;
    private static boolean standaloneFlag = false;
    private static String domainName = "t_dom_1";

    private String tmpDomain = "t_dom_tmp";
    private String csName1 = "dom_cs_1";
    private String clName1 = "dom_cl_1";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        getDataGroups();
        if (!standaloneFlag)
            createDomain(domainName, ((groupList.size() - 1) == 0) ? 1 : (groupList.size() - 1), true);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        if (!standaloneFlag)
            dropDomain(domainName);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void domainCreateDrop() {
        if (standaloneFlag) return;
        boolean result;

        Random r = new Random();

        tmpDomain = tmpDomain + r.nextInt(100);

        result = createDomain(tmpDomain, 2);
        assertEquals(!standaloneFlag, result);

        result = dropDomain(tmpDomain);
        assertEquals(!standaloneFlag, result);
    }

    @Test
    public void csOfDomain() {
        boolean result;
        if (standaloneFlag) return;

        result = createCSofDomain(csName1, domainName);
        assertEquals(!standaloneFlag, result);

        if (result) {
            result = createHashCL(clName1, csName1);
            assertEquals(!standaloneFlag, result);
            if (!standaloneFlag)
                getCLDetail(csName1 + "." + clName1);
        }
        result = dropCS(csName1);
        assertTrue(result);
    }

    @Test
    public void listDomains() {
        if (standaloneFlag) return;
        boolean result;
        result = listDomainsLow();
        assertEquals(!standaloneFlag, result);
    }

    @Test
    public void listCSInDomin() {
        if (standaloneFlag) return;
        boolean result;
        result = listCSInDomainLow(domainName);
        assertEquals(!standaloneFlag, result);
    }

    @Test
    public void listCLInDomain() {
        if (standaloneFlag) return;
        boolean result;
        result = listCLInDomainLow(domainName);
        assertEquals(!standaloneFlag, result);
    }

    @Test
    public void domainIsExists() {
        if (standaloneFlag) return;
        boolean result;

        result = domainIsExistsLow(domainName);
        assertEquals(!standaloneFlag, result);

        result = domainIsExistsLow(tmpDomain);
        assertEquals(false, result);
    }

    @Test
    public void alterDomain() {
        if (standaloneFlag) return;
        boolean result;

        result = addDataGroupToDomain(domainName, groupList.size());
        assertEquals(!standaloneFlag, result);
    }

    private static void getDataGroups() {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove("SYSCatalogGroup");
            groupList.remove("SYSCoord");
            standaloneFlag = false;
        } catch (BaseException e) {
            standaloneFlag = true;
            groupList = new ArrayList<String>();
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
    }

    private static boolean createDomain(String name, int groupsNum, boolean ignore) {
        boolean flag = false;
        try {
            BSONObject options = new BasicBSONObject();
            options = (BSONObject) JSON.parse("{'Groups': [" + chooseDataGroups(groupsNum) + "],AutoSplit:true}");
            sdb.createDomain(name, options);
            flag = true;
        } catch (BaseException e) {
            if (ignore) {
                SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
                SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
                SDBTestHelper.println("ErrorMess: " + e.getMessage());
            }
        }
        return flag;
    }

    private boolean createDomain(String name, int groupsNum) {
        boolean flag = false;
        try {
            BSONObject options = new BasicBSONObject();
            options = (BSONObject) JSON.parse("{'Groups': [" + chooseDataGroups(groupsNum) + "],AutoSplit:true}");
            sdb.createDomain(name, options);
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private static String chooseDataGroups(int groupsNum) {
        if (standaloneFlag)
            return null;

        int length = (groupsNum > groupList.size()) ? groupList.size() : groupsNum;
        String ret = "";
        for (int i = 0; i < length; i++) {
            ret = ret + "'" + groupList.get(i) + "',";
        }
        return (ret.length() == 0) ? ret : ret.substring(0, ret.length() - 1);
    }

    private static boolean dropDomain(String name) {
        boolean flag = false;
        try {
            sdb.dropDomain(name);
            SDBTestHelper.println("drop domains " + name + " succ!");
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private boolean createCSofDomain(String csName, String domainName) {
        try {
            sdb.dropCollectionSpace(csName);
        } catch (BaseException e) {
        }
        boolean flag = false;
        try {
            BSONObject options = new BasicBSONObject();
            options = (BSONObject) JSON.parse("{Domain:'" + domainName + "'}");
            sdb.createCollectionSpace(csName, options);
            SDBTestHelper.println("create cs " + csName + " at domain:" + domainName + " succ!");
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private boolean dropCS(String csName) {
        boolean flag = false;
        try {
            sdb.dropCollectionSpace(csName);
            SDBTestHelper.println("drop cs " + csName + " succ!");
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private boolean createHashCL(String clName, String csName) {
        boolean flag = false;
        try {
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            BSONObject options = new BasicBSONObject();
            options = (BSONObject) JSON.parse("{ShardingKey:{a:-1,b:1},ShardingType:'hash',Partition:4096}");
            cs.createCollection(clName, options);
            SDBTestHelper.println("create hash-cl " + clName + " succ!");
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private void getCLDetail(String clFullName) {
        try {
            BSONObject matcher = new BasicBSONObject();
            matcher = (BSONObject) JSON.parse("{Name:'" + clFullName + "'}");
            DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS, matcher, null, null);
            while (cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                SDBTestHelper.println(record.toString());
            }
            cursor.close();
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
    }

    private boolean listDomainsLow() {
        boolean flag = false;
        SDBTestHelper.println("List-All-Domains:");
        try {
            DBCursor cursor = sdb.listDomains(null, null, null, null);
            while (cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                SDBTestHelper.println(record.toString());
            }
            cursor.close();
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("List-All-Domains throws exception:");
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        SDBTestHelper.println("List-All-Domains finish");
        return flag;
    }

    private boolean listCSInDomainLow(String domainName) {
        boolean flag = false;
        try {
            Domain domain = sdb.getDomain(domainName);
            DBCursor cursor = domain.listCSInDomain();
            SDBTestHelper.println("List CS of Domain:" + domainName);
            while (cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                SDBTestHelper.println(record.toString());
            }
            cursor.close();
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private boolean listCLInDomainLow(String domainName) {
        boolean flag = false;
        try {
            Domain domain = sdb.getDomain(domainName);
            DBCursor cursor = domain.listCLInDomain();
            SDBTestHelper.println("List CL of Domain:" + domainName);
            while (cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                SDBTestHelper.println(record.toString());
            }
            cursor.close();
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }

    private boolean domainIsExistsLow(String domainName) {
        try {
            return sdb.isDomainExist(domainName);
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
            return false;
        }
    }

    private boolean addDataGroupToDomain(String domainName, int groupsNum) {
        boolean flag = false;
        try {
            Domain domain = sdb.getDomain(domainName);
            BSONObject options = new BasicBSONObject();
            options = (BSONObject) JSON.parse("{'Groups': [" + chooseDataGroups(groupsNum) + "],AutoSplit:true}");
            domain.alterDomain(options);
            listDomains();
            flag = true;
        } catch (BaseException e) {
            SDBTestHelper.println("ErrorCode: rc=" + e.getErrorCode());
            SDBTestHelper.println("ErrorDesc: " + e.getErrorType());
            SDBTestHelper.println("ErrorMess: " + e.getMessage());
        }
        return flag;
    }
}
