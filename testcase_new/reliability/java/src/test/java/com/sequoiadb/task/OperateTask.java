/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:OperateTask.java
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import com.sequoiadb.exception.ReliabilityException;

import java.util.logging.Logger;

public abstract class OperateTask extends Task {
    private final static Logger log = Logger
            .getLogger( OperateTask.class.getName() );

    public enum FaultStatus {
        INIT, MAKESUCCESS, MAKEFAILURE, RESTORESUCESS, RESTOREFAILURE, EXCEPTION
    }

    private TaskMgr mgr = null;

    public OperateTask( String name ) {
        super( name );
    }

    public OperateTask() {
        this.setName( this.getClass().getSimpleName() );
    }

    public void setMgr( TaskMgr mgr ) {
        this.mgr = mgr;
    }

    public abstract void exec() throws Exception;

    public void run() {
        setStatus( Task.TaskStatus.TASKSTART );
        log.info( "Thread '" + this.getName() + "' run " );
        try {
            exec();
        } catch ( Error | Exception e ) {
            if ( e instanceof InterruptedException ) {
                setStatus( Task.TaskStatus.TASKINTERRUPT );
            } else {
                setStatus( Task.TaskStatus.TASKTHROWEXCEPTION );
                Exception e1 = new ReliabilityException( e );
                e1.setStackTrace( e.getStackTrace() );
                setException( e1 );
            }
        }
        if ( getException() == null ) {
            setStatus( Task.TaskStatus.TASKSTOP );
        }
        log.info( "Thread '" + this.getName() + "' end" );
    }

    public Task getTaskByName( String name ) {
        return mgr.getTaskByName( name );
    }

    @Deprecated
    @Override
    public void fini() throws ReliabilityException {
    }

    @Deprecated
    @Override
    public void init() throws ReliabilityException {
    }

    @Override
    public void check() throws ReliabilityException {
    }
}
