package com.sequoiadb.lob.serial;

import java.util.ArrayList;
import java.util.List;

import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-26713:开启maxcachesize，偏移写lob数据
 * @Author liuli
 * @Date 2022.07.07
 * @UpdateAuthor liuli
 * @UpdateDate 2022.07.07
 * @version 1.10
 */
public class TestLob26713 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl_26713";
    private DBCollection cl = null;
    private int lobSize = 1024 * 1024;
    private int writeSize = 1024 * 700;
    private int lobNum = 30;
    private byte[] lobBuff;
    private List< ObjectId > ids = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        for ( int i = 0; i < lobNum; i++ ) {
            ObjectId id = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );
            ids.add( id );
        }
    }

    @Test
    public void testLob() {
        sdb.updateConfig( new BasicBSONObject( "maxcachesize", 128 ) );

        byte[] writeBuff = LobOprUtils.getRandomBytes( writeSize );
        for ( int i = 0; i < lobNum; i++ ) {
            DBLob lob = cl.openLob( ids.get( i ),
                    DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
            lob.lockAndSeek( 1024 * 100, writeSize );
            lob.write( writeBuff );
            lob.close();
        }
        lobBuff = RandomWriteLobUtil.appendBuff( lobBuff, writeBuff,
                1024 * 100 );

        for ( ObjectId id : ids ) {
            RandomWriteLobUtil.checkShareLobResult( cl, id, lobSize, lobBuff );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.deleteConfig( new BasicBSONObject( "maxcachesize", 1 ),
                    new BasicBSONObject() );
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
