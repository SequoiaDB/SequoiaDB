package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestMainCollection extends SingleCSCLTestCase {
    @Test
    public void testMainCL() {
        String mainCLName = "main_" + ObjectId.get().toString();
        String subCLName = csName + "." + clName;
        BSONObject options = new BasicBSONObject();
        options.put("IsMainCL", true);
        BSONObject shardingKey = new BasicBSONObject();
        shardingKey.put("_id", 1);
        options.put("ShardingKey", shardingKey);
        options.put("ShardingType", "range");
        options.put("ReplSize", -1);
        DBCollection mainCL = cs.createCollection(mainCLName, options);

        try {
            BSONObject attachOptions = new BasicBSONObject();
            BSONObject lowObj = new BasicBSONObject();
            lowObj.put("_id", new MinKey());
            BSONObject upObj = new BasicBSONObject();
            upObj.put("_id", new MaxKey());
            attachOptions.put("LowBound", lowObj);
            attachOptions.put("UpBound", upObj);
            mainCL.attachCollection(subCLName, attachOptions);

            BSONObject obj = new BasicBSONObject();
            obj.put("test", ObjectId.get());
            mainCL.insert(obj);

            mainCL.detachCollection(subCLName);

            DBCursor cursor = cl.query();
            assertTrue(cursor.hasNext());
            BSONObject retObj = cursor.getNext();
            assertEquals(obj, retObj);
            assertFalse(cursor.hasNext());
        } catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_RTN_CMD_NO_SERVICE_AUTH.getErrorCode()) {
                throw e;
            }
        } finally {
            if (mainCL != null) {
                cs.dropCollection(mainCLName);
            }
        }
    }
}
