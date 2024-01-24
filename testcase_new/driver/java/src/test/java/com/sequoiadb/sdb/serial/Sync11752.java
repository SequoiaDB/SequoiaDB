package com.sequoiadb.sdb.serial;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-11752:数据持久化
 * @author laojingtang 2017/07/24
 */

public class Sync11752 extends SdbTestBase {
    private Sequoiadb db = null;
    private String csName = "cs11752";
    private String clName = "cl11752";

    @BeforeClass
    public void setup() {
        BSONObject option = new BasicBSONObject();
        option.put( "ReplSize", 0 );
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl = db.createCollectionSpace( csName )
                .createCollection( clName, option );
        cl.insert( new BasicBSONObject( "a", 1 ) );
    }

    @Test
    public void test() {
        // normal test
        BSONObject option = new BasicBSONObject( "Block", true )
                .append( "CollectionSpace", csName ).append( "Role", "all" );
        int[] num = { 1, 0, -1 };
        for ( int i : num ) {
            option.put( "Deep", i );
            db.sync( option );
        }

        option.put( "Block", false );
        db.sync( option );

        // 不带参数
        try {
            db.sync();
        } catch ( BaseException e ) {
            // 备节点数据未同步可能某个cs在备节点不存在，导致可能报-264，属于正常报错
            if ( e.getErrorCode() != -264 ) {
                throw e;
            }
        }

        // options为 null，同上，可能报-264，属于正常报错
        try {
            db.sync( null );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -264 ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void teardown() {
        try {
            db.dropCollectionSpace( csName );
        } finally {
            db.close();
        }
    }

}
