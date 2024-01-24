package com.sequoiadb.crud.compress.snappy;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6635: truncate删除记录 1、CL压缩类型为snappy，truncate删除记录 2、检查返回结果
 *                       3、再次往CL插入相同记录，检查插入结果和压缩结果
 * @Author linsuqiang
 * @Date 2016-12-20
 * @Version 1.00
 */
public class TestSnappy6635 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6635";
    private String dataGroupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( SnappyUilts.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        try {
            DBCollection cl = createCL();
            SnappyUilts.insertData( cl, 1000 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @SuppressWarnings("deprecation")
    @Test
    public void test() {
        Sequoiadb db = null;
        DBCollection cl = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            // do truncate
            cl.truncate();
            // check truncated
            checkTruncated( cl );
            // insert the same records
            SnappyUilts.insertData( cl, 1000 );
            // check compressed
            SnappyUilts.checkCompressed( cl, dataGroupName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private DBCollection createCL() {
        DBCollection cl = null;
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        BSONObject option = new BasicBSONObject();
        try {
            dataGroupName = SnappyUilts.getDataGroups( sdb ).get( 0 );
            option.put( "Group", dataGroupName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "snappy" );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    private void checkTruncated( DBCollection cl ) {
        if ( ( int ) cl.getCount() != 0 ) {
            Assert.fail( "fail to truncate!" );
        }
    }
}