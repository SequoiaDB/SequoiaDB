package com.sequoiadb.sdb;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Util {
    public static boolean isCluster( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            int errno = e.getErrorCode();
            if ( new BaseException( "SDB_RTN_COORD_ONLY" )
                    .getErrorCode() == errno ) {
                System.out.println(
                        "This test is for cluster environment only." );
                return false;
            }
        }
        return true;
    }
}
