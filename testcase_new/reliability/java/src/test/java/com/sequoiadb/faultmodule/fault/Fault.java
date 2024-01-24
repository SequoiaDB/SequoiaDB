package com.sequoiadb.faultmodule.fault;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.faultmodule.fault.FaultStage.Stage;

public abstract class Fault {

    private String name;

    private FaultStage stage;

    public Fault( String name ) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public Stage getStage() {
        return stage.get();
    }

    public int getStageStatus() {
        return stage.getStatus();
    }

    protected FaultStage getFaultStage() {
        return stage;
    }

    protected void setStage( FaultStage stage ) {
        this.stage = stage;
    }

    public abstract void make() throws FaultException;

    public abstract boolean checkMake() throws FaultException;

    public abstract void restore() throws FaultException;

    public abstract boolean checkRestore() throws FaultException;

    public abstract void init() throws FaultException;

    public abstract void fini() throws FaultException;

}
