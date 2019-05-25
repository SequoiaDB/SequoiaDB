package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestUpdate extends SingleCSCLTestCase {
    @Test
    public void testUpdate() {
        BSONObject doc = new BasicBSONObject();
        doc.put("int", 100);
        cl.insert(doc);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("int", 100);

        BSONObject inc = new BasicBSONObject();
        inc.put("int", 10);

        BSONObject modifier = new BasicBSONObject();
        modifier.put("$inc", inc);

        cl.update(matcher, modifier, null);

        DBCursor cursor = cl.query();
        assertTrue(cursor.hasNext());
        BSONObject retDoc = cursor.getNext();
        int val = (Integer) retDoc.get("int");
        assertEquals(110, val);
        assertFalse(cursor.hasNext());
    }
}
