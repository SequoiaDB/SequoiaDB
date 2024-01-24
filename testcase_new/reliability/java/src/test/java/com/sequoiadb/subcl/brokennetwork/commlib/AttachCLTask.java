package com.sequoiadb.subcl.brokennetwork.commlib;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.task.OperateTask;
import org.bson.BSONObject;
import org.bson.util.JSON;

public class AttachCLTask extends OperateTask {
    private int attachedSclCnt = 0;
    private String safeUrl = null;
    private String mclName = null;

    public AttachCLTask( String mclName, String safeUrl ) {
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
            int rangeStart = 0;
            for ( int i = 0; i < Utils.SCLNUM; i++ ) {
                int rangeEnd = rangeStart + Utils.RANGE_WIDTH;
                String sclFullName = SdbTestBase.csName + "." + mclName + "_"
                        + i;
                mcl.attachCollection( sclFullName,
                        ( BSONObject ) JSON.parse( "{ LowBound: { a: "
                                + rangeStart + " }, " + "UpBound: { a: "
                                + rangeEnd + " } }" ) );
                rangeStart += Utils.RANGE_WIDTH;
                attachedSclCnt++;
            }
        } catch ( BaseException e ) {
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    public int getAttachedSclCnt() {
        return attachedSclCnt;
    }
}