package com.sequoiadb.lob;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.OperateTask;
import org.bson.BasicBSONObject;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public abstract class ClTask extends OperateTask {
    /**
     * 在
     *
     * @param num
     * @return
     */
    public static ClTask getClTask( final int num, final String csName,
            final String clName ) {
        return new ClTask() {
            @Override
            public void exec() throws Exception {
                setName( "data opration task thread " );
                try ( Sequoiadb db = MyUtil.getSdb()) {
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    for ( int i = 0; i < num; i++ ) {
                        cl.insert( new BasicBSONObject( "a", i ) );
                        cl.delete( new BasicBSONObject() );
                    }
                    for ( int i = 0; i < num; i++ ) {
                        cl.insert( new BasicBSONObject( "a", i ) );
                    }
                    db.commit();
                }
            }
        };
    }
}
