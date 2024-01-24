package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.SingleTestCase;
import org.bson.types.ObjectId;
import org.junit.Test;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import static org.junit.Assert.*;

public class TestCollectionSpace extends SingleTestCase {
    private String csName;

    public void setUp() {
        csName = "TestCollectionSpace_" + new ObjectId().toString();
    }

    public void tearDown() {
        csName = null;
    }

    @Test
    public void testCreateDrop() {
        assertFalse(sdb.isCollectionSpaceExist(csName));

        CollectionSpace cs = sdb.createCollectionSpace(csName);
        assertEquals(csName, cs.getName());
        assertEquals(sdb, cs.getSequoiadb());
        assertEquals(0, cs.getCollectionNames().size());

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }

    @Test
    public void testAlter() {
        assertFalse(sdb.isCollectionSpaceExist(csName));

        CollectionSpace cs = sdb.createCollectionSpace(csName);
        assertEquals(csName, cs.getName());
        assertEquals(sdb, cs.getSequoiadb());
        assertEquals(0, cs.getCollectionNames().size());

        BSONObject options = new BasicBSONObject();
        options.put("LobPageSize",8192);
        cs.alterCollectionSpace(options);

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }

    @Test
    public void testSetAttributes() {
        assertFalse(sdb.isCollectionSpaceExist(csName));

        CollectionSpace cs = sdb.createCollectionSpace(csName);
        assertEquals(csName, cs.getName());
        assertEquals(sdb, cs.getSequoiadb());
        assertEquals(0, cs.getCollectionNames().size());

        BSONObject options = new BasicBSONObject();
        options.put("LobPageSize",8192);
        cs.setAttributes(options);

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }

    @Test
    public void testMultiAlter() {
        assertFalse(sdb.isCollectionSpaceExist(csName));

        CollectionSpace cs = sdb.createCollectionSpace(csName);
        assertEquals(csName, cs.getName());
        assertEquals(sdb, cs.getSequoiadb());
        assertEquals(0, cs.getCollectionNames().size());

        BSONObject alterArray = new BasicBSONList();
        BSONObject alterObject = new BasicBSONObject();
        alterObject.put("Name", "set attributes");
        BSONObject alterOption = new BasicBSONObject();
        alterOption.put("PageSize", 1111);
        alterObject.put("Args", alterOption);
        alterArray.put(Integer.toString(0), alterObject);

        alterObject = new BasicBSONObject();
        alterObject.put("Name", "set attributes");
        alterOption = new BasicBSONObject();
        alterOption.put("PageSize", 8192);
        alterObject.put("Args", alterOption);
        alterArray.put(Integer.toString(1), alterObject);

        alterOption = new BasicBSONObject();
        alterOption.put("IgnoreException", true);

        BSONObject options = new BasicBSONObject();
        options.put("Alter", alterArray);
        options.put("Options", alterOption);
        cs.alterCollectionSpace(options);

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }

    @Test
    public void testDropCSWithTrueEnsureEmpty() {
        CollectionSpace cs = sdb.createCollectionSpace(csName);

        BSONObject options = new BasicBSONObject();
        options.put("EnsureEmpty", true);

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName, options);
        assertFalse(sdb.isCollectionSpaceExist(csName));

        CollectionSpace cs2 = sdb.createCollectionSpace(csName);

        String clName = "createCSWithCL";
        DBCollection cl = cs2.createCollection(clName);

        try {
            assertTrue(sdb.isCollectionSpaceExist(csName));
            sdb.dropCollectionSpace(csName, options);
            assertTrue("Exception must have been threw before",false);
        }catch (BaseException e) {
            assertEquals(SDBError.SDB_DMS_CS_NOT_EMPTY.getErrorCode(), e.getErrorCode());
        }

    }

    @Test
    public void testDropCSWithFalseEnsureEmpty() {
        CollectionSpace cs = sdb.createCollectionSpace(csName);

        String clName = "createCSWithFalseEnsureEmpty";
        DBCollection cl = cs.createCollection(clName);

        BSONObject options = new BasicBSONObject();
        options.put("EnsureEmpty", false);

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName, options);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }

    @Test
    public void testDropCSWithoutOption() {
        CollectionSpace cs = sdb.createCollectionSpace(csName);

        String clName = "createCS";
        DBCollection cl = cs.createCollection(clName);

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }
}
