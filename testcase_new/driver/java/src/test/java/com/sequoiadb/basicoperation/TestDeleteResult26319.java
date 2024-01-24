package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.options.DeleteOption;
import com.sequoiadb.base.result.DeleteResult;
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
 * @description seqDB-26319:delete数据检查返回记录数
 * @author ZhangYanan
 * @createDate 2021.04.01
 * @updateUser ZhangYanan
 * @updateDate 2021.04.01
 * @updateRemark
 * @version v1.0
 */
public class TestDeleteResult26319 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clname = "cl_26319";
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
        DeleteOption option = new DeleteOption();

        BSONObject matcher = new BasicBSONObject();
        BSONObject mValue2 = new BasicBSONObject();
        int deleteNum = 20;
        mValue2.put( "$gte", deleteNum );
        matcher.put( "no", mValue2 );

        DeleteResult deleteResult2 = cl.deleteRecords( matcher,
                option.setFlag( DeleteOption.FLG_DELETE_ONE ) );
        Assert.assertEquals( deleteResult2.getDeletedNum(), 1 );

        DeleteResult deleteResult1 = cl.deleteRecords( matcher );
        Assert.assertEquals( deleteResult1.getDeletedNum(),
                insertNum - deleteNum - 1 );
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