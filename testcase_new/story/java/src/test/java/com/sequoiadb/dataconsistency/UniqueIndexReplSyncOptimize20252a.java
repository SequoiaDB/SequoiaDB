package com.sequoiadb.dataconsistency;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @testlink 更新重复键及_id字段，穿插执行元数据操作：创建删除索引
 * @author zhaoyu
 * @Date 2019.11.11
 */
public class UniqueIndexReplSyncOptimize20252a extends SdbTestBase {

    private String clName = "cl20252a";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private int loopNum = 10000;
    private String groupName;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }
        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        if ( DataConsistencyUtil.isOneNodeInGroup( sdb, groupName ) ) {
            throw new SkipException( "one node in group skip testcase" );
        }
        cs = sdb.getCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        dbcl.createIndex( "a", "{a:1}", true, true );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:2,a:1,order:1}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:3,a:2,order:2}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:4,a:4,order:3}" ) );
        dbcl.insert( insertRecords );
    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < loopNum; i++ ) {
            dbcl.update( "{_id:2}", "{$set:{a:15}}", null );
            dbcl.update( "{_id:2}", "{$set:{a:1,b:" + i + "}}", null );
            dbcl.update( "{_id:4}", "{$set:{_id:5,a:5,b:" + i + "}}", null );
            dbcl.update( "{_id:3}", "{$set:{a:15}}", null );
            dbcl.update( "{_id:3}", "{$set:{a:2,b:" + i + "}}", null );
            dbcl.update( "{_id:5}", "{$set:{_id:4,a:4,b:" + i + "}}", null );
            // analyze会写日志，但是这个日志不会并发重放，验证并发重放转成非并发重放的正确性
            if ( 0 == i % 1000 ) {
                BSONObject analyzeOtions = ( BSONObject ) JSON.parse(
                        "{Collection: '" + csName + "." + clName + "' }" );
                sdb.analyze( analyzeOtions );
                dbcl.dropIdIndex();
                dbcl.createIdIndex( null );
                dbcl.dropIndex( "a" );
                dbcl.createIndex( "a", "{a:1}", true, true );
            }
        }

        int bValue = loopNum - 1;
        for ( BSONObject doc : insertRecords ) {
            doc.put( "b", bValue );
        }

        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName,
                insertRecords, "" );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

}
