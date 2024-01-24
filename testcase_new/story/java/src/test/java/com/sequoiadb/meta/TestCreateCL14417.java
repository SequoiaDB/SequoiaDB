package com.sequoiadb.meta;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.truncate.TruncateUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-14417:在CS不指定域下，创建自动切分表CL 1.创建CS不指定域,创建自动切分表
 * @Author fanyu
 * @Date 2018-02-05
 * @Version 1.00
 */
public class TestCreateCL14417 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs_14417";
    private String clName = "cl_14417";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( TruncateUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone, skip testcase" );
        }
        if ( TruncateUtils.getDataGroups( sdb ).size() < 3 ) {
            throw new SkipException( "less then 3 groups, skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        try {
            sdb.createCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( csName );
            BSONObject option = new BasicBSONObject();
            option.put( "ShardingKey", ( BSONObject ) JSON.parse( "{a:1}" ) );
            option.put( "ShardingType", "hash" );
            option.put( "AutoSplit", true );
            // create a AutoSplit CL with CS is not specied domain
            cs.createCollection( clName, option );
        } finally {
            db.close();
        }
    }
}
