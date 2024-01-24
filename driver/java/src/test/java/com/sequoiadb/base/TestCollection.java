package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.Binary;
import org.junit.Assert;
import org.junit.Test;

import java.util.Arrays;

import static org.junit.Assert.*;

public class TestCollection extends SingleCSTestCase {

    @Test
    public void testDBQueryEraseFlags() {
        int flags = DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_EXPLAIN | DBQuery.FLG_QUERY_STRINGOUT | DBQuery.FLG_QUERY_WITH_RETURNDATA;
        int newFlags = DBQuery.eraseFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_FORCE_HINT, DBQuery.FLG_QUERY_EXPLAIN, DBQuery.FLG_QUERY_STRINGOUT, DBQuery.FLG_QUERY_WITH_RETURNDATA));
        assertEquals(0, newFlags);
        newFlags = DBQuery.eraseFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_EXPLAIN, DBQuery.FLG_QUERY_STRINGOUT));
        assertEquals(DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA, newFlags);
        flags = -100;
        newFlags = DBQuery.eraseFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_EXPLAIN));
        assertEquals(-1124, newFlags);
        flags = -1;
        newFlags = DBQuery.eraseFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_EXPLAIN, DBQuery.FLG_QUERY_STRINGOUT));
        assertEquals(-1026, newFlags);
    }

    @Test
    public void testDBQueryFilterFlags() {
        int flags = DBQuery.FLG_QUERY_FORCE_HINT |
                    DBQuery.FLG_QUERY_EXPLAIN |
                    DBQuery.FLG_QUERY_STRINGOUT |
                    DBQuery.FLG_QUERY_WITH_RETURNDATA |
                    DBQuery.FLG_QUERY_PARALLED;
        // case 1:
        int newFlags = DBQuery.filterFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_EXPLAIN));
        assertEquals(DBQuery.FLG_QUERY_EXPLAIN, newFlags);
        // case 2:
        newFlags = DBQuery.filterFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_FORCE_HINT, DBQuery.FLG_QUERY_EXPLAIN,
                        DBQuery.FLG_QUERY_STRINGOUT, DBQuery.FLG_QUERY_WITH_RETURNDATA));
        assertEquals(DBQuery.FLG_QUERY_FORCE_HINT|DBQuery.FLG_QUERY_EXPLAIN|
                DBQuery.FLG_QUERY_STRINGOUT|DBQuery.FLG_QUERY_WITH_RETURNDATA, newFlags);
        // case 3:
        newFlags = DBQuery.filterFlags(flags,
                Arrays.asList(DBQuery.FLG_QUERY_FORCE_HINT, DBQuery.FLG_QUERY_EXPLAIN,
                        DBQuery.FLG_QUERY_STRINGOUT, DBQuery.FLG_QUERY_WITH_RETURNDATA,
                        DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE));
        assertEquals(DBQuery.FLG_QUERY_FORCE_HINT|DBQuery.FLG_QUERY_EXPLAIN|
                DBQuery.FLG_QUERY_STRINGOUT|DBQuery.FLG_QUERY_WITH_RETURNDATA, newFlags);
    }

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
