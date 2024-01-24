package com.sequoiadb.threadexecutor;

import com.sequoiadb.threadexecutor.annotation.parser.MethodInfo;

class WorkerErrMsg {
    private int step;
    private Object obj;
    private MethodInfo methodInfo;
    private Exception e;

    public WorkerErrMsg(Object errObj, MethodInfo method, int step, Exception e) {
        this.obj = errObj;
        this.methodInfo = method;
        this.step = step;
        this.e = e;
    }

    public int getStep() {
        return step;
    }

    public Object getObj() {
        return obj;
    }

    public MethodInfo getMethodInfo() {
        return methodInfo;
    }

    public Exception getException() {
        return e;
    }

}
