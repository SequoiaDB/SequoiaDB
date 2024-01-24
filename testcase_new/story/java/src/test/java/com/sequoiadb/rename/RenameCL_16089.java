package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCL_16089.java 并发执行split切分和修改cl名
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16089 extends SdbTestBase {

    private String clName = "rename_CL_16089";
    private String newCLName = "rename_CL_16089_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private int recordNum = 10000;
    private String sourceGroup = null;
    private String targetGroup = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > rgNames = CommLib.getDataGroupNames( sdb );
        if ( rgNames.size() <= 1 ) {
            throw new SkipException(
                    "current environment less than tow groups" );
        }
        sourceGroup = rgNames.get( 0 );
        targetGroup = rgNames.get( 1 );
        cs = sdb.getCollectionSpace( csName );
        cl = createShardingCL();
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        SplitThread splitThread = new SplitThread();

        renameCLThread.start();
        splitThread.start();

        boolean rename = renameCLThread.isSuccess();
        boolean split = splitThread.isSuccess();

        if ( !rename ) {
            Integer[] errnosA = { -147, -334, -190 };
            BaseException errorA = ( BaseException ) renameCLThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( renameCLThread.getErrorMsg() );
            }
        }

        if ( !split ) {
            Integer[] errnosB = { -23, -147, -190 };
            BaseException errorB = ( BaseException ) splitThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( splitThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( rename && !split ) {
                RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
                List< String > groups = new ArrayList< String >();
                groups.add( sourceGroup );
                checkSplitResult( db, csName, newCLName, groups );
            } else if ( !rename && split ) {
                cs = db.getCollectionSpace( csName );
                Assert.assertTrue( cs.isCollectionExist( clName ),
                        "cl is been rename faild, should exist" );
                List< String > groups = new ArrayList< String >();
                groups.add( sourceGroup );
                groups.add( targetGroup );
                checkSplitResult( db, csName, clName, groups );
            } else if ( rename && split ) {
                RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
                List< String > groups = new ArrayList< String >();
                groups.add( sourceGroup );
                groups.add( targetGroup );
                checkSplitResult( db, csName, newCLName, groups );
            } else {
                Assert.fail( "rename cl and split cl all failed" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, csName, clName );
            CommLib.clearCL( sdb, csName, newCLName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    private class SplitThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( sourceGroup, targetGroup,
                        new BasicBSONObject( "Partition", 1024 ),
                        new BasicBSONObject( "Partition", 2048 ) );
            }
        }
    }

    private DBCollection createShardingCL() {
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "hash" );
        options.put( "Group", sourceGroup );
        return cs.createCollection( clName, options );
    }

    private void checkSplitResult( Sequoiadb db, String csName, String clName,
            List< String > groups ) {

        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        if ( !cur.hasNext() ) {
            Assert.fail( "cl is not exist, " + csName + "." + clName );
        }

        Set< String > actGroups = new HashSet< String >();
        BSONObject obj = cur.getNext();
        BasicBSONList cataInfo = ( BasicBSONList ) obj.get( "CataInfo" );
        for ( int i = 0; i < cataInfo.size(); i++ ) {
            BSONObject info = ( BSONObject ) cataInfo.get( i );
            String groupName = ( String ) info.get( "GroupName" );
            // 判断groupName是否属于groups
            if ( !groups.contains( groupName ) ) {
                Assert.fail( "groupName error: exp: " + groups.toString()
                        + " act: " + cataInfo.toString() );
            }
            // 将groupName存到set中,去重
            actGroups.add( groupName );

            // 判断切分范围
            if ( groups.size() == 1 ) {
                Assert.assertEquals( info.get( "LowBound" ),
                        new BasicBSONObject( "", 0 ) );
                Assert.assertEquals( info.get( "UpBound" ),
                        new BasicBSONObject( "", 4096 ) );
            } else if ( ( int ) info.get( "ID" ) == 0 ) {
                Assert.assertEquals( info.get( "LowBound" ),
                        new BasicBSONObject( "", 0 ) );
                Assert.assertEquals( info.get( "UpBound" ),
                        new BasicBSONObject( "", 1024 ) );
            } else if ( ( int ) info.get( "ID" ) == 1 ) {
                Assert.assertEquals( info.get( "LowBound" ),
                        new BasicBSONObject( "", 1024 ) );
                Assert.assertEquals( info.get( "UpBound" ),
                        new BasicBSONObject( "", 2048 ) );
            } else {
                Assert.assertEquals( info.get( "LowBound" ),
                        new BasicBSONObject( "", 2048 ) );
                Assert.assertEquals( info.get( "UpBound" ),
                        new BasicBSONObject( "", 4096 ) );
            }
        }

        // 检查切分组是否符合预期值
        if ( actGroups.size() != groups.size() ) {
            Assert.fail( "cataInfo error: exp: " + groups.toString() + " act: "
                    + actGroups.toString() );
        }
    }

}
