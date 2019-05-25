package com.sequoiadb.test.db;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.Random;

import static org.junit.Assert.assertTrue;

public class SdbSetSessionAttr {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static ReplicaGroup rg;
    private static Node node;
    private static DBCursor cursor;
    private static boolean isCluster = true;

	/*
    @BeforeClass
	public static void setConnBeforeClass() throws Exception{
		isCluster = Constants.isCluster();
		try{
			sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","");
			BSONObject conf = new BasicBSONObject();
			conf.put("PreferedInstance", 3);
			sdb.setSessionAttr(conf);
			shard = sdb.getShard("group1");
			node = shard.createNode("ubuntu-dev1", Constants.SERVER3,
					                Constants.DATAPATH3,
					                new HashMap<String, String>());
			node.start();
		} catch (BaseException e){
			System.out.println(e.getMessage());
			e.printStackTrace();
			return;
		}
	}
	
	@AfterClass
	public static void DropConnAfterClass() throws Exception {
		try{
			node.stop();
			shard.removeNode("ubuntu-dev1", Constants.SERVER3, null);
			sdb.disconnect();
		} catch (BaseException e){
			System.out.println(e.getMessage());
			e.printStackTrace();
			return;
		}
	}
	
	@Before
	public void setUp() throws Exception {
		if(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)){
			sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		}
		else
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		BSONObject conf = new BasicBSONObject();
		conf.put("ReplSize", 0);
		cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
	}

	@After
	public void tearDown() throws Exception {
		sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
	}

	@Test
	public void setSessionAttr(){
		if(!isCluster)
			return;
		BSONObject record = new BasicBSONObject();
		record.put("a", 1);
		cl.insert(record);
		BSONObject conf = new BasicBSONObject();
		conf.put("PreferedReplica", "s");
		sdb.setSessionAttr(conf);
		long num1 = 0;
		long num2 = 0;
	    final int num = 10;
		Sequoiadb ddb = new Sequoiadb("ubuntu-dev1", Constants.SERVER3, "", "");
		BSONObject selector = new BasicBSONObject("TotalRead", "");
		cursor = ddb.getSnapshot(6, null, selector, null);
		while(cursor.hasNext()) {
			num1 = (Long)cursor.getNext().get("TotalRead");
			break;
		}
		for( int i = 0; i < num; i++ ){
			cl.query();
		}
		cursor = ddb.getSnapshot(6, null, selector, null);
		while(cursor.hasNext()) {
			num2 = (Long)cursor.getNext().get("TotalRead");
			break;
		}
		System.out.println("num = " + num + " , num1 = " + num1 + ", num2 = " + num2);
		assertTrue( num <= (num2 - num1) );
	}
	*/

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        isCluster = Constants.isCluster();
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void setSessionAttr_test_arguments() {
        if (!isCluster)
            return;
        String[] str = {"M", "m", "S", "s", "A", "a"};
        int[] in = {1, 2, 3, 4, 5, 6, 7};
        Random random = new Random();
        int r1 = random.nextInt(str.length);
        int r2 = random.nextInt(in.length);
        BSONObject conf = new BasicBSONObject();
        if (random.nextInt(2) == 0) {
            System.out.println("r1 is " + r1);
            conf.put("PreferedInstance", str[r1]);
        } else {
            System.out.println("r2 is " + r2);
            conf.put("PreferedInstance", in[r2]);
        }
        try {
            sdb.setSessionAttr(conf);
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
        final int num = 10;
        for (int i = 0; i < num; i++) {
            cl.query();
        }
    }

    @Test
    public void setSessionAttr_test_array() {
        if (!isCluster)
            return;
        BSONObject conf = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", 1);
        arr.put("1", "S");
        conf.put("PreferedInstance", arr);
        conf.put("PreferedInstanceMode", "ordered");
        try {
            sdb.setSessionAttr(conf);
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
        final int num = 10;
        for (int i = 0; i < num; i++) {
            cl.query();
        }
    }

    @Test
    public void setSessionAttr_test_timeout() {
        if (!isCluster)
            return;
        BSONObject conf = new BasicBSONObject();
        conf.put("Timeout", -1);
        try {
            sdb.setSessionAttr(conf);
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
    }

    @Test
    public void getSessionAttr_test() {
        if (!isCluster)
            return;
        try {
            BSONObject result = sdb.getSessionAttr();
            System.out.println(result.toString());
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
    }

    @Test
    public void getSessionAttr_data_test() {
        if (isCluster)
            return;
        try {
            BSONObject result = sdb.getSessionAttr();
            assertTrue( null == result );
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
    }
}
