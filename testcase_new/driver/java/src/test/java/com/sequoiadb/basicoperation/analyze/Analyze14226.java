package com.sequoiadb.basicoperation.analyze;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * Created by laojingtang on 18-1-31.
 */
public class Analyze14226 extends SdbTestBase {
    private SdbWarpper db;

    @BeforeClass
    public void setup() {
        db = new SdbWarpper( coordUrl );
    }

    @AfterClass
    public void teardown() {
        db.close();
    }

    /**
     * 1.指定不存在的cl，执行统计，检查结果 1.命令行执行失败，提示信息及错误码正确
     */
    @Test
    public void test() {
        try {
            db.analyze( new BasicBSONObject( "Collection",
                    "Analyze14226.Analyze14226" ) );
            Assert.fail(
                    "Should throw SDB_DMS_CS_NOTEXIST(-34): Collection does not exist" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -34 ) {
                throw e;
            }
        }
    }
}
