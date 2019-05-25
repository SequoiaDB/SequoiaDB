package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class TestGetQueryMeta extends SingleCSCLTestCase {
    @Test
    public void testGetQueryMeta() {
        BSONObject doc = new BasicBSONObject();
        doc.put("test", 100);
        cl.insert(doc);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("test", 100);

        DBCursor cursor = cl.getQueryMeta(matcher, null, null, 0, -1, 0);
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        assertFalse(cursor.hasNext());
    }
}
