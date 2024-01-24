package com.sequoiadb.faultmodule.th;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.faultmodule.fault.Fault;
import com.sequoiadb.faultmodule.fault.FaultStage.Stage;
import com.sequoiadb.faultmodule.msg.FaultMQ;
import com.sequoiadb.faultmodule.msg.FaultMsg;

public class FaultWorker extends Thread {

    private volatile static FaultWorker workTh = new FaultWorker();
    public static FaultMQ msgMQ = new FaultMQ();

    private FaultWorker() {
    }

    @Override
    public void run() {
        Thread thisTh = Thread.currentThread();
        while ( workTh == thisTh ) {
            pop2Work();
        }
    }

    private static void init() {
        msgMQ.clear();
    }

    private static void fini() {
        msgMQ.clear();
    }

    public static void startFault() {
        init();
        workTh.start();
    }

    public static void stopFault() {
        workTh = null;
        fini();
    }

    private void pop2Work() {
        FaultMsg m = msgMQ.pop();
        Fault f = m.getFault();
        try {
            if ( f.getStage() == Stage.init && f.getStageStatus() == 0 ) {
                f.init();
            }

            if ( f.getStage() == Stage.init && f.getStageStatus() != 0 ) {
                f.make();
                f.checkMake();
                synchronized ( m ) {
                    m.setProcessed( true );
                    m.notify();
                }
                return;
            }

            if ( f.getStage() == Stage.make && f.getStageStatus() != 0 ) {
                f.restore();
                f.checkRestore();
                synchronized ( m ) {
                    m.setProcessed( true );
                    m.notify();
                }
            }

            f.fini();
        } catch ( Exception e ) {
            try {
                if ( f.getStage() == Stage.restore ) {
                    f.fini();
                }
            } catch ( FaultException e1 ) {
                e1.printStackTrace();
            }
            synchronized ( m ) {
                m.setProcessed( true );
                m.notify();
            }
            m.setMakeExp( e );
        }
    }
}
