package com.sequoiadb.test.db;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.Sequoiadb.SptEvalResult;
import com.sequoiadb.base.Sequoiadb.SptReturnType;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class SdbJSProcedures {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

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
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
    }

    @After
    public void tearDown() throws Exception {
        /* rmProcedure */
        String name = "sum_in_java";
        try {
            sdb.rmProcedure(name);
        } catch (BaseException e) {
        }
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void JSProcedures() {
        System.out.println("Running test1...");
        // check whether it is in the standalone environment
        try {
            sdb.listReplicaGroups();
        } catch (BaseException e) {
            int errno = e.getErrorCode();
            if (SDBError.SDB_RTN_COORD_ONLY.getErrorCode() == errno) {
                System.out.println("This test is for cluster environment only.");
                return;
            }
        }
        /* crtJSProcedure */
        String code = "function sum_in_java(x, y){return x + y ;}";
        BSONObject option1 = new BasicBSONObject();
        option1.put("GroupName", Constants.BACKUPGROUPNAME);
        option1.put("Path", Constants.BACKUPPATH);
        option1.put("Name", Constants.BACKUPNAME);
        option1.put("Description", "This is test in java.");
        option1.put("EnsureInc", false);
        option1.put("OverWrite", false);
        try {
            sdb.crtJSProcedure(code);
        } catch (BaseException e) {
            System.out.println("Failed to create js procedure");
            System.out.println("Error message is: " + e.getMessage() + e.getErrorCode());
            assertTrue(false);
        }
        /* listProcedure */
        BSONObject cond = new BasicBSONObject();
        cond.put("name", "sum_in_java");
        try {
            cursor = sdb.listProcedures(cond);
        } catch (BaseException e) {
            System.out.println("Failed to list js procedure");
            System.out.println("Error message is: " + e.getMessage() + e.getErrorCode());
        }
        int count = 0;
        while (cursor.hasNext()) {
            System.out.println("js procedure is: " + cursor.getNext().toString());
            count++;
        }
        assertEquals(1, count);
        /* evalJS with right name */
        String code1 = "sum_in_java(1, 2)";
        SptEvalResult evalResult1 = null;
        try {
            evalResult1 = sdb.evalJS(code1);
            cursor = null;
            cursor = evalResult1.getCursor();
            assertTrue(cursor != null);
            while (cursor.hasNext()) {
                System.out.println("In evalResult1, cursor data is: " + cursor.getNext());
            }
        } catch (BaseException e) {
            System.out.println("Failed to eval js procedure with right js function name");
        } finally {
            System.out.println("In evalResult1, returnType is:��" + evalResult1.getReturnType());
            assertTrue(evalResult1.getErrMsg() == null);
        }
		/* evalJS with wrong name */
        String code2 = "sum_in_java1(1, 2)";
        SptEvalResult evalResult2 = null;

        evalResult2 = sdb.evalJS(code2);
        cursor = null;
        cursor = evalResult2.getCursor();
        assertTrue(cursor == null);
        SptReturnType retType = null;
        retType = evalResult2.getReturnType();
        assertTrue(retType == null);
        BSONObject errObj = null;
        errObj = evalResult2.getErrMsg();
        assertTrue(errObj != null);
        System.out.println("While running eval with wrong name," +
            " error message is: " + errObj.toString());

		/* rmProcedure */
        String name = "sum_in_java";
        try {
            sdb.rmProcedure(name);
        } catch (BaseException e) {
            System.out.println("Failed to remove js procedure");
            System.out.println("Error message is: " + e.getMessage() + e.getErrorCode());
        }
    }

    @Test
    public void EvalJS() {
        System.out.println("Running test2...");
        // check whether it is in the standalone environment
        try {
            sdb.listReplicaGroups();
        } catch (BaseException e) {
            int errno = e.getErrorCode();
            if (SDBError.SDB_RTN_COORD_ONLY.getErrorCode() == errno) {
                System.out.println("This test is for cluster environment only.");
                return;
            }
        }
		/* crtJSProcedure */
        String code = "function sum_in_java(x, y){return x + y ;}";
        BSONObject option1 = new BasicBSONObject();
        option1.put("GroupName", Constants.BACKUPGROUPNAME);
        option1.put("Path", Constants.BACKUPPATH);
        option1.put("Name", Constants.BACKUPNAME);
        option1.put("Description", "This is test in java.");
        option1.put("EnsureInc", false);
        option1.put("OverWrite", false);
        try {
            sdb.crtJSProcedure(code);
        } catch (BaseException e) {
            System.out.println("Failed to create js procedure");
            System.out.println("Error message is: " + e.getMessage() + e.getErrorCode());
            assertTrue(false);
        }
		/* eval js and check the return value */
        String code1 = "sum_in_java(1, 2)";
        SptEvalResult evalResult1 = null;
        // eval
        evalResult1 = sdb.evalJS(code1);
        // check the return data
        cursor = null;
        cursor = evalResult1.getCursor();
        assertTrue(cursor != null);
        while (cursor.hasNext()) {
            System.out.println("In evalResult, cursor data is: " + cursor.getNext());
        }
        // check the return type
        SptReturnType retType = null;
        retType = evalResult1.getReturnType();
        assertTrue(retType != null);
        assertTrue(retType == SptReturnType.TYPE_NUMBER);
        // check the return error message
        BSONObject errObj = evalResult1.getErrMsg();
        assertTrue(errObj == null);

		/* rmProcedure */
        String name = "sum_in_java";
        try {
            sdb.rmProcedure(name);
        } catch (BaseException e) {
            System.out.println("Failed to remove js procedure");
            System.out.println("Error message is: " + e.getMessage() + e.getErrorCode());
        }
    }

}
