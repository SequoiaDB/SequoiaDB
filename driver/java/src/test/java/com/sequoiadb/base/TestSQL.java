package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestSQL extends SingleCSCLTestCase {
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
    public void testExec() {
        BSONObject obj = new BasicBSONObject();
        obj.put("test", ObjectId.get());

        cl.insert(obj);

        String sql = String.format("select * from %s", cl.getFullName());
        DBCursor cursor = sdb.exec(sql);
        assertTrue(cursor.hasNext());
        BSONObject retObj = cursor.getNext();
        assertEquals(obj, retObj);
        assertFalse(cursor.hasNext());
    }

    @Test
    public void testExecUpdate() {
        ObjectId oid = ObjectId.get();
        ObjectId newOid = ObjectId.get();

        String name = "test";

        BSONObject obj = new BasicBSONObject();
        obj.put(name, oid.toString());
        cl.insert(obj);

        obj.removeField(name);
        obj.put(name, newOid.toString());

        String sql = String.format("update %s set test='%s' where test='%s'",
            cl.getFullName(), newOid.toString(), oid.toString());
        sdb.execUpdate(sql);

        sql = String.format("select * from %s", cl.getFullName());
        DBCursor cursor = sdb.exec(sql);
        assertTrue(cursor.hasNext());
        BSONObject retObj = cursor.getNext();
        assertEquals(obj, retObj);
        assertFalse(cursor.hasNext());
    }
}
