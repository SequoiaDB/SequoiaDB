package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.CommLib;

/**
 * 用例要求： 1.创建主子表，子表是hash表； 2.使用java驱动向主表中批量的写入数据，其中_id字段是自动生成；
 * 3.将子表按照_id字段进行hash切分[domainAutoSplit]操作，查看数据落盘是否均匀.
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class Split101 extends SdbTestBase {
    private long count;
    private DBCollection mainCl;
    private DBCollection subCl1;
    private Sequoiadb sdb = null;
    private CollectionSpace commCS1;
    private CommLib commlib = new CommLib();
    private ArrayList< BSONObject > InsertRecods;
    private String domainName = "domain101";
    private String mainClName1 = "mainCL101_1";
    private String subClName1 = "subCL101_1";
    private String csName1 = "mainCS101";
    private List< String > groupsName;
    private int recsCnt = 1000;
    private BSONObject domainOption = new BasicBSONObject();
    private ArrayList< String > replicaGroups = new ArrayList< String >();

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 跳过 standAlone 和数据组不足的环境
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            // 源组和目标组
            groupsName = commlib.getDataGroupNames( sdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            replicaGroups = commlib.getDataGroupNames( sdb );
            // init domain arg
            domainOption.put( "Groups", replicaGroups );
            domainOption.put( "AutoSplit", true );
            // create domain
            sdb.createDomain( domainName, domainOption );
            // create cs
            BSONObject options = ( BSONObject ) JSON
                    .parse( "{Domain:'" + domainName + "'}" );
            commCS1 = sdb.createCollectionSpace( csName1, options );
            mainCl = createCL( mainClName1, commCS1,
                    "{IsMainCL:true,ShardingKey:{a:1}}" );
            subCl1 = createCL( subClName1, commCS1,
                    "{ShardingKey:{\"_id\":1}," + "ShardingType:\"hash\"}" );

        } catch ( BaseException e ) {
            Assert.fail( "TestCase100 setUp error:" + e.getMessage() );
        }
        attach();
        insertData();

    }

    @Test
    public void checkSplit() {
        Sequoiadb destDataNode = null;
        DBCursor cursor = null;
        try {
            for ( int i = 0; i < groupsName.size(); i++ ) {
                // group1组数据量检查
                destDataNode = sdb.getReplicaGroup( groupsName.get( i ) )
                        .getMaster().connect();
                DBCollection destCL1 = destDataNode
                        .getCollectionSpace( csName1 )
                        .getCollection( subClName1 );
                count = destCL1.getCount();
                if ( count < ( recsCnt / groupsName.size() * 0.7 )
                        || count > ( recsCnt / groupsName.size() * 1.3 ) ) {
                    Assert.fail( "split count error " );
                }
            }
            // 检查数据是否丢失
            cursor = this.subCl1.query( null, null, "{_id:1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            Assert.assertEquals( actual, this.InsertRecods );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName1 ) ) {
                sdb.dropCollectionSpace( commCS1.getName() );
            }
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    public DBCollection createCL( String clName, CollectionSpace cs,
            String option ) {
        DBCollection Cl = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            Cl = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( option ) );
        } catch ( BaseException e ) {
            Assert.fail( "createCl error" + e.getMessage() );
        }
        return Cl;
    }

    public void attach() {
        // attach子表
        try {
            this.mainCl.attachCollection( subCl1.getFullName(),
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{a:1},UpBound:{a:100}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( " subCl1 attach error:" + e.getMessage() );
        }
    }

    public void insertData() {
        this.InsertRecods = new ArrayList< BSONObject >();
        try {
            for ( int i = 0; i < recsCnt; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "a", 1 );
                mainCl.insert( bson );
                this.InsertRecods.add( bson );
            }
        } catch ( BaseException e ) {
            Assert.fail( "insert error:" + e.getMessage() );
        }
    }

}
