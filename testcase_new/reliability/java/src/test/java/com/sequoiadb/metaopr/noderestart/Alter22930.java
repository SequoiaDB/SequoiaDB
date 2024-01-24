package com.sequoiadb.metaopr.noderestart;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

public class Alter22930 extends SdbTestBase {
    private String clName = "cl22930";
    private Sequoiadb db;
    private CollectionSpace cs;
    private DBCollection cl;
    private String groupName;
    private GroupMgr groupMgr = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName )
                        .append( "Compressed", true )
                        .append( "CompressionType", "lzw" ) );
    }

    @Test
    public void test() throws ReliabilityException {
        List< BSONObject > datas = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < 10000; j++ ) {
                BSONObject obj = new BasicBSONObject( "a", j * 10000 + i )
                        .append( "b", "abcdefghijklmnopqrstuvwxyz" + j )
                        .append( "c", "abcdefghijklmnopqrstuvwxyz" )
                        .append( "d", "abcdefghijklmnopqrstuvwxyz" );
                datas.add( obj );
            }
            cl.insert( datas );
            datas.clear();
        }
        chechCompressed();

        cl.alterCollection( new BasicBSONObject( "Compressed", false ) );

        checkInsert();

        db.getReplicaGroup( groupName ).stop();
        db.getReplicaGroup( groupName ).start();

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        checkInsert();

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void chechCompressed() throws ReliabilityException {
        Sequoiadb master = groupMgr.getGroupByName( groupName ).getMaster()
                .connect();
        DBCursor cur = master.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", cl.getFullName() ), null, null );
        if ( cur.hasNext() ) {
            BSONObject clInfo = cur.getNext();
            @SuppressWarnings("unchecked")
            ArrayList< BSONObject > details = ( ArrayList< BSONObject > ) clInfo
                    .get( "Details" );

            cur.close();
            Assert.assertTrue(
                    ( boolean ) details.get( 0 ).get( "DictionaryCreated" ) );
        } else {
            Assert.fail( "connect master get " + cl.getFullName()
                    + " info failed." );
        }
    }

    private void checkInsert() {
        DBCursor cur = cl
                .query( new BasicBSONObject( "a", 1 ),
                        new BasicBSONObject( "_id",
                                new BasicBSONObject( "$include", 0 ) ),
                        null, null );
        while ( cur.hasNext() ) {
            BSONObject expObj = new BasicBSONObject( "a", 1 )
                    .append( "b", "abcdefghijklmnopqrstuvwxyz0" )
                    .append( "c", "abcdefghijklmnopqrstuvwxyz" )
                    .append( "d", "abcdefghijklmnopqrstuvwxyz" );
            Assert.assertEquals( cur.getNext(), expObj );
        }
    }
}
