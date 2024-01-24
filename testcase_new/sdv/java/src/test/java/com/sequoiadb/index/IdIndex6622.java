package com.sequoiadb.index;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-6622:查询数据过程中删除$id索引
 * @Author WenHua Huang 2016.12.14
 */

public class IdIndex6622 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_6622";
    private int recsNum = 20000;
    private ArrayList< BSONObject > insertor = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        // create cl
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options = ( BSONObject ) JSON.parse(
                "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024}" );
        cl = cs.createCollection( clName, options );

        // insert records
        for ( int i = 0; i < recsNum; i++ ) {
            insertor.add( new BasicBSONObject( "a", i ) );
        }
        cl.insert( insertor, 0 );
    }

    @Test
    public void queryData() {
        DropIdIndex dropIdIndex = new DropIdIndex();
        QueryData queryData = new QueryData();
        queryData.start();
        dropIdIndex.start();
        Assert.assertTrue( ( dropIdIndex.isSuccess() && queryData.isSuccess() ),
                dropIdIndex.getErrorMsg() + queryData.getErrorMsg() );
    }

    class DropIdIndex extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIdIndex();
                // explain查看是否走索引，判断索引是否删除成功
                DBCursor cursor = cl.explain( null, null, null,
                        ( BSONObject ) JSON.parse( "{'':'$id'}" ), 0, -1, 0,
                        null );
                String scanType = null;
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    if ( record.get( "Name" )
                            .equals( SdbTestBase.csName + "." + clName ) ) {
                        scanType = ( String ) record.get( "ScanType" );
                    }
                }
                Assert.assertEquals( scanType, "tbscan" );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    class QueryData extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // 查询一定要遍历
                DBCursor cursor = cl.query( null, "{'_id':{'$include':0}}",
                        "{'a':1}", "{'':'$id'}" );
                int num = 0;
                while ( cursor.hasNext() ) {
                    Assert.assertEquals( cursor.getNext(),
                            new BasicBSONObject( "a", num ) );
                    num++;
                }
                Assert.assertEquals( recsNum, num );
            } catch ( BaseException e ) {
                // -10: SEQUOIADBMAINSTREAM-5796
                if ( e.getErrorCode() != -48 && e.getErrorCode() != -10 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

}
