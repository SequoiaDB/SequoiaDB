package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

import org.bson.BSONObject;
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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @TestName: 数据操作同时对子表进行切分
 * @author ouyangzhongnan
 * @version 1.00
 */
public class CrudSplitTest10481 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private static Sequoiadb sdb_other = null;
    private ArrayList< String > replicaGroupNames = null;
    private CollectionSpace cs = null;
    private DBCollection maincl;
    private String mainclName = "maincl_10481";
    private String[] subclNames = { "maincl_10481_subcl_0",
            "maincl_10481_subcl_1" };

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        // 一定需要需要G3D3,并获取三个数据组
        CommLib commlib = new CommLib();
        if ( commlib.isStandAlone( sdb ) || commlib.OneGroupMode( sdb ) ) {
            sdb.disconnect();
            throw new SkipException(
                    "run mode is standalone or oneGroupMode,test case skip" );
        }
        try {
            replicaGroupNames = sdb.getReplicaGroupNames();
            List< String > exclude = new ArrayList<>();
            exclude.add( "SYSCoord" );
            exclude.add( "SYSCatalogGroup" );
            replicaGroupNames.removeAll( exclude );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false,
                    "judge at least three data sets failed " + e.getMessage() );
        }

        // 获取建立另一个连接，连不同的coord
        try {
            ReplicaGroup coordRG = sdb.getReplicaGroup( "SYSCoord" );
            BSONObject coordRGBson = ( BSONObject ) coordRG.getDetail();
            BasicBSONList coordList = ( BasicBSONList ) coordRGBson
                    .get( "Group" );
            Iterator< Object > iterator = coordList.iterator();
            while ( iterator.hasNext() ) {
                BSONObject bsonObject = ( BSONObject ) iterator.next();
                String coordHostName = ( String ) bsonObject.get( "HostName" );
                if ( !coordHostName.equals( SdbTestBase.hostName ) ) {
                    sdb_other = new Sequoiadb(
                            coordHostName + ":" + SdbTestBase.serviceName, "",
                            "" );
                    break;
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false,
                    "get other coord and to connection it failed "
                            + e.getMessage() );
        }

        createCL();
        initData();
    }

    @Test
    public void test() {
        SplitTest split = new SplitTest();
        try {
            split.start();

            int a = 0;
            for ( int i = 400; i < 3000; i++ ) {
                a = i % 200;
                maincl.insert( ( BSONObject ) JSON
                        .parse( " {name_:'" + i + "',a:" + a + "} " ) );
            }
            for ( int i = 0; i < 200; i++ ) {
                maincl.upsert(
                        ( BSONObject ) JSON
                                .parse( " {name:'name_" + i + "'} " ),
                        ( BSONObject ) JSON.parse(
                                " {$set:{name:'updatename_" + i + "'}} " ),
                        null );
            }
            for ( int i = 200; i < 400; i++ ) {
                maincl.delete( ( BSONObject ) JSON
                        .parse( " {name:'name_" + i + "'} " ) );
            }

            Assert.assertEquals( split.isSuccess(), true, split.getErrorMsg() );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, "crud data faild: " + e.getMessage() );
        } finally {
            if ( split != null ) {
                split.join();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        checkData();
        // drop maincl and subcl
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            for ( String subclName : subclNames ) {
                if ( cs.isCollectionExist( subclName ) ) {
                    cs.dropCollection( subclName );
                }
            }
            if ( cs.isCollectionExist( mainclName ) ) {
                cs.dropCollection( mainclName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( "drop maincl or subcl failed: " + e.getMessage() );
        } finally {
            sdb.disconnect();
            sdb_other.disconnect();
        }
    }

    /**
     * 创建主表子表，并挂载
     */
    private void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        try {
            String mainclOptions = "{IsMainCL:true, ShardingKey:{a:1},ShardingType:'range',ReplSize:0,Compressed:true}";
            String[] subclOptions = {
                    "{Group:'" + replicaGroupNames.get( 0 ) + "'}",
                    "{Group:'" + replicaGroupNames.get( 1 )
                            + "',ShardingKey:{a:1},ShardingType:'range',ReplSize:0,Compressed:true}", };
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.createCollection( mainclName,
                    ( BSONObject ) JSON.parse( mainclOptions ) );
            cs.createCollection( subclNames[ 0 ],
                    ( BSONObject ) JSON.parse( subclOptions[ 0 ] ) );
            cs.createCollection( subclNames[ 1 ],
                    ( BSONObject ) JSON.parse( subclOptions[ 1 ] ) );
            maincl.attachCollection( SdbTestBase.csName + "." + subclNames[ 0 ],
                    ( BSONObject ) JSON
                            .parse( "{ LowBound:{a:0},UpBound:{a:100} }" ) );
            maincl.attachCollection( SdbTestBase.csName + "." + subclNames[ 1 ],
                    ( BSONObject ) JSON
                            .parse( "{ LowBound:{a:100},UpBound:{a:200} }" ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false,
                    "create collections faild: " + e.getMessage() );
        }
    }

    private void initData() {
        try {
            List< BSONObject > initData = new ArrayList<>();
            int a = 0;
            for ( int i = 0; i < 400; i++ ) {
                a = i % 200;
                initData.add( ( BSONObject ) JSON
                        .parse( " {name:'name_" + i + "', a:" + a + "} " ) );
            }
            maincl.bulkInsert( initData, 0 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, "init data faild: " + e.getMessage() );
        }
    }

    private void checkData() {
        boolean flag = true;
        try {
            // check count record
            long count = maincl.getCount();
            if ( count == 2800 ) {
                // check subcl and group
                DBCursor explain_A50 = maincl.explain(
                        ( BSONObject ) JSON.parse( " {a:50} " ), null, null,
                        null, 0, -1, DBQuery.FLG_QUERY_STRINGOUT, null );
                DBCursor explain_A149 = maincl.explain(
                        ( BSONObject ) JSON.parse( " {a:149} " ), null, null,
                        null, 0, -1, DBQuery.FLG_QUERY_STRINGOUT, null );
                DBCursor explain_A150 = maincl.explain(
                        ( BSONObject ) JSON.parse( " {a:150} " ), null, null,
                        null, 0, -1, DBQuery.FLG_QUERY_STRINGOUT, null );
                BSONObject[] objArr = { explain_A50.getNext(),
                        explain_A149.getNext(), explain_A150.getNext() };

                if ( ( explain_A50.getNext() == null
                        || explain_A149.getNext() == null
                        || explain_A150.getNext() == null )
                        && ( ( ( BasicBSONList ) objArr[ 0 ]
                                .get( "SubCollections" ) ).size() == 1
                                && ( ( BasicBSONList ) objArr[ 1 ]
                                        .get( "SubCollections" ) ).size() == 1
                                && ( ( BasicBSONList ) objArr[ 2 ]
                                        .get( "SubCollections" ) )
                                                .size() == 1 ) ) {
                    String subcl_A50 = ( String ) ( ( BSONObject ) ( ( BasicBSONList ) objArr[ 0 ]
                            .get( "SubCollections" ) ).get( 0 ) ).get( "Name" );
                    if ( !( replicaGroupNames.get( 0 )
                            .equals( objArr[ 0 ].get( "GroupName" ) )
                            && ( SdbTestBase.csName + "." + subclNames[ 0 ] )
                                    .equals( subcl_A50 ) ) ) {
                        System.out.println( "a" );
                        flag = false;
                    }
                    String subcl_A149 = ( String ) ( ( BSONObject ) ( ( BasicBSONList ) objArr[ 1 ]
                            .get( "SubCollections" ) ).get( 0 ) ).get( "Name" );
                    if ( !( replicaGroupNames.get( 1 )
                            .equals( objArr[ 1 ].get( "GroupName" ) )
                            && ( SdbTestBase.csName + "." + subclNames[ 1 ] )
                                    .equals( subcl_A149 ) ) ) {
                        System.out.println( "b" );
                        flag = false;
                    }
                    String subcl_A150 = ( String ) ( ( BSONObject ) ( ( BasicBSONList ) objArr[ 2 ]
                            .get( "SubCollections" ) ).get( 0 ) ).get( "Name" );
                    if ( !( replicaGroupNames.get( 0 )
                            .equals( objArr[ 2 ].get( "GroupName" ) )
                            && ( SdbTestBase.csName + "." + subclNames[ 1 ] )
                                    .equals( subcl_A150 ) ) ) {
                        System.out.println( "c" );
                        flag = false;
                    }
                } else {
                    flag = false;
                }
            } else {
                flag = false;
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, "check data faild: " + e.getMessage() );
        }
        Assert.assertTrue( flag, "check data not expected" );
    }

    /**
     * 切分线程
     */
    class SplitTest extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            DBCollection subcls_1 = sdb_other
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( subclNames[ 1 ] );
            subcls_1.split( replicaGroupNames.get( 1 ),
                    replicaGroupNames.get( 0 ),
                    ( BSONObject ) JSON.parse( " {a:150} " ),
                    ( BSONObject ) JSON.parse( " {a:200} " ) );
        }
    }

}
