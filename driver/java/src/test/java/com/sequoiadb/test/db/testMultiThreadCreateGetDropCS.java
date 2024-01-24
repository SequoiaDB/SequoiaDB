package com.sequoiadb.test.db;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.junit.*;


public class testMultiThreadCreateGetDropCS {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);

        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {

        }
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void tesThreadCreateGetDropCS() {
        int THREAD_COUNT = 10;
//		Thread[] testThread = new Thread[THREAD_COUNT];
//		for ( int i = 0 ; i < THREAD_COUNT ; i ++ ) {
//			testThread[i] = new Thread(new MultiThreadCreateGetDropCS());
//			testThread[i].start();
//		}
//		for (int i = 0; i < THREAD_COUNT; i++) {
//			try {
//				testThread[i].join();
//			} catch (InterruptedException e) {
//				e.printStackTrace();
//			}
//		}
//		assertFalse(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1));
    }
}
