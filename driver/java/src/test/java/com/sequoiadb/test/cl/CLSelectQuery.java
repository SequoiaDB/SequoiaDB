package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.*;

import static org.junit.Assert.assertEquals;

//import static org.junit.Assert.assertTrue;
//import java.util.ArrayList;
//import java.util.List;
//import java.util.Random;
//import com.sequoiadb.base.DBQuery;
//import com.sequoiadb.exception.BaseException;
//import com.sequoiadb.testdata.SDBTestHelper;

public class CLSelectQuery {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        // sdb connection
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // collection space
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // collection
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        // insert
        BSONObject obj = new BasicBSONObject();
        obj.put("��Ʒ���", 1000001);
        BSONObject arr = new BasicBSONList();
        BSONObject subObj1 = new BasicBSONObject();
        BSONObject subObj2 = new BasicBSONObject();
        subObj1.put("������", "���ӱ�");
        subObj1.put("Ӣ����", "Mobile Digital Pen");
        arr.put("0", subObj1);
        arr.put("1", subObj2);
        obj.put("��Ʒ��", arr);
        obj.put("Ʒ�����", "Mvpen");
        BSONObject arr1 = new BasicBSONList();
        arr1.put("0", "��");
        arr1.put("1", "��");
        arr1.put("2", "��");
        arr1.put("3", "��");
        arr1.put("4", "��");
        arr1.put("5", "��");
        arr1.put("6", "��");
        obj.put("��ɫ", arr1);
        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("���", 86.59);
        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("�߶�", 11.21);
        BSONObject subObj5 = new BasicBSONObject();
        subObj5.put("���", 23.29);
        BSONObject arr2 = new BasicBSONList();
        arr2.put("0", subObj3);
        arr2.put("1", subObj4);
        arr2.put("2", subObj5);
        obj.put("������", arr2);
        BSONObject subObj6 = new BasicBSONObject();
        subObj6.put("���", "�й�");
        subObj6.put("ʡ��", "�㶫");
        subObj6.put("����", "����");
        BSONObject subObj7 = new BasicBSONObject();
        subObj7.put("���", "�й�");
        subObj7.put("ʡ��", "�㽭");
        subObj7.put("����", "����");
        BSONObject subObj8 = new BasicBSONObject();
        subObj8.put("���", "�й�");
        subObj8.put("ʡ��", "�㶫");
        subObj8.put("����", "�麣");
        BSONObject arr3 = new BasicBSONList();
        arr3.put("0", subObj6);
        arr3.put("1", subObj7);
        arr3.put("2", subObj8);
        obj.put("���", arr3);
        obj.put("��װ����", "��װ");
        cl.insert(obj);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Test
    public void testQuery() {
        DBCursor cursor = cl.query();
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            //cursor.getNext();
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testQueryElementMatch() {
        BSONObject matchObj = null;
        BSONObject orderObj = null;
        BSONObject hintObj = null;
        BSONObject selectSubObj1 = new BasicBSONObject();
        selectSubObj1.put("ʡ��", "�㶫");
        BSONObject selectSubObj2 = new BasicBSONObject();
        selectSubObj2.put("$elemMatch", selectSubObj1);
        BSONObject selectObj = new BasicBSONObject();
        selectObj.put("���", selectSubObj2);
        System.out.println(selectObj.toString());
        DBCursor cursor = cl.query(matchObj, selectObj, orderObj, hintObj);
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            //cursor.getNext();
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testQueryElementMatchOne() {
        BSONObject matchObj = null;
        BSONObject orderObj = null;
        BSONObject hintObj = null;
        BSONObject selectSubObj1 = new BasicBSONObject();
        selectSubObj1.put("ʡ��", "�㶫");
        BSONObject selectSubObj2 = new BasicBSONObject();
        selectSubObj2.put("$elemMatchOne", selectSubObj1);
        BSONObject selectObj = new BasicBSONObject();
        selectObj.put("���", selectSubObj2);
        System.out.println(selectObj.toString());
        DBCursor cursor = cl.query(matchObj, selectObj, orderObj, hintObj);
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            //cursor.getNext();
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testQuerySlice() {
        BSONObject matchObj = null;
        BSONObject orderObj = null;
        BSONObject hintObj = null;
        BSONObject selectSubArr1 = new BasicBSONList();
        selectSubArr1.put("0", 4);
        selectSubArr1.put("1", 1);
        BSONObject selectSubObj1 = new BasicBSONObject();
        selectSubObj1.put("$slice", selectSubArr1);
        BSONObject selectObj = new BasicBSONObject();
        selectObj.put("��ɫ", selectSubObj1);
        System.out.println(selectObj.toString());
        DBCursor cursor = cl.query(matchObj, selectObj, orderObj, hintObj);
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            System.out.println(obj.get("��ɫ").toString());
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testQueryDefault() {
        BSONObject matchObj = null;
        BSONObject orderObj = null;
        BSONObject hintObj = null;
        BSONObject selectSubObj1 = new BasicBSONObject();
        selectSubObj1.put("$default", "defaultValue");
        BSONObject selectObj = new BasicBSONObject();
        selectObj.put("������.�߶�", selectSubObj1);
        System.out.println(selectObj.toString());
        DBCursor cursor = cl.query(matchObj, selectObj, orderObj, hintObj);
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            //System.out.println(obj.get("��ɫ").toString());
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testQueryInclude() {
        BSONObject matchObj = null;
        BSONObject orderObj = null;
        BSONObject hintObj = null;
        BSONObject selectSubObj1 = new BasicBSONObject();
        selectSubObj1.put("$include", 0);
        BSONObject selectObj = new BasicBSONObject();
        selectObj.put("���", selectSubObj1);
        System.out.println(selectObj.toString());
        DBCursor cursor = cl.query(matchObj, selectObj, orderObj, hintObj);
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            //System.out.println(obj.get("��ɫ").toString());
            i++;
        }
        assertEquals(1, i);
    }
}

