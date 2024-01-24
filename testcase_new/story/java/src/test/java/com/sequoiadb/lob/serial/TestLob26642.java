package com.sequoiadb.lob.serial;

import com.sequoiadb.lob.utils.LobOprUtils;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-26642:开启maxcachesize，追加读写lob数据
 * @Author liuli
 * @Date 2022.07.06
 * @UpdateAuthor liuli
 * @UpdateDate 2022.07.06
 * @version 1.10
 */
public class TestLob26642 extends SdbTestBase {
    private Sequoiadb db = null;
    private DBCollection dbcl = null;
    private CollectionSpace cs = null;
    private String clName = "cl_26642";
    private int lobSize = 200;
    private int writeSize = 20;
    private byte[] lobBuff;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        dbcl = cs.createCollection( clName );
    }

    @Test
    public void testLob26642() {
        db.updateConfig( new BasicBSONObject( "maxcachesize", 128 ) );

        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        DBLob lob = dbcl.openLob( id, DBLob.SDB_LOB_WRITE );
        lob.seek( 0, DBLob.SDB_LOB_SEEK_END );
        byte[] writeBuff = LobOprUtils.getRandomBytes( writeSize );
        lob.write( writeBuff );
        lob.close();
        lobBuff = RandomWriteLobUtil.appendBuff( lobBuff, writeBuff, lobSize );

        RandomWriteLobUtil.checkShareLobResult( dbcl, id, lobSize + writeSize,
                lobBuff );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            db.deleteConfig( new BasicBSONObject( "maxcachesize", 1 ),
                    new BasicBSONObject() );
            if ( db != null ) {
                db.close();
            }
        }
    }
}
