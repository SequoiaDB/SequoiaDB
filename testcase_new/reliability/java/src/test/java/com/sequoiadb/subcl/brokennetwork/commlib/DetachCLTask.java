package com.sequoiadb.subcl.brokennetwork.commlib;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.task.OperateTask;

public class DetachCLTask extends OperateTask {
    private int detachedSclCnt = 0;
    private String safeUrl = null;
    private String mclName = null;

    public DetachCLTask( String mclName, String safeUrl ) {
        this.mclName = mclName;
        this.safeUrl = safeUrl;
    }

    @Override
    public void exec() throws Exception {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( safeUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
            DBCollection mcl = cs.getCollection( mclName );
            for ( int i = 0; i < Utils.SCLNUM; i++ ) {
                String sclFullName = SdbTestBase.csName + "." + mclName + "_"
                        + i;
                mcl.detachCollection( sclFullName );
                detachedSclCnt++;
            }
        } catch ( BaseException e ) {
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    public int getDetachedSclCnt() {
        return detachedSclCnt;
    }
}
