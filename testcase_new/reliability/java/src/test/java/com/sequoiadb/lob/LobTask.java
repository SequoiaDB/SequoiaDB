package com.sequoiadb.lob;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.OperateTask;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.List;
import java.util.Map;

import static com.sequoiadb.metaopr.commons.MyUtil.getSdb;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public abstract class LobTask extends OperateTask {

    String csName = null, clName = null;
    DBCollection collection = null;
    Sequoiadb db = null;
    String hostName = null;

    public LobTask setHostName( String hostName ) {
        this.hostName = hostName;
        return this;
    }

    /**
     * 优先从主获取数据:db.setSessionAttr({PreferedInstance:"M"})
     */
    private BSONObject setSessionAttrOption = null;

    public LobTask setSessionAttr( BSONObject option ) {
        setSessionAttrOption = new BasicBSONObject( option.toMap() );
        return this;
    }

    public BSONObject getSetSessionAttrOption() {
        return setSessionAttrOption;
    }

    @Override
    public void exec() {
        if ( hostName != null )
            db = new Sequoiadb( hostName, "", "" );
        else
            db = getSdb();

        if ( setSessionAttrOption != null )
            db.setSessionAttr( setSessionAttrOption );

        if ( csName == null || clName == null )
            throw new IllegalArgumentException( "cs or cl can not be null" );
        collection = db.getCollectionSpace( csName ).getCollection( clName );
        lobOperate();

        db.close();
    }

    public LobTask setCsAndClName( String csName, String clName ) {
        this.clName = clName;
        this.csName = csName;
        return this;
    }

    abstract void lobOperate();

    public static LobTask getCreateLobsTask( final List< LobBean > lobs ) {
        return new LobTask() {
            @Override
            void lobOperate() {
                if ( lobs == null )
                    throw new IllegalArgumentException(
                            "lobs can not be null" );

                for ( LobBean lob : lobs ) {
                    DBLob dbLob = collection.createLob();
                    dbLob.write( lob.getContent() );
                    lob.setId( dbLob.getID() );
                    dbLob.close();
                }
            }
        };
    }

    public static LobTask getDeleteLobsTask( final List< LobBean > lobs ) {
        return new LobTask() {
            @Override
            void lobOperate() {
                for ( LobBean lob : lobs ) {
                    collection.removeLob( lob.getId() );
                    lob.setInSdb( false );
                }
            }
        };
    }

    public static LobTask getReadLobsTask( final List< LobBean > lobs ) {
        return new LobTask() {
            @Override
            void lobOperate() {
                for ( LobBean lob : lobs ) {
                    MyUtil.readLob( collection, lob.getId() );
                }
            }
        };
    }
}
