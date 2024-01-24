package com.sequoiadb.testcommon;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

public abstract class SdbThreadBase implements Runnable {
    private List< Exception > exceptionList = Collections
            .synchronizedList( new ArrayList< Exception >() );
    private List< Thread > threadList = new ArrayList< Thread >();

    public void start() {
        start( 1 );
    }

    public void start( int threadNum ) {
        synchronized ( this ) {
            for ( int i = 0; i < threadNum; i++ ) {
                Thread t = new Thread( this );
                threadList.add( t );
                t.start();
            }
        }
    }

    // 返回结果集
    public List< Exception > getExceptions() {
        join();
        return exceptionList;
    }

    private String getErrorMsg( Throwable exception ) {
        if ( exception == null )
            return "";
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        PrintStream printStream = new PrintStream( bytes );
        printStream.println();
        printStream.println( "Thread name: " + Thread.currentThread().getName()
                + " err msg start:" );
        exception.printStackTrace( printStream );
        printStream.flush();
        return bytes.toString();
    }

    public String getErrorMsg() {
        join();
        StringBuilder reStr = new StringBuilder();
        for ( Exception exception : exceptionList ) {
            reStr.append( getErrorMsg( exception ) );
        }
        return reStr.toString();
    }

    // join所有线程
    public void join() {
        synchronized ( this ) {
            Iterator< Thread > iter = threadList.iterator();
            while ( iter.hasNext() ) {
                Thread t = ( Thread ) iter.next();
                try {
                    t.join();
                } catch ( InterruptedException e ) {
                    exceptionList.add( e );
                }
            }
            threadList.clear();
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
            exec();
        } catch ( Exception e ) {
            exceptionList.add( e );
        } catch ( Error e ) {
            exceptionList.add( new Exception( e ) );
        }
    }

    public abstract void exec() throws Exception;
}
