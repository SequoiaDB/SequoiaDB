package com.sequoiadb.faultmodule.task;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import com.sequoiadb.faultmodule.fault.Fault;
import com.sequoiadb.faultmodule.msg.FaultMsg;
import com.sequoiadb.faultmodule.th.FaultWorker;

public class FaultTask {

    private LinkedList< FaultMsg > faultMsgs;
    private Set< String > faultNames;
    private List< Exception > lExceptions;

    private FaultTask() {
        faultMsgs = new LinkedList<>();
        faultNames = new HashSet<>();
        lExceptions = new ArrayList<>();
    }

    public static FaultTask getFault( String... faultName ) {
        FaultTask task = new FaultTask();
        for ( String name : faultName ) {
            task.getFaultNames().add( name );
        }
        return task;
    }

    public void setFault( String... faultName ) {
        for ( String name : faultName ) {
            faultNames.add( name );
        }
    }

    public void removeFault( String... faultName ) {
        for ( String name : faultName ) {
            faultNames.remove( name );
        }
    }

    public Set< String > getFaultNames() {
        return faultNames;
    }

    @SuppressWarnings({ "unchecked", "rawtypes" })
    public void makeFault( String faultName, String hostName, String svcName,
            String user, String passwd ) throws Exception {
        Class< ? extends Fault > fault = null;
        try {
            fault = ( Class< ? extends Fault > ) Class.forName(
                    "com.sequoiadb.faultmodule.fault.impl." + faultName );
        } catch ( ClassNotFoundException e ) {
            fault = ( Class< ? extends Fault > ) Class.forName(
                    "com.sequoiadb.faultmodule.fault.FaultInjection" );
        }
        Constructor c = fault.getDeclaredConstructor( String.class,
                String.class, String.class, String.class, String.class );
        Fault f = ( Fault ) c.newInstance( hostName, svcName, user, passwd,
                faultName );
        FaultMsg m = new FaultMsg( f );
        FaultWorker.msgMQ.push( m );
        faultMsgs.push( m );
    }

    public void make( String hostName, String svcName, String user,
            String passwd ) throws Exception {
        for ( String name : faultNames ) {
            makeFault( name, hostName, svcName, user, passwd );
        }
        checkResult();
    }

    public void restore() throws Exception {
        for ( FaultMsg m : faultMsgs ) {
            m.setProcessed( false );
            FaultWorker.msgMQ.push( m );
        }
        checkResult();
    }

    private void checkResult() throws Exception {
        lExceptions.clear();
        for ( FaultMsg msg : faultMsgs ) {
            synchronized ( msg ) {
                while ( !msg.isProcessed() ) {
                    msg.wait();
                }
                if ( null != msg.getMakeExp() ) {
                    lExceptions.add( msg.getMakeExp() );
                }
            }
        }
        if ( !lExceptions.isEmpty() ) {
            throw new Exception( getErrorMsg() );
        }
    }

    private String getErrorMsg() {
        StringBuffer reStr = new StringBuffer();
        for ( Exception e : lExceptions ) {
            if ( e == null )
                return "";
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            PrintStream printStream = new PrintStream( bytes );
            printStream.println();
            printStream.println( "------Fault Make: "
                    + Thread.currentThread().getName() + " err msg start: " );
            e.printStackTrace( printStream );
            printStream.println( "------Fault Make: " + Thread.currentThread()
                    + " err msg end." );
            printStream.flush();
            reStr.append( bytes.toString() );
        }
        return reStr.toString();
    }
}
