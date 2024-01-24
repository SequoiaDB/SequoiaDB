package com.sequoiadb.testcommon;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.transaction.TransUtils;

public abstract class SdbThreadBase implements Runnable {
    private List< Throwable > exceptionList = Collections
            .synchronizedList( new ArrayList< Throwable >() );
    private Integer syncRes = new Integer( 0 );
    private Integer syncRunning = new Integer( 0 );
    private Object result = null;
    private AtomicInteger count = new AtomicInteger( 0 );
    private static final int MAX_THREAD_NUMBER = 100;
    private static final int MAX_NUM_PERALLOC = 20;

    private static ExecutorService service = Executors
            .newFixedThreadPool( MAX_THREAD_NUMBER );
    private Thread thread = null;

    public void start() {
        start( 1 );

    }

    public void start( int threadNum ) {
        int threadNumForAllow = 0;
        if ( threadNum > MAX_NUM_PERALLOC ) {
            threadNumForAllow = MAX_NUM_PERALLOC;
        } else {
            threadNumForAllow = threadNum;
        }

        count.set( threadNumForAllow );
        synchronized ( service ) {
            for ( int i = 0; i < threadNumForAllow; i++ ) {
                service.execute( this );
            }
        }
    }

    public static void shutdown() {
        service.shutdown();
    }

    /*
     * -------------------------------------------------------------------------
     * - getExecResult -- 获取线程的执行结果，只适应启动一个线程的情况 必须在线程结束前通过setExecResult进行设置
     * 如果线程没有开始或者已经结束调用直接返回，否则会阻塞到结果被设置 Parameters: Returns:
     * 结果对象实例，如果是一个DBCursor，返回后通过 Object ret = getExecResult() ; if ( ret
     * instanceof DBCursor){ DBCursor cursor = (DBCursor)ret; } --------------
     * ------------------------------------------------------------
     */
    public Object getExecResult() throws InterruptedException {
        if ( thread == null || this.result != null ) {
            return this.result;
        }

        while ( this.result == null ) {
            synchronized ( syncRes ) {
                syncRes.wait( 10 );
            }
        }

        return this.result;
    }

    /*
     * -------------------------------------------------------------------------
     * - setExecResult -- 设置线程的执行结果，只适合启单个线程的情况 Parameters: result: Object
     * 可以是任意对象类型 Returns: void
     * --------------------------------------------------------------
     * ------------
     */
    public void setExecResult( Object result ) {
        assert thread != null;
        this.result = result;
        synchronized ( syncRes ) {
            syncRes.notifyAll();
        }
    }

    private String transacationID;

    public void setTransactionID( Sequoiadb db ) {
        assert thread != null;
        this.transacationID = TransUtils.getTransactionID( db );
        synchronized ( syncRes ) {
            syncRes.notifyAll();
        }
    }

    public String getTransactionID() throws InterruptedException {

        /*
         * if ( thread == null || this.transacationID != null ) { return
         * this.transacationID; }
         */

        final int getTransactionIDTimeOut = 60000;
        int time = 0;
        while ( this.transacationID == null ) {
            synchronized ( syncRes ) {
                if ( time < getTransactionIDTimeOut ) {
                    syncRes.wait( 10 );
                    time += 10;
                } else {
                    throw new BaseException( -999, "get transacationID Error" );
                }

            }
        }
        return transacationID;

    }

    // 返回结果集
    public List< Throwable > getExceptions() {
        join();
        return exceptionList;
    }

    public String getErrorMsg() {
        join();
        StringBuilder buffer = new StringBuilder();
        for ( Throwable exception : exceptionList ) {
            buffer.append( getErrorMsg( exception ) );
        }
        return buffer.toString();
    }

    private String getErrorMsg( Throwable e ) {
        if ( e == null )
            return "";
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        PrintStream printStream = new PrintStream( bytes );
        printStream.println();
        printStream.println( "------  err msg start: " );
        e.printStackTrace( printStream );
        printStream.println( "------  err msg end." );
        printStream.flush();
        return bytes.toString();
    }

    // join所有线程
    public void join() {
        synchronized ( this ) {
            try {
                if ( count.get() != 0 ) {
                    this.wait();
                }
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    public boolean isSuccess() {
        join();
        if ( exceptionList.size() != 0 ) {
            return false;
        }
        return true;
    }

    public void run() {
        try {
            if ( count.get() == 1 ) {
                thread = Thread.currentThread();
                synchronized ( syncRunning ) {
                    syncRunning.notifyAll();
                }
            }
            exec();
        } catch ( Throwable e ) {
            exceptionList.add( e );
        } finally {
            if ( 0 == count.decrementAndGet() ) {
                synchronized ( this ) {
                    this.notify();
                    thread = null;
                }
            }
        }
    }

    /*
     * -------------------------------------------------------------------------
     * - matchBlockingMethod -- 当前线程是否阻塞在相应的调用上 Parameters: className: 类名
     * (DBCollection.class.getName()) methodName: 方法名(query ...) Returns:
     * 如果当前线程执行CL.update()阻塞 matchBlockingMethod(cl.getClass().getName(),
     * "update")则返回true 如果当前线程执行CL.query()阻塞，则返回true
     * matchBlockingMethod(DBCursor.class.getName(), "query")则返回true 否则返回false
     * --------------------------------------------------
     * ------------------------
     */
    public boolean matchBlockingMethod( String className, String methodName ) {
        while ( thread == null ) {
            synchronized ( syncRunning ) {
                try {
                    syncRunning.wait( 100 );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }

            if ( 0 == count.get() ) {
                return false;
            }
        }

        final int fiftySeonds = 50000;
        final int totalTimes = 3;
        int matchTimes = 0;
        int alreadyWaitTime = 0;
        boolean ret = true;

        int pos = 0;
        do {
            Thread traceThread = null;
            synchronized ( this ) {
                traceThread = thread;
            }

            if ( traceThread == null ) {
                System.out.println( "Thread not start" );
                ret = false;
                break;
            }

            if ( alreadyWaitTime >= fiftySeonds ) {
                System.out.println( traceThread.getName() + " Timeout..." );
                System.out.println( traceThread.getName()
                        + Arrays.toString( traceThread.getStackTrace() ) );
                ret = false;
                break;
            }

            try {
                Thread.sleep( 5 );
                alreadyWaitTime += 5;
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }

            StackTraceElement[] stackElem = traceThread.getStackTrace();
            for ( pos = 0; pos < stackElem.length; ++pos ) {
                if ( stackElem[ pos ].getClassName().equals( className )
                        && stackElem[ pos ].getMethodName()
                                .equals( methodName ) ) {
                    ++matchTimes;
                    alreadyWaitTime -= 5;
                    break;
                }
            }

            if ( matchTimes >= totalTimes ) {
                break;
            }

        } while ( true );

        return ret;
    }

    public abstract void exec() throws Exception;

    public static void main( String[] args ) {
        Thread t = Thread.currentThread();
        Thread t1 = t;
        t = null;
        t1.getStackTrace();

        SdbThreadBase base = new SdbThreadBase() {

            @Override
            public void exec() throws Exception {
                // TODO Auto-generated method stub
                Thread.sleep( 5000 );
            }

        };

        base.start();
        // base.matchBlockingMethod( base.getClass().getName(), "exec" );

        try {
            Thread.sleep( 10000 );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        // base.matchBlockingMethod( base.getClass().getName(), "exec" );

    }
}
