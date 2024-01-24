package com.sequoiadb.faultmodule.msg;

import com.sequoiadb.faultmodule.fault.Fault;

public class FaultMsg {

    private Fault fault;
    private Exception makeExp;
    private boolean isProcessed;

    public FaultMsg( Fault fault ) {
        this.fault = fault;
        isProcessed = false;
    }

    public Fault getFault() {
        return fault;
    }

    public void setFault( Fault fault ) {
        this.fault = fault;
    }

    public Exception getMakeExp() {
        return makeExp;
    }

    public void setMakeExp( Exception makeExp ) {
        this.makeExp = makeExp;
    }

    public boolean isProcessed() {
        return isProcessed;
    }

    public void setProcessed( boolean isProcessed ) {
        this.isProcessed = isProcessed;
    }
}
