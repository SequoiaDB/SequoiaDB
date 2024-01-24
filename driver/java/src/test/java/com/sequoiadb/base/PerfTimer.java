package com.sequoiadb.base;

public class PerfTimer {
    private long begin;
    private long end;

    public void start() {
        begin = System.currentTimeMillis();
        end = begin;
    }

    public void stop() {
        end = System.currentTimeMillis();
    }

    public long duration() {
        return end - begin;
    }
}
