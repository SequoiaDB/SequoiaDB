package com.sequoiadb.rename;

import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCS_16812.java seqDB-16812:并发修改cs名和analyze统计集合空间信息
 * @author luweikang
 * @date 2018年10月17日
 * @review wuyan 2018.10.31
 */
public class RenameCS_16812 extends SdbTestBase {

    private String oldCSName = "renameCS_16812_old";
    private String newCSName = "renameCS_16812_new";
    private String clName = "rename_CL_16812";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private int recordNum = 1000;
    private String groupName = null;

    // 覆盖mode 5种模式
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        return new Object[][] {
                new Object[] { 1, oldCSName + "_" + 0, newCSName + "_" + 0 },
                new Object[] { 2, oldCSName + "_" + 1, newCSName + "_" + 1 },
                new Object[] { 3, oldCSName + "_" + 2, newCSName + "_" + 2 },
                new Object[] { 4, oldCSName + "_" + 3, newCSName + "_" + 3 },
                new Object[] { 5, oldCSName + "_" + 4, newCSName + "_" + 4 }, };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > rgNames = CommLib.getDataGroupNames( sdb );
        groupName = rgNames.get( 0 );

        for ( int i = 0; i < 5; i++ ) {
            cs = sdb.createCollectionSpace( oldCSName + "_" + i );
            cl = cs.createCollection( clName,
                    new BasicBSONObject( "Group", groupName ) );
            RenameUtil.insertData( cl, recordNum );
            sdb.analyze( new BasicBSONObject( "CollectionSpace",
                    oldCSName + "_" + i ) );
        }
    }

    @Test(dataProvider = "operData")
    public void test( int analyzeMode, String testCSName,
            String testNewCSName ) {
        RenameCSThread reCSNameThread = new RenameCSThread( testCSName,
                testNewCSName );
        AnalyzeCL analyzeCL = new AnalyzeCL( analyzeMode, testCSName );

        reCSNameThread.start();
        analyzeCL.start();

        boolean analyze = analyzeCL.isSuccess();
        Assert.assertTrue( reCSNameThread.isSuccess(),
                "concurrent exec rename cs and analyze, renameCS failed "
                        + reCSNameThread.getErrorMsg() );

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCSResult( db, testCSName, testNewCSName, 1 );
        }

        if ( !analyze ) {
            Integer[] errnos = { -23, -34, -264 };
            BaseException error = ( BaseException ) analyzeCL.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( analyzeCL.getErrorMsg() );
            }
        }

        // 只校验rename之后旧统计信息有清理,新统计信息有创建,不关注内容
        try ( Sequoiadb dataDB = sdb.getReplicaGroup( groupName ).getMaster()
                .connect()) {
            DBCollection stat = dataDB.getCollectionSpace( "SYSSTAT" )
                    .getCollection( "SYSCOLLECTIONSTAT" );
            BSONObject filter1 = new BasicBSONObject();
            filter1.put( "CollectionSpace", testCSName );
            long num1 = stat.getCount( filter1 );
            Assert.assertEquals( num1, 0, "check cs " + testCSName
                    + " be rename, analyze shuold cleanUp" );
            BSONObject filter2 = new BasicBSONObject();
            filter2.put( "CollectionSpace", testNewCSName );
            long num2 = stat.getCount( filter2 );
            Assert.assertEquals( num2, 1, "check new name cs " + testNewCSName
                    + " , analyze shuold create" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < 5; i++ ) {
                CommLib.clearCS( sdb, newCSName + "_" + i );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        private String oldCSName;
        private String newCSName;

        public RenameCSThread( String oldCSName, String newCSName ) {
            this.oldCSName = oldCSName;
            this.newCSName = newCSName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( oldCSName, newCSName );
            }
        }
    }

    private class AnalyzeCL extends SdbThreadBase {

        private int analyzeMode;
        private String csName1;

        public AnalyzeCL( int analyzeMode, String csName ) {
            this.analyzeMode = analyzeMode;
            this.csName1 = csName;
        }

        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject options = new BasicBSONObject();
                options.put( "Mode", analyzeMode );
                options.put( "Collection", csName1 + "." + clName );
                options.put( "GroupName", groupName );
                options.put( "NodeSelect", "master" );
                db.analyze( options );
            }
        }

    }

}
