package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.TestCase;
import com.sequoiadb.test.TestConfig;
import org.junit.Test;

import java.net.SocketTimeoutException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

public class TestConnection extends TestCase {
    @Test
    public void TestConnect() {
        ConfigOptions options = new ConfigOptions();
        options.setConnectTimeout(3000);
        Sequoiadb sdb = new Sequoiadb(
            TestConfig.getSingleHost(),
            Integer.valueOf(TestConfig.getSinglePort()),
            TestConfig.getSingleUsername(),
            TestConfig.getSinglePassword(),
            options);
        sdb.disconnect();
    }

    @Test
    public void TestConnectTimeout() {
        ConfigOptions options = new ConfigOptions();
        options.setMaxAutoConnectRetryTime(1000);
        options.setConnectTimeout(2000);
        try {
            Sequoiadb sdb = new Sequoiadb("10.10.10.10", 65533, "", "", options);
            fail("connection should be timeout");
            sdb.disconnect();
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_NETWORK.getErrorCode(), e.getErrorCode());
            assertEquals(SocketTimeoutException.class, e.getCause().getClass());
        }
    }
}
