/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:FaultWrapper.java
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.fault;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.task.OperateTask;

import java.util.Calendar;
import java.util.Date;
import java.util.logging.Logger;

public class FaultWrapper extends Fault {
    private final static Logger log = Logger
            .getLogger( FaultWrapper.class.getName() );

    private Fault instance;
    private int checkTimes = 3;
    private OperateTask.FaultStatus status;

    private void handleException( FaultException e ) {
        status = OperateTask.FaultStatus.EXCEPTION;
    }

    public FaultWrapper( Fault instance ) {
        super( instance.getName() );
        this.instance = instance;
    }

    public FaultWrapper( Fault instance, int checkTimes ) {
        super( instance.getName() );
        this.instance = instance;
        this.checkTimes = checkTimes;
    }

    public void make() throws FaultException {
        log.info( "make " + instance.getName() );

        try {
            instance.make();
            if ( status != OperateTask.FaultStatus.EXCEPTION ) {
                if ( checkMakeResult() ) {
                    status = OperateTask.FaultStatus.MAKESUCCESS;
                    log.info( "make " + instance.getName() + " success" );
                } else {
                    status = OperateTask.FaultStatus.MAKEFAILURE;
                    log.info( "make " + instance.getName() + " failure " );
                }
            }
        } catch ( FaultException e ) {
            handleException( e );
            throw e;
        }
    }

    public boolean checkMakeResult() throws FaultException {
        boolean checkResult = false;
        status = OperateTask.FaultStatus.MAKEFAILURE;
        for ( int i = 0; i < checkTimes; ++i ) {
            try {
                checkResult = instance.checkMakeResult();
            } catch ( FaultException e ) {
                if ( i >= checkTimes - 1 ) {
                    throw e;
                }
            }
            if ( checkResult ) {
                status = OperateTask.FaultStatus.MAKESUCCESS;
                break;
            }
            try {
                Thread.sleep( 500 );
            } catch ( Exception e ) {
                // ignore
            }
        }
        return checkResult;
    }

    public void restore() throws FaultException {
        if ( status != OperateTask.FaultStatus.MAKESUCCESS ) {
            return;
        }

        Date date = Calendar.getInstance().getTime();
        log.info( "restore " + instance.getName() );
        int times = 0 ;
        do {
            try {
                instance.restore();
                if ( status != OperateTask.FaultStatus.EXCEPTION ) {
                    if ( checkRestoreResult() ) {
                        date = Calendar.getInstance().getTime();
                        log.info( "restore " + instance.getName() + " success." );
                        break ;
                    } else {
                        log.info( "restore " + instance.getName() + " failure." );
                        status = OperateTask.FaultStatus.RESTOREFAILURE;
                        try {
                            Thread.sleep( 1000 );
                        } catch ( InterruptedException e ) {
                            // TODO Auto-generated catch block
                            e.printStackTrace();
                        }
                        continue;
                    }
                }
            } catch ( FaultException e ) {
                handleException( e );
                throw e;
            }
        } while(times++ < checkTimes) ;
    }

    public boolean checkRestoreResult() throws FaultException {
        boolean checkResult = false;
        status = OperateTask.FaultStatus.RESTOREFAILURE;
        for ( int i = 0; i < checkTimes; ++i ) {
            checkResult = instance.checkRestoreResult();
            if ( checkResult ) {
                status = OperateTask.FaultStatus.RESTORESUCESS;
                break;
            }
        }
        return checkResult;
    }

    @Override
    public void init() throws FaultException {
        instance.init();
    }

    @Override
    public void fini() throws FaultException {
        instance.fini();
    }

}
