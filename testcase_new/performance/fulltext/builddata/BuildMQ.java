package com.sequoiadb.builddata;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

public enum BuildMQ {
    MSGQUEUE;

    private ConcurrentLinkedQueue<String> msgQueue = new ConcurrentLinkedQueue<>();
    private AtomicInteger hasPullCount = new AtomicInteger(0);

    public void push(String msg) {
        msgQueue.add(msg);
    }

    public String peek() {
        return msgQueue.peek();
    }

    public String pop() {
        String msg = msgQueue.poll();
        hasPullCount.incrementAndGet();
        return msg;
    }

    public boolean empty() {
        return msgQueue.isEmpty();
    }

    public void clear() {
        msgQueue.clear();
    }

    public int getLenth() {
        return msgQueue.size();
    }

    public int getPullCount() {
        return hasPullCount.get();
    }

    @Override
    public String toString() {
        return msgQueue.toString();
    }
}
