package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class TestAggregate extends SingleCSCLTestCase {
    @Test
    public void testAggregate() {
        final int n = 100;
        List<BSONObject> objs = new ArrayList<BSONObject>(n);
        Random rand = new Random();

        for (int i = 0; i < n; i++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("num", i);
            obj.put("group", "hello");
            obj.put("int", rand.nextInt());
            obj.put("double", 1234.567);

            objs.add(obj);
        }

        cl.bulkInsert(objs, 0);
        assertEquals(n, cl.getCount());

        int sum = (n - 1) * n / 2;

        BSONObject group = new BasicBSONObject();
        group.put("_id", "$group");
        BSONObject sumObj = new BasicBSONObject();
        sumObj.put("$sum", "$num");
        group.put("sum", sumObj);

        BSONObject obj = new BasicBSONObject();
        obj.put("$group", group);

        List<BSONObject> list = new ArrayList<BSONObject>();
        list.add(obj);
        DBCursor cursor = cl.aggregate(list);
        assertTrue(cursor.hasNext());
        BSONObject result = cursor.getNext();
        int retSum = ((Double) result.get("sum")).intValue();
        assertEquals(sum, retSum);
        cursor.close();
    }
}
