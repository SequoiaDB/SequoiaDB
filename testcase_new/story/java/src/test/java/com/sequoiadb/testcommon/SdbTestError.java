package com.sequoiadb.testcommon;

public enum SdbTestError {
    // 虽然现在只有一个错误码和错误描述。以后也许会有用的。
    SDB_TEST_ERROR(-10000, "sdb test error");

    private int code;
    private String desc;

    private SdbTestError( int code, String desc ) {
        this.code = code;
        this.desc = desc;
    }

    @Override
    public String toString() {
        return this.name() + "(" + this.code + ")" + ": " + this.desc;
    }

    public int getErrorCode() {
        return this.code;
    }

    public String getErrorDescription() {
        return this.desc;
    }

    public String getErrorType() {
        return this.name();
    }

    public static SdbTestError getTestError( int errorCode ) {
        switch ( errorCode ) {
        case -10000:
            return SDB_TEST_ERROR;
        default:
            return null;
        }
    }
}
