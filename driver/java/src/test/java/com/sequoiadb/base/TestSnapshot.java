package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

public class TestSnapshot extends SingleCSCLTestCase {
    @Test
    public void testResetSnapshot() {
        sdb.resetSnapshot();

        sdb.resetSnapshot( null );
        
        BSONObject option = new BasicBSONObject();
        option.put("Type", "database");
        sdb.resetSnapshot(option);
    }
}
