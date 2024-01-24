package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestDelete extends SingleCSCLTestCase {
    @Test
    public void testDelete() {
        BSONObject doc = new BasicBSONObject();
        doc.put("int", 100);
        cl.insert(doc);

        DBCursor cursor = cl.query();
        assertTrue(cursor.hasNext());
        BSONObject retDoc = cursor.getNext();
        cursor.close();
        assertEquals(doc, retDoc);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("int", 100);

        cl.delete(matcher);

        cursor = cl.query();
        assertFalse(cursor.hasNext());
    }
}
