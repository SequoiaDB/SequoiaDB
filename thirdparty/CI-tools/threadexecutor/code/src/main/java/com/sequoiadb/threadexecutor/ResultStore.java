package com.sequoiadb.threadexecutor;

public class ResultStore {
    private int retCode = 0;
    private Throwable e = null;

    public void saveResult(int retCode, Throwable e) {
        this.retCode = retCode;
        this.e = e;
    }

    public void clearResult() {
        retCode = 0;
        e = null;
    }

    public int getRetCode() {
        return retCode;
    }

    public Throwable getThrowable() {
        return e;
    }
}
