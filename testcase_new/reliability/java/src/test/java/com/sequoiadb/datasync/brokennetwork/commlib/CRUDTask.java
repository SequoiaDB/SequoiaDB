package com.sequoiadb.datasync.brokennetwork.commlib;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.task.OperateTask;
import org.bson.BSONObject;
import org.bson.util.JSON;

public class CRUDTask extends OperateTask {
    private String safeUrl = null;
    private String clName = null;

    public CRUDTask( String safeUrl, String clName ) {
        this.safeUrl = safeUrl;
        this.clName = clName;
    }

    @Override
    public void exec() throws Exception {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( safeUrl, "", "" );
            DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            int repeatTimes = 5000;
            for ( int i = 0; i < repeatTimes; i++ ) {
                BSONObject rec = ( BSONObject ) JSON
                        .parse( "{ a: " + i + " }" );
                cl.insert( rec );
                BSONObject modifier = ( BSONObject ) JSON
                        .parse( "{ $set: { b: 1 } }" );
                cl.update( rec, modifier, null );
                cl.delete( rec );
            }
        } catch ( BaseException e ) {
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }
}
