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
 * FileName: CreateMainCl21.java test content: 创建主表的CS指定domain testlink case:
 * seqDB-21
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl21 extends SdbTestBase {

    private String csName = "maincs_21";
    private String clName = "maincl_21";
    private String domainName = "domain_21";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SubCLUtils.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -34, e.getMessage() );
        } finally {
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @Test
    public void testCreateCsByAppointDomain() {
        ArrayList< String > dataGroup = SubCLUtils.getDataGroups( sdb );
        // 创建domain
        createDomain( dataGroup );
        // 创建cs在domain中
        BSONObject csOptions = new BasicBSONObject();
        csOptions.put( "Domain", domainName );
        try {
            cs = sdb.createCollectionSpace( csName, csOptions );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to create cs  " + "ErrorMsg:\n" + e.getMessage() );
        }
        // 创建maincl在cs中
        createMaincl();
        // 检验快照信息
        checkSnapshot();
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        options.put( "ShardingType", "range" );
        try {
            cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( "Failed to create maincl_21  " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
    }

    public void createDomain( ArrayList< String > dataGroup ) {
        BSONObject domainBson = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        groups.put( "0", dataGroup.get( 0 ) );
        groups.put( "1", dataGroup.get( 1 ) );
        domainBson.put( "Groups", groups );
        domainBson.put( "AutoSplit", true );
        try {
            sdb.createDomain( domainName, domainBson );
        } catch ( BaseException e ) {
            Assert.fail( "Failed to create domain " + "ErrorMsg:\n"
                    + e.getMessage() );
        }

    }

    public void checkSnapshot() {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + "." + clName );
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        try {
            cursor1 = sdb.getSnapshot( 4, matcher, null, null );
            cursor2 = sdb.getSnapshot( 8, matcher, null, null );
            BSONObject snapshotRes1 = null;
            BSONObject snapshotRes2 = null;
            BasicBSONList snapshotDetails = null;
            BasicBSONList snapshotCataInfo = null;
            BSONObject detail = null;
            BSONObject cataInfo = null;
            while ( cursor1.hasNext() ) {
                snapshotRes1 = cursor1.getNext();
                if ( snapshotRes1 != null ) {
                    snapshotDetails = ( BasicBSONList ) snapshotRes1
                            .get( "Details" );
                    if ( snapshotDetails != null
                            && snapshotDetails.size() != 0 ) {
                        detail = ( BSONObject ) snapshotDetails.get( 0 );
                        if ( detail.get( "GroupName" ) != null ) {
                            Assert.fail( "snapshot4 is wrong" );
                        }
                    }
                }
            }
            while ( cursor2.hasNext() ) {
                snapshotRes2 = cursor2.getNext();
                if ( snapshotRes2 != null ) {
                    snapshotCataInfo = ( BasicBSONList ) snapshotRes2
                            .get( "CataInfo" );
                    if ( snapshotCataInfo != null
                            && snapshotCataInfo.size() != 0 ) {
                        cataInfo = ( BSONObject ) snapshotCataInfo.get( 0 );
                        if ( cataInfo.get( "GroupName" ) != null ) {
                            Assert.fail( "snapshot4 is wrong" );
                        }
                    }
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
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
