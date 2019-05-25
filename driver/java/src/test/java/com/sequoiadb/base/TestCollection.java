package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSTestCase;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestCollection extends SingleCSTestCase {

    @Test
    public void testCreateDrop() {
        String clName = "testCreateDrop";
        assertFalse(cs.isCollectionExist(clName));

        DBCollection cl = cs.createCollection(clName);
        assertEquals(clName, cl.getName());
        assertEquals(csName, cl.getCSName());
        assertEquals(sdb, cl.getSequoiadb());
        assertEquals(0, cl.getCount());

        assertTrue(cs.isCollectionExist(clName));
        cs.dropCollection(clName);
        assertFalse(cs.isCollectionExist(clName));
    }
}
