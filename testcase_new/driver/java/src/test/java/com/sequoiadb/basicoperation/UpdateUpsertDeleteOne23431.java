package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description: seqDB-23431:updateOne/deleteOne/upsertOne接口验证
 * @author fanyu
 * @Date:2021年01月18日
 * @version:1.0
 */

public class UpdateUpsertDeleteOne23431 extends SdbTestBase {
    private boolean runSuccess = false;
    private String csName = "cs23431";
    private String clName = "cl23431";
    private int recordsNum = 5;
    private List< BSONObject > bsonObjectList = new ArrayList<>();
    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        // 创建cl
        cl = cs.createCollection( clName );
        // 准备记录
        for ( int i = 0; i < recordsNum; i++ ) {
            bsonObjectList
                    .add( new BasicBSONObject( "a", i ).append( "b", i ) );
        }
        cl.insert( bsonObjectList );
    }

    @Test
    private void test() {
        BSONObject matcher = new BasicBSONObject( "a",
                new BasicBSONObject( "$gte", 0 ) );
        BSONObject hint = new BasicBSONObject( "", "_id" );
        // 匹配多条记录，更新一条记录
        BSONObject modifier1 = new BasicBSONObject( "$set",
                new BasicBSONObject( "a", "update1" ).append( "b",
                        "update1" ) );
        cl.update( matcher, modifier1, hint, DBCollection.FLG_UPDATE_ONE );
        Assert.assertEquals( cl.getCount( new BasicBSONObject( "a", "update1" )
                .append( "b", "update1" ) ), 1 );

        BSONObject modifier2 = new BasicBSONObject( "$set",
                new BasicBSONObject( "a", "update2" ).append( "b",
                        "update2" ) );
        cl.update( matcher.toString(), modifier2.toString(), hint.toString(),
                DBCollection.FLG_UPDATE_ONE );
        Assert.assertEquals( cl.getCount( new BasicBSONObject( "a", "update2" )
                .append( "b", "update2" ) ), 1 );

        // 匹配多条记录，upsert一条记录
        BSONObject modifier3 = new BasicBSONObject( "$set",
                new BasicBSONObject( "a", "update3" ).append( "b",
                        "update3" ) );
        cl.upsert( matcher, modifier3, hint, new BasicBSONObject(),
                DBCollection.FLG_UPDATE_ONE );
        Assert.assertEquals( cl.getCount( new BasicBSONObject( "a", "update3" )
                .append( "b", "update3" ) ), 1 );

        // 匹配多条记录，删除一条记录
        cl.delete( matcher, hint, DBCollection.FLG_DELETE_ONE );
        Assert.assertEquals( cl.getCount(), recordsNum - 1 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
