package com.sequoiadb.commlib;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

/**
 * 建议所有用例都要实现这个接口，以保证用例结构一致
 * 
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */

public interface StandTestInterface {
    @BeforeClass
    void setup();

    @AfterClass
    void tearDown();
}
