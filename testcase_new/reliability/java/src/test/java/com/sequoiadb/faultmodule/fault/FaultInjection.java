package com.sequoiadb.faultmodule.fault;

import java.util.logging.Logger;

import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;

public class FaultInjection extends FaultBase {

    protected String hostName;
    protected String svcName;
    protected Ssh ssh;
    protected int port = 22;
    protected String user;
    protected String passwd;
    protected String enableName;
    protected String probability;
    protected final static Logger log = Logger
            .getLogger( FaultInjection.class.getName() );

    public FaultInjection( String hostName, String svcName, String user,
            String passwd, String name ) {
        super( name );
        this.hostName = hostName;
        this.svcName = svcName;
        this.user = user;
        this.passwd = passwd;
        setFaultType();
    }

    private void setFaultType() {
        switch ( getName() ) {
        case FaultName.MEMORYLIMIT:
            enableName = "posix/mm/mmap"; // libc/mm/mmap will make node dump
            probability = "0.25";
            break;
        case FaultName.DISKLIMIT:
            enableName = "posix/io/rw/write";
            probability = "0.25";
        default:
            break;
        }
    }

    private String getAndCheckPid() throws ReliabilityException {
        ssh.exec( "lsof -nP -iTCP:" + svcName
                + " -sTCP:LISTEN | sed '1d' | awk '{print $2}'" );
        String pid = ssh.getStdout().trim();
        if ( pid.length() > 0 ) {
            return pid;
        } else {
            throw new FaultException( "Can not match the pid for " + svcName );
        }
    }

    private boolean checkOutput() throws FaultException {
        String output = ssh.getStdout().trim();
        if ( output.length() == 0 ) {
            return true;
        } else {
            throw new FaultException( output );
        }
    }

    @Override
    protected void makeFault() throws ReliabilityException {
        log.info( getName() + " make target node:[" + hostName + ":" + svcName
                + "]" );
        String pid = getAndCheckPid();
        ssh.exec( "fiu-ctrl -c 'enable_random name=" + enableName
                + ",probability=" + probability + "' " + pid );
    }

    @Override
    protected boolean checkMakeFault() throws FaultException {
        return checkOutput();
    }

    @Override
    protected void restoreFault() throws ReliabilityException {
        log.info( getName() + " restore target node:[" + hostName + ":"
                + svcName + "]" );
        String pid = getAndCheckPid();
        ssh.exec( "fiu-ctrl -c 'disable name=" + enableName + "' " + pid );
    }

    @Override
    protected boolean checkRestoreFault() throws FaultException {
        return checkOutput();
    }

    @Override
    protected void initFault() throws ReliabilityException {
        ssh = new Ssh( hostName, user, passwd, port );
    }

    @Override
    protected void finiFault() {
        ssh.disconnect();
    }
}
