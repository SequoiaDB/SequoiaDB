package com.sequoiadb.split.basefunc;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
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
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-10879: hash分区表指定分区键值为bool类型，执行切分 1、创建cl，指定shardingType为“hash”
 *                        2、向cl中插入数据,其中分区键字段值为bool类型 3、执行split，设置百分比切分条件
 *                        4、查看数据切分结果（分别连接coord、源组data、目标组data查询
 * @Author linsuqiang
 * @Date 2017-01-04
 * @Version 1.00
 */
public class TestSplit10879 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl_10879";
    private String srcGroup = null;
    private String dstGroup = null;
    private List< BSONObject > insertedRecs = new ArrayList< BSONObject >();
    private double percent;
    private int expSrcLowBound;
    private int expSrcUpBound;
    private int expDstLowBound;
    private int expDstUpBound;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            if ( SplitBaseUtils.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( SplitBaseUtils.OneGroupMode( sdb ) ) {
                throw new SkipException( "skip One group mode" );
            }
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

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
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        try {
            initData();
            DBCollection cl = SplitBaseUtils.createHashCl( sdb, clName,
                    srcGroup );
            cl.bulkInsert( insertedRecs, 0 );
            cl.split( srcGroup, dstGroup, percent );
            SplitBaseUtils.checkSplitOnCoord( cl, insertedRecs );
            checkCatalog( cl );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    private void initData() {
        // initialize data group information
        List< String > rgNames = SplitBaseUtils.getDataGroups( sdb );
        srcGroup = rgNames.get( 0 );
        dstGroup = rgNames.get( 1 );
        // initialize records to insert
        int recsCnt = 10000;
        for ( int i = 0; i < recsCnt; i++ ) {
            BSONObject rec = new BasicBSONObject();
            boolean value = ( i % 2 == 0 );
            rec.put( "a", value );
            insertedRecs.add( rec );
        }
        // initialize split condition
        percent = 50;
        // define excepted catalog
        expSrcLowBound = 0;
        expSrcUpBound = 2048;
        expDstLowBound = 2048;
        expDstUpBound = 4096;
    }

    private void checkCatalog( DBCollection cl ) {
        try {
            // get CataInfo
            BSONObject option = new BasicBSONObject();
            option.put( "Name", csName + '.' + clName );
            DBCursor snapshot = sdb.getSnapshot( 8, option, null, null );
            BasicBSONList cataInfo = ( BasicBSONList ) snapshot.getNext()
                    .get( "CataInfo" );
            snapshot.close();

            // justify source group catalog information
            BSONObject srcInfo = ( BSONObject ) cataInfo.get( 0 );
            if ( !( ( ( String ) srcInfo.get( "GroupName" ) ).equals( srcGroup )
                    && ( ( BasicBSONObject ) srcInfo.get( "LowBound" ) )
                            .getInt( "" ) == expSrcLowBound
                    && ( ( BasicBSONObject ) srcInfo.get( "UpBound" ) )
                            .getInt( "" ) == expSrcUpBound ) ) {
                Assert.fail( "split fail: source group cataInfo is wrong" );
            }

            // justify destination group catalog information
            BSONObject dstInfo = ( BSONObject ) cataInfo.get( 1 );
            if ( !( ( ( String ) dstInfo.get( "GroupName" ) ).equals( dstGroup )
                    && ( ( BasicBSONObject ) dstInfo.get( "LowBound" ) )
                            .getInt( "" ) == expDstLowBound
                    && ( ( BasicBSONObject ) dstInfo.get( "UpBound" ) )
                            .getInt( "" ) == expDstUpBound ) ) {
                Assert.fail(
                        "split fail: destination group cataInfo is wrong" );
            }
        } catch ( BaseException e ) {
            e.getStackTrace();
            Assert.fail( e.getMessage() );
        }
    }
}
