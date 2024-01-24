package com.sequoiadb.transaction.pendingobject;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName seqDB-20142:不同的集合中回滚事务产生相同的索引键值的pending object
 * @Author zhaoyu
 * @Date 2019年11月1日
 */
public class Transaction20142 extends SdbTestBase {

    private String clNameBase = "cl20142";
    private List< String > clNames = new ArrayList< String >();
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();
    private int clNum = 5;
    private int insertNum = 100;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( csName );
        for ( int i = 0; i < clNum; i++ ) {
            String clName = clNameBase + "_" + i;
            clNames.add( clName );
            cs.createCollection( clName );
        }
    }

    @Test
    public void testIdIndex() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        try {
            // 事务中插入删除记录
            TransUtils.beginTransaction( db );
            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection tcl = db.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                ArrayList< BSONObject > insertR1s = new ArrayList< BSONObject >();
                for ( int j = 0; j < insertNum; j++ ) {
                    insertR1s.add( ( BSONObject ) JSON.parse(
                            "{_id:" + j + ",a:" + j + ",b:" + j + "}" ) );
                }
                tcl.insert( insertR1s );
            }

            int recordNum = insertNum - 1;
            expDataList.clear();
            for ( int j = 0; j < recordNum; j++ ) {
                String record = "{_id:" + j + ",a:" + j + ",b:'insert20142'}";
                expDataList.add( ( BSONObject ) JSON.parse( record ) );
            }

            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection tcl = db.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                tcl.delete( "{a:{$lt:" + recordNum + "}}" );

                // 非事务中插入记录，唯一索引值与插入记录的值相同
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                cl.insert( expDataList );
            }

            db.rollback();

            // 校验结果
            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                TransUtils.queryAndCheck( cl, "{_id:1}", "{_id:''}",
                        expDataList );
            }

        } finally {
            db.commit();
            if ( db != null ) {
                db.close();
            }
            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                cl.delete( "" );
            }
        }
    }

    @Test
    public void testcommonUniqueIdx() {
        for ( int i = 0; i < clNames.size(); i++ ) {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clNames.get( i ) );
            cl.createIndex( "idx20142", "{a:1}", true, true );
        }
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        try {

            // 事务中插入删除记录
            TransUtils.beginTransaction( db );
            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection tcl = db.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                ArrayList< BSONObject > insertR1s = new ArrayList< BSONObject >();
                for ( int j = 0; j < insertNum; j++ ) {
                    insertR1s.add( ( BSONObject ) JSON
                            .parse( "{a:" + j + ",b:" + j + "}" ) );
                }
                tcl.insert( insertR1s );
            }

            int recordNum = insertNum - 1;
            expDataList.clear();
            for ( int j = 0; j < recordNum; j++ ) {
                String record = "{a:" + j + ",b:'insert20142'}";
                expDataList.add( ( BSONObject ) JSON.parse( record ) );
            }

            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection tcl = db.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                tcl.delete( "{a:{$lt:" + recordNum + "}}" );

                // 非事务中插入记录，唯一索引值与插入记录的值相同
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                cl.insert( expDataList );
            }

            db.rollback();

            // 校验结果
            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                TransUtils.queryAndCheck( cl, "{a:1}", "{a:''}", expDataList );
            }

        } finally {
            db.commit();
            if ( db != null ) {
                db.close();
            }
            for ( int i = 0; i < clNames.size(); i++ ) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clNames.get( i ) );
                cl.delete( "" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        for ( int i = 0; i < clNames.size(); i++ ) {
            cs.dropCollection( clNames.get( i ) );
        }

        sdb.close();
    }

}
