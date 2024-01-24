package com.sequoiadb.faultmodule.msg;

import java.util.concurrent.LinkedBlockingQueue;

public class FaultMQ {

    private LinkedBlockingQueue< FaultMsg > msgQueue = new LinkedBlockingQueue<>();

    public void push( FaultMsg msg ) {
        try {
            msgQueue.put( msg );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }

    public FaultMsg pop() {
        FaultMsg m = null;
        try {
            m = msgQueue.take();
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
        return m;
    }

    public void clear() {
        msgQueue.clear();
    }

    public int getSize() {
        return msgQueue.size();
    }

    @Override
    public String toString() {
        return msgQueue.toString();
    }
}
