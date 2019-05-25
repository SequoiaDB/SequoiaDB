package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static org.junit.Assert.*;

public class TestInsert extends SingleCSCLTestCase {

    @Override
    public void setUp() {
        super.setUp();
        cl.truncate();
    }

    @Override
    public void tearDown() {
        super.tearDown();
        cl.truncate();
    }

    @Test
    public void testInsert() {
        BSONObject obj = new BasicBSONObject();
        obj.put("str", "hello");
        obj.put("int", 123);
        obj.put("double", 1234.567);

        cl.insert(obj);
        assertEquals(1, cl.getCount());

        DBCursor cursor = cl.query();
        assertTrue(cursor.hasNext());
        BSONObject res = cursor.getNext();
        assertTrue(res.equals(obj));
        assertFalse(cursor.hasNext());
        cursor.close();
    }

    @Test
    public void testBulkInsert() {
        final int n = 100;
        List<BSONObject> objs = new ArrayList<BSONObject>(n);
        Random rand = new Random();

        for (int i = 0; i < n; i++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("id", i);
            obj.put("str", "hello");
            obj.put("int", rand.nextInt());
            obj.put("double", 1234.567);

            objs.add(obj);
        }

        cl.bulkInsert(objs, 0);
        assertEquals(n, cl.getCount());

        BSONObject orderby = new BasicBSONObject();
        orderby.put("id", 1);

        List<BSONObject> res = new ArrayList<BSONObject>(n);
        DBCursor cursor = cl.query(null, null, orderby, null);
        try {
            for (int i = 0; i < n; i++) {
                assertTrue(cursor.hasNext());
                BSONObject obj;
                if (i % 2 == 0) {
                    obj = cursor.getNext();
                } else {
                    byte[] bytes = cursor.getNextRaw();
                    obj = Helper.decodeBSONBytes(bytes);
                }
                res.add(obj);

                BSONObject curObj = cursor.getCurrent();
                assertEquals(obj, curObj);
            }
            assertFalse(cursor.hasNext());
        } finally {
            cursor.close();
        }

        for (int i = 0; i < n; i++) {
            assertEquals(objs.get(i), res.get(i));
        }

        cl.truncate();
        assertEquals(0, cl.getCount());
    }
}
