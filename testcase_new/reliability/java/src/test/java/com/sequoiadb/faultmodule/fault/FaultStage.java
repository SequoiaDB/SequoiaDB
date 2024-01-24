package com.sequoiadb.faultmodule.fault;

public class FaultStage {

    public enum Stage {
        init, make, restore, fini;
    }

    private Stage stage;
    private int status;

    public FaultStage( Stage stage ) {
        this.stage = stage;
        status = 0;
    }

    public Stage get() {
        return stage;
    }

    public int getStatus() {
        return status;
    }

    void setStatus( int status ) {
        this.status = status;
    }
}
