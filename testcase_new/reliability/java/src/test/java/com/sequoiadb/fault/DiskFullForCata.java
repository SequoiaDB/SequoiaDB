package com.sequoiadb.fault;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.exception.FaultException;

public class DiskFullForCata extends DiskFull {
    private DBCollection sysCataCL = null;

    public DiskFullForCata( String hostName, String padPath, int presetPercent,
            DBCollection sysCataCL ) {
        super( hostName, padPath, presetPercent );
        this.sysCataCL = sysCataCL;
    }

    @Override
    public void init() throws FaultException {
        // fill up almost space of disk
        super.init();
        super.make();
        // fill up almost space of sysCataCL
        CommLib.fillUpCL( sysCataCL, 512 );
    }

    @Override
    public void make() {
        System.out.println( "target cl: " + sysCataCL.getName() );
        CommLib.fillUpCL( sysCataCL, 128 );
    }

    @Override
    public void restore() throws FaultException {
        super.restore();
        sysCataCL.delete( "{deleteFlag:1}" );
    }
}
