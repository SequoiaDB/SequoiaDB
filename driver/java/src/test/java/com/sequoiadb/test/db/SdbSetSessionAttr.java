package com.sequoiadb.test.db;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.junit.*;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;

import static org.junit.Assert.assertEquals;
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
			// sdb
			sdb = new Sequoiadb(Constants.COOR_NODE_CONN,"","");
			// todo:
			BSONObject conf = new BasicBSONObject();
			conf.put("PreferedInstance", 3);
			sdb.setSessionAttr(conf);
			// create another node
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
		// cs
		if(sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)){
			sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
			cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
		}
		else
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
	public void setSessionAttr(){
		if(!isCluster)
			return;
		// insert one record
		BSONObject record = new BasicBSONObject();
		record.put("a", 1);
		cl.insert(record);
		// todo:
		BSONObject conf = new BasicBSONObject();
		conf.put("PreferedReplica", "s");
		sdb.setSessionAttr(conf);
		// check
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
        // test
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
        BSONObject conf1 = new BasicBSONObject();
        BSONObject conf2 = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", 1);
        arr.put("1", "S");

        // case 1: PreferedInstance
        conf1.put("PreferedInstance", arr);
        conf1.put("PreferedInstanceMode", "ordered");
        setConfAndCheck( conf1 );

        // case 2: PreferredInstance
        arr.put("1", "M");
        conf2.put("PreferredInstance", arr);
        conf2.put("PreferredInstanceMode", "ordered");
        setConfAndCheck( conf2 );
    }

    private void setConfAndCheck( BSONObject conf ){
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

        BSONObject result = sdb.getSessionAttr( false );
        for ( String key: conf.keySet() ){
            assertEquals( conf.get( key ), result.get( key ));
        }
    }

    @Test
    public void setSessionAttr_test_timeout() {
        if (!isCluster)
            return;
        BSONObject conf = new BasicBSONObject();
        conf.put("Timeout", -1);
        // test
        try {
            sdb.setSessionAttr(conf);
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
    }

    @Test
    public void setSessionAttr_test_trans() {
        BSONObject options = new BasicBSONObject();
        options.put("TransIsolation", 1);
        options.put("TransTimeout", 120);
        options.put("TransLockWait", true);
        options.put("TransUseRBS", false);
        options.put("TransAutoCommit", true);
        options.put("TransAutoRollback", false);

        sdb.setSessionAttr(options);
        BSONObject sessionAttr = sdb.getSessionAttr();
        System.out.println(sessionAttr);
        String expectString = "{ \"PreferedInstance\" : \"M\" , \"PreferedInstanceMode\" : \"random\" , \"PreferedStrict\" : false , \"Timeout\" : -1 , \"TransIsolation\" : 1 , \"TransTimeout\" : 120 , \"TransUseRBS\" : false , \"TransLockWait\" : true , \"TransAutoCommit\" : true , \"TransAutoRollback\" : false }";
        BSONObject expectObject =(BSONObject)JSON.parse(expectString);
        System.out.println(expectObject);

        boolean result = sessionAttr.equals(expectObject);
        Assert.assertEquals(sessionAttr.get("TransIsolation"),expectObject.get("TransIsolation"));
        Assert.assertEquals(sessionAttr.get("TransTimeout"),expectObject.get("TransTimeout"));
        Assert.assertEquals(sessionAttr.get("TransLockWait"),expectObject.get("TransLockWait"));
        Assert.assertEquals(sessionAttr.get("TransUseRBS"),expectObject.get("TransUseRBS"));
        Assert.assertEquals(sessionAttr.get("TransAutoCommit"),expectObject.get("TransAutoCommit"));
        Assert.assertEquals(sessionAttr.get("TransAutoRollback"),expectObject.get("TransAutoRollback"));
    }

    @Test
    public void getSessionAttr_test() {
        if (!isCluster)
            return;
        try {
            // case 1: getSessionAttr() test
            BSONObject result = sdb.getSessionAttr();
            BSONObject result2 = sdb.getSessionAttr();
            System.out.println(result.toString());
            Assert.assertTrue( result == result2);
            BSONObject result3 = sdb.getSessionAttr(false);
            Assert.assertTrue( result != result3);
            Assert.assertTrue( result.equals(result3));
            // case 2: setSessionAttr() test
            sdb.setSessionAttr(new BasicBSONObject());
            BSONObject result4 = sdb.getSessionAttr(true);
            Assert.assertTrue( result3 != result4);
            Assert.assertTrue( result3.equals(result4));
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }

    }

    @Test
    public void getSessionAttr_data_test() {
        if (isCluster)
            return;
        // test
        try {
            BSONObject result = sdb.getSessionAttr();
            assertTrue( null == result );
        } catch (BaseException e) {
            System.out.println(e.getMessage());
            assertTrue(false);
        }
    }

    @Test
    public void setSessionAttr_IgnoreCase_test() {

        Object resultObj;
        BasicBSONObject option;
        String originalKey = "PreferedInstance";
        String lowercaseKey = "preferedinstance";
        String uppercaseKey = "PREFEREDINSTANCE";
        String useKey;

        // test value
        String value1 = "M";
        String value2 = "a";
        String value3 = "-A";
        String value4 = "-s";
        int value5 = 1;
        Object[] value6 = {1,"A"};
        Object[] value7 = {1,"m"};
        Object[] value8 = {1,"-s"};
        Object[] value9 = {1,"-A"};

        // expected object
        String expected1 = "M";
        String expected2 = "A";
        String expected3 = "-A";
        String expected4 = "-S";
        int expected5 = 1;
        BasicBSONList expected6 = new BasicBSONList();
        expected6.put(0,1);
        expected6.put(1,"A");
        BasicBSONList expected7 = new BasicBSONList();
        expected7.put(0,1);
        expected7.put(1,"M");
        BasicBSONList expected8 = new BasicBSONList();
        expected8.put(0,1);
        expected8.put(1,"-S");
        BasicBSONList expected9 = new BasicBSONList();
        expected9.put(0,1);
        expected9.put(1,"-A");

        Map<Object,Object> caseMap = new HashMap<>();
        // case 1-9:
        caseMap.put(value1,expected1);
        caseMap.put(value2,expected2);
        caseMap.put(value3,expected3);
        caseMap.put(value4,expected4);
        caseMap.put(value5,expected5);
        caseMap.put(value6,expected6);
        caseMap.put(value7,expected7);
        caseMap.put(value8,expected8);
        caseMap.put(value9,expected9);

        int i = 1;
        for (Object value: caseMap.keySet()) {
            Object expected = caseMap.get(value);
            useKey = ((i++)%2 == 0)? lowercaseKey:uppercaseKey;     // use lowercaseKey or uppercaseKey
            option = new BasicBSONObject(useKey,value);
            sdb.setSessionAttr(option);
            resultObj = sdb.getSessionAttr().get(originalKey);
            assertTrue(resultObj.equals(expected));
            option.remove(useKey);
        }
    }

}
