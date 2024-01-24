package com.sequoiadb.autoIncrement;

import org.bson.BSONObject;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;

public class AutoIncrementUtils {
    public static void checkResult( DBCollection cl, int expectNum,
            long expectCurrentValue ) {
        // 校验记录数
        int count = ( int ) cl.getCount();
        Assert.assertEquals( count, expectNum );

        // 获取实际记录并按照id排序（由于驱动生成_id,coord生成自增字段值，因此无法通过_id排序来校验自增字段），则自增字段会从当前值严格递增
        DBCursor cursor = cl.query( "", "", "{id:1}", "" );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            Assert.assertEquals( record.get( "id" ), expectCurrentValue++ );
        }

    }

}
