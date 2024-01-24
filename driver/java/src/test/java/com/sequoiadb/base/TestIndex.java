package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestIndex extends SingleCSCLTestCase {
    private static final String INDEX_DEF = "IndexDef";
    private static final String ID_INDEX_NAME = "$id";
    private static final String FIELD_NAME = "name";
    private static final String FIELD_UNIQUE = "unique";
    private static final String FIELD_DROPDUPS = "dropDups";
    private static final String FIELD_ENFORCED = "enforced";

    @Test
    public void testIndex() {
        DBCursor cursor;

        // only $id at first
        cursor = cl.getIndexes();
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        BSONObject index = (BSONObject) obj.get(INDEX_DEF);
        assertEquals(ID_INDEX_NAME, index.get(FIELD_NAME));
        assertTrue((Boolean) index.get(FIELD_UNIQUE));
        assertFalse((Boolean) index.get(FIELD_DROPDUPS));
        assertTrue((Boolean) index.get(FIELD_ENFORCED));
        assertFalse(cursor.hasNext());
        cursor.close();

        // drop id index
        cl.dropIdIndex();
        cursor = cl.getIndexes();
        assertFalse(cursor.hasNext());
        cursor.close();

        // create id index
        cl.createIdIndex(null);

        // create an normal index
        String name = "a_idx";
        cl.createIndex(name, "{a:1}", false, false);

        cursor = cl.getIndex(name);
        assertTrue(cursor.hasNext());
        obj = cursor.getNext();
        index = (BSONObject) obj.get(INDEX_DEF);
        assertEquals(name, index.get(FIELD_NAME));
        assertFalse((Boolean) index.get(FIELD_UNIQUE));
        assertFalse((Boolean) index.get(FIELD_DROPDUPS));
        assertFalse((Boolean) index.get(FIELD_ENFORCED));
        assertFalse(cursor.hasNext());
        cursor.close();

        cl.dropIndex(name);

        // only $id at last
        cursor = cl.getIndexes();
        assertTrue(cursor.hasNext());
        obj = cursor.getNext();
        index = (BSONObject) obj.get(INDEX_DEF);
        assertEquals(ID_INDEX_NAME, index.get(FIELD_NAME));
        assertTrue((Boolean) index.get(FIELD_UNIQUE));
        assertFalse((Boolean) index.get(FIELD_DROPDUPS));
        assertTrue((Boolean) index.get(FIELD_ENFORCED));
        assertFalse(cursor.hasNext());
        cursor.close();
    }
}
