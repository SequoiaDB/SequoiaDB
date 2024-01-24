package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.options.UpsertOption;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-26318:upsert数据检查返回记录数
 * @author ZhangYanan
 * @createDate 2021.04.01
 * @updateUser ZhangYanan
 * @updateDate 2021.04.01
 * @updateRemark
 * @version v1.0
 */
public class TestUpsertResult26318 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clname = "cl_26318";
    private DBCollection cl = null;
    private CollectionSpace cs = null;
    private int insertNum = 100;

    @BeforeClass
    public void setSdb() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clname ) ) {
            cs.dropCollection( clname );
        }
        cl = cs.createCollection( clname );
        List< BSONObject > bulkInsertList = new ArrayList<>();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "no", i );
            obj.put( "a", i );
            bulkInsertList.add( obj );
        }
        cl.bulkInsert( bulkInsertList );
        cl.createIndex( "testIndex", "{no:1}", false, false );
    }

    @Test
    public void test() {
        UpsertOption option = new UpsertOption();

        BSONObject modifier = new BasicBSONObject();
        BSONObject mValue1 = new BasicBSONObject();
        mValue1.put( "b", 1 );
        modifier.put( "$set", mValue1 );

        BSONObject matcher = new BasicBSONObject();
        BSONObject mValue2 = new BasicBSONObject();
        int updateNum = 20;
        mValue2.put( "$gte", updateNum );
        matcher.put( "no", mValue2 );

        UpdateResult updateResult1 = cl.upsertRecords( matcher, modifier );
        Assert.assertEquals( updateResult1.getUpdatedNum(),
                insertNum - updateNum );
        Assert.assertEquals( updateResult1.getInsertNum(), 0 );
        Assert.assertEquals( updateResult1.getModifiedNum(),
                insertNum - updateNum );

        BSONObject modifier1 = new BasicBSONObject();
        BSONObject mValue3 = new BasicBSONObject();
        mValue3.put( "b", 0 );
        modifier1.put( "$set", mValue3 );

        BSONObject matcher1 = new BasicBSONObject();
        BSONObject mValue4 = new BasicBSONObject();
        mValue4.put( "$gt", updateNum );
        matcher1.put( "no", mValue4 );

        UpdateResult updateResult2 = cl.upsertRecords( matcher1, modifier1,
                ( UpsertOption ) option
                        .setFlag( UpsertOption.FLG_UPDATE_ONE ) );
        Assert.assertEquals( updateResult2.getUpdatedNum(), 1 );
        Assert.assertEquals( updateResult2.getInsertNum(), 0 );
        Assert.assertEquals( updateResult2.getModifiedNum(), 1 );
    }

    @AfterClass
    public void closeSdb() {
        try {
            if ( cs.isCollectionExist( clname ) ) {
                cs.dropCollection( clname );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}