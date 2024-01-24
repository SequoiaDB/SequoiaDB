package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: CreateMainCl23.java test content: 创建主表时指定主表的group testlink case:
 * seqDB-23
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl23 extends SdbTestBase {

    private Sequoiadb db = null;
    private String clName = "maincl_23";
    private CollectionSpace cs = null;
    private ArrayList< String > dataGroups = null;

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @Test
    public void testCreateMainclByAppointGroup() {
        dataGroups = SubCLUtils.getDataGroups( db );
        if ( dataGroups.size() == 0 ) {
            return;
        }
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collectionspace" );
        }
        // 通过指定数据组创建主表
        createMaincl();
        // 检测对应的快照信息
        checkSnapshot();
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        options.put( "Group", dataGroups.get( 0 ) );
        try {
            cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create maincl " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
    }

    public void checkSnapshot() {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + "." + clName );
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        try {
            cursor1 = db.getSnapshot( 4, matcher, null, null );
            cursor2 = db.getSnapshot( 8, matcher, null, null );
            BSONObject snapshotRes1 = null;
            BSONObject snapshotRes2 = null;
            BasicBSONList snapshotDetails = null;
            BasicBSONList snapshotCataInfo = null;
            BSONObject detail = null;
            BSONObject cataInfo = null;
            while ( cursor1.hasNext() ) {
                snapshotRes1 = cursor1.getNext();
                snapshotDetails = ( BasicBSONList ) snapshotRes1
                        .get( "Details" );
                if ( snapshotDetails != null && snapshotDetails.size() != 0 ) {
                    detail = ( BSONObject ) snapshotDetails.get( 0 );
                    if ( detail.get( "GroupName" ) != null ) {
                        Assert.fail( "snapshot4 is wrong" );
                    }
                }
            }
            while ( cursor2.hasNext() ) {
                snapshotRes2 = cursor2.getNext();
                snapshotCataInfo = ( BasicBSONList ) snapshotRes2
                        .get( "CataInfo" );
                if ( snapshotCataInfo != null
                        && snapshotCataInfo.size() != 0 ) {
                    cataInfo = ( BSONObject ) snapshotCataInfo.get( 0 );
                    if ( cataInfo.get( "GroupName" ) != null ) {
                        Assert.fail( "snapshot8 is wrong" );
                    }
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "failed to get snapshot :" + e.getMessage() );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
            if ( cursor2 != null ) {
                cursor2.close();
            }
        }

    }
}
