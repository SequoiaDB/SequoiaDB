package com.sequoiadb.test.cs;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;

public class RenameCLTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    public final static String TEST_CL_OLDNAME = "oldCL";
    public final static String TEST_CL_NEWNAME = "newCL";
    
    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (!sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1))
            cs=sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        else
            cs=sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        /*Create OLDCL and delete NEWCL */
        if(!cs.isCollectionExist(TEST_CL_OLDNAME))
            cs.createCollection(TEST_CL_OLDNAME);
        if(cs.isCollectionExist(TEST_CL_NEWNAME))
            cs.dropCollection(TEST_CL_NEWNAME);
    }

    @After
    public void tearDown() throws Exception {
        /*Create OLDCL and delete NEWCL */
        try {
            if(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1))
                sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            if(cs.isCollectionExist(TEST_CL_NEWNAME))
                cs.dropCollection(TEST_CL_NEWNAME);
            if(cs.isCollectionExist(TEST_CL_OLDNAME))
                cs.dropCollection(TEST_CL_OLDNAME);
        }finally {
            sdb.close();
        }

    }

    @Test
    public void test() {
        String oldName=TEST_CL_OLDNAME;
        String newName=TEST_CL_NEWNAME;
        cs.renameCollection(oldName, newName);
        assertEquals(false, cs.isCollectionExist(oldName));
        assertEquals(true, cs.isCollectionExist(newName));
    }

}
