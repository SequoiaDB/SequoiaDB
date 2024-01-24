package com.sequoiadb.index;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.task.OperateTask;

import java.util.List;
import java.util.logging.Logger;

import static com.sequoiadb.metaopr.commons.MyUtil.*;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-8
 * @Version 1.00
 */
public abstract class IndexTask extends OperateTask {
    public IndexTask( String threadName ) {
        super( threadName );
    }

    Sequoiadb db = null;
    private String coorUrl = null;

    private final static Logger log = Logger
            .getLogger( IndexTask.class.getName() );

    public void setCoorUrl( String coorUrl ) {
        this.coorUrl = coorUrl;
    }

    public String getCoorUrl() {
        return coorUrl;
    }

    @Override
    public void exec() throws Exception {
        if ( coorUrl == null )
            db = getSdb();
        else
            db = new Sequoiadb( coorUrl, "", "" );
        operate();
        db.close();
    }

    abstract void operate();

    public static IndexTask getCreateIndexTask( final String csName,
            final String clName, final List< IndexBean > indexBeanList ) {
        return new IndexTask( "create index task thread" ) {
            @Override
            void operate() {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( IndexBean indexBean : indexBeanList ) {
                    createIndex( cl, indexBean );
                }
            }
        };
    }

    public static IndexTask getRemoveIndexTask( final String csName,
            final String clName, final List< IndexBean > indexBeanList ) {
        return new IndexTask( "deleted index task thread" ) {
            @Override
            void operate() {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( IndexBean indexBean : indexBeanList ) {
                    removeIndex( cl, indexBean );
                }
            }
        };
    }
}
