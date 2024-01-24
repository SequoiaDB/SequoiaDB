package com.sequoiadb.faultmodule.fault;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.faultmodule.fault.Fault;
import com.sequoiadb.faultmodule.fault.FaultStage.Stage;

public abstract class FaultBase extends Fault {

    public FaultBase( String name ) {
        super( name );
        setStage( new FaultStage( Stage.init ) );
    }

    protected abstract void makeFault() throws Exception;

    @Override
    public void make() throws FaultException {
        setStage( new FaultStage( Stage.make ) );
        try {
            makeFault();
        } catch ( Exception e ) {
            getFaultStage().setStatus( -1 );
            throw new FaultException( e );
        }
    }

    protected abstract boolean checkMakeFault() throws Exception;

    @Override
    public boolean checkMake() throws FaultException {
        try {
            if ( checkMakeFault() ) {
                getFaultStage().setStatus( 1 );
                return true;
            } else {
                getFaultStage().setStatus( -1 );
                return false;
            }
        } catch ( Exception e ) {
            getFaultStage().setStatus( -1 );
            throw new FaultException( e );
        }
    }

    protected abstract void restoreFault() throws Exception;

    @Override
    public void restore() throws FaultException {
        setStage( new FaultStage( Stage.restore ) );
        try {
            restoreFault();
        } catch ( Exception e ) {
            getFaultStage().setStatus( -1 );
            throw new FaultException( e );
        }
    }

    protected abstract boolean checkRestoreFault() throws Exception;

    @Override
    public boolean checkRestore() throws FaultException {
        try {
            if ( checkRestoreFault() ) {
                getFaultStage().setStatus( 1 );
                return true;
            } else {
                getFaultStage().setStatus( -1 );
                return false;
            }
        } catch ( Exception e ) {
            getFaultStage().setStatus( -1 );
            throw new FaultException( e );
        }
    }

    protected abstract void initFault() throws Exception;

    @Override
    public void init() throws FaultException {
        try {
            initFault();
        } catch ( Exception e ) {
            getFaultStage().setStatus( -1 );
            throw new FaultException( e );
        }
        getFaultStage().setStatus( 1 );
    }

    protected abstract void finiFault() throws Exception;

    @Override
    public void fini() throws FaultException {
        setStage( new FaultStage( Stage.fini ) );
        try {
            finiFault();
        } catch ( Exception e ) {
            getFaultStage().setStatus( -1 );
            throw new FaultException( e );
        }
        getFaultStage().setStatus( 1 );
    }
}
