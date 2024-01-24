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
 * @testlink seqDB-22094:存在多个唯一索引，插入的记录产生duplicated key错误的同时，更新配置项maxreplsync
 * @author zhaoyu
 * @Date 2020.04.21
 */
public class UniqueIndexReplSyncOptimize22094 extends SdbTestBase {

    private String clName = "cl22094";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private int loopNum = 100000;
    private String groupName;
    private ArrayList< BSONObject > expRecords = new ArrayList<>();
    private BSONObject record;

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
        dbcl.createIndex( "a22094", "{a:1}", true, true );
        record = ( BSONObject ) JSON.parse( "{_id:1,a:1,c:1,order:1}" );
        expRecords.add( record );
        record = ( BSONObject ) JSON
                .parse( "{_id:1000,a:1000,c:'insertRecord3000',order:1000}" );
        expRecords.add( record );
        record = ( BSONObject ) JSON
                .parse( "{_id:3000,a:3000,c:'insertRecord3000',order:3000}" );
        expRecords.add( record );
        dbcl.insert( expRecords );
    }

    @Test
    public void test() throws Exception {
        BSONObject record1 = ( BSONObject ) JSON.parse(
                "{_id:1,a:1, c:'12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890',order:1}" );
        BSONObject record2 = ( BSONObject ) JSON.parse(
                "{_id:1000,a:1000, c:'12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890',order:1000}" );
        BSONObject record3 = ( BSONObject ) JSON.parse(
                "{_id:3000,a:3000, c:'12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890',order:3000}" );

        for ( int i = 0; i < loopNum; i++ ) {
            dbcl.delete( "{a:1}" );
            dbcl.insert( record1 );
            dbcl.delete( "{a:1000}" );
            dbcl.insert( record2 );
            dbcl.delete( "{a:3000}" );
            dbcl.insert( record3 );
            if ( i % 1000 == 0 ) {
                if ( i % 2000 == 0 ) {
                    System.out.println( "testcase: " + this.getClass().getName()
                            + " update maxreplsync set 200" );
                    sdb.updateConfig(
                            ( BSONObject ) JSON.parse( "{maxreplsync:200}" ) );
                } else {
                    System.out.println( "testcase: " + this.getClass().getName()
                            + " update maxreplsync set default" );
                    sdb.updateConfig(
                            ( BSONObject ) JSON.parse( "{maxreplsync:10}" ) );
                }

            }

        }
        expRecords.clear();
        expRecords.add( record1 );
        expRecords.add( record2 );
        expRecords.add( record3 );
        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName,
                expRecords, "" );

    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.deleteConfig( ( BSONObject ) JSON.parse( "{maxreplsync:1}" ),
                    ( BSONObject ) JSON.parse( "{Global:true}" ) );
            cs.dropCollection( clName );
        } finally {

            sdb.close();
        }
    }
}
