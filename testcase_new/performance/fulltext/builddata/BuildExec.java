package com.sequoiadb.builddata;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public enum BuildExec {

    EXECSERVICE;

    private ExecutorService exec;

    public void init() {
        exec = Executors.newCachedThreadPool();
    }

    public void fini() {
        exec.shutdown();
        try {
            exec.awaitTermination(Long.MAX_VALUE, TimeUnit.NANOSECONDS);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void execute(Runnable workTh) {
        exec.execute(workTh);
    }
}
