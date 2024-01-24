/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:FaultMakeTask.java
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import java.util.logging.Logger;
import java.util.Random;

import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.Fault;
import com.sequoiadb.fault.FaultWrapper;

public class FaultMakeTask extends Task {
    @SuppressWarnings("unused")
    private final static Logger log = Logger
            .getLogger( FaultMakeTask.class.getName() );

    public static final String MAKE_RESULT = "MakeResult";
    public static final String RESTORE_RESULT = "RestoreResult";

    private FaultWrapper faultInstance;
    private int duration;
    private int _randomStartMaxDuration = 0;
    private volatile boolean isMakeSuccess = false;

    public FaultMakeTask( Fault instance, int maxDlay, int duration,
            int checkTimes ) {
        super( instance.getName() );
        faultInstance = new FaultWrapper( instance, checkTimes );
        this.duration = duration;
        this._randomStartMaxDuration = maxDlay;
    }

    @SuppressWarnings("static-access")
    @Override
    public void run() {
        Random random = new Random();
        try {
            if ( _randomStartMaxDuration <= 0 ) {
                Thread.currentThread().sleep( 100 );
            } else {
                Thread.currentThread().sleep(
                        random.nextInt( _randomStartMaxDuration * 1000 - 100 )
                                + 100 );
            }
        } catch ( Exception e ) {
        }
        try {
            faultInstance.make();
            isMakeSuccess = true;
        } catch ( ReliabilityException e ) {
            setException( e );
        }
        try {
            Thread.currentThread().sleep( duration * 1000 );
        } catch ( Exception e ) {
        }
        try {
            faultInstance.restore();
        } catch ( ReliabilityException e ) {
            setException( e );
        }

    }

    public boolean isMakeSuccess() {
        return isMakeSuccess;
    }

    @Override
    public void init() throws ReliabilityException {
        faultInstance.init();
    }

    @Override
    public void check() throws ReliabilityException {
    }

    @Override
    public void fini() throws ReliabilityException {
        faultInstance.fini();
    }

    public Fault getFaultInstance() {
        return faultInstance;
    }
}
