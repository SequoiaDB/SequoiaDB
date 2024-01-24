package com.sequoiadb.sdb;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-20135:interruptOperation()接口测试
 * @Author XiaoNi Huang
 * @Date 2019.10.31
 */

public class InterruptOperation20135 extends SdbTestBase {
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    /*
     * interruptOperation： 当前操作已完成，不影响；当前操作未完成，则中断。
     * 说明：驱动只简单验证接口调用没有问题，功能在mysql已充分验证。
     */
    @Test
    public void test() {
        DBCursor cursor = sdb.listCollectionSpaces();
        sdb.interruptOperation();
        while ( cursor.hasNext() ) { // 已getMore的不影响
            cursor.getNext();
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }
}
