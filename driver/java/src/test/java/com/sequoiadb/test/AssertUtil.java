package com.sequoiadb.test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import static org.junit.Assert.fail;

public class AssertUtil {
    public static void assertNotThrows(Executable executable) {
        if (executable == null) {
            throw new RuntimeException("executable can not be null");
        }

        try {
            executable.execute();
        } catch (Throwable e) {
            fail(String.format("Expected no exception, but an exception was thrown: %s", e.getMessage()));
        }
    }

    public static void assertSDBError(SDBError expected, Executable executable) {
        if (expected == null) {
            throw new RuntimeException("expected can not be null");
        }
        if (executable == null) {
            throw new RuntimeException("executable can not be null");
        }

        try {
            executable.execute();
            fail("Expected to raise a BaseException, but it did not.");
        } catch (Throwable e) {
            if (!(e instanceof BaseException)) {
                fail(String.format("Expected SDBError to be %s, but it did not: %s",
                        expected.getErrorCode(), e.getMessage()));
            }
        }
    }

    public static void assertNotSDBError(SDBError expected, Executable executable) {
        if (expected == null) {
            throw new RuntimeException("expected can not be null");
        }
        if (executable == null) {
            throw new RuntimeException("executable can not be null");
        }

        try {
            executable.execute();
        } catch (Throwable e) {
            if (e instanceof BaseException && ((BaseException) e).getErrorCode() == expected.getErrorCode()) {
                fail(String.format("Expected SDBError not %s, but it is.", expected.getErrorCode()));
            }
        }
    }

    public interface Executable {
        void execute() throws Throwable;
    }
}
