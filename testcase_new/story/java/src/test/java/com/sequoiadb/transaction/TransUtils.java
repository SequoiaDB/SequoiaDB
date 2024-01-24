package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 
 * @description Utils for this package class
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class TransUtils extends SdbTestBase {

    public static final int FLG_INSERT_CONTONDUP = 0x00000001;
    /**
     * @param delayTime
     *            线程延时启动时间，线程Thread.sleep(事务等锁超时时间-5s)
     * @param transTimeoutSession
     *            会话级别事务超时时间
     */
    public static final int transTimeoutSession = 10;
    public static final int delayTime = ( transTimeoutSession - 5 ) * 1000;

    /*
     * loopNum rbs清理的用例，重复执行次数
     */
    public static final int loopNum = 50;

    public static CollectionSpace createCS( String csName, Sequoiadb db )
            throws BaseException {
        CollectionSpace tmp = null;
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            tmp = db.createCollectionSpace( csName );

        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    public static CollectionSpace createCS( String csName, Sequoiadb db,
            String option ) throws BaseException {
        CollectionSpace tmp = null;
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            tmp = db.createCollectionSpace( csName,
                    ( BSONObject ) JSON.parse( option ) );

        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    public static Domain createDomain( Sequoiadb sdb, String name,
            ArrayList< String > groupArr, int size, boolean autoSplit )
            throws BaseException {
        Domain domain = null;
        try {
            if ( sdb.isDomainExist( name ) ) {
                domain = sdb.getDomain( name );
            } else {
                StringBuilder groups = new StringBuilder();
                String option = new String();
                for ( int i = 0; i < groupArr.size() && i < size; i++ ) {
                    groups.append( "\"" ).append( groupArr.get( i ) )
                            .append( "\"," );
                }
                groups.deleteCharAt( groups.length() - 1 );
                groups.insert( 0, "[" );
                groups.append( "]" );
                if ( autoSplit ) {
                    option = "{\"Groups\":" + groups + ",\"AutoSplit\":true}";
                } else {
                    option = "{\"Groups\":" + groups + ",\"AutoSplit\":false}";
                }
                domain = sdb.createDomain( name,
                        ( BSONObject ) JSON.parse( option ) );
            }

        } catch ( BaseException e ) {
            throw e;
        }
        return domain;
    }

    public static boolean getDatabaseSnapshot( Sequoiadb db,
            String groupName ) {
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                "{RawData:true, IsPrimary:true, GroupName:'" + groupName + "'}",
                "{TransInfo:'', NodeName:''}", null );
        while ( cursor.hasNext() ) {
            BSONObject info = cursor.getNext();
            BSONObject transInfo = ( BSONObject ) info.get( "TransInfo" );
            String nodeName = ( String ) info.get( "NodeName" );
            int transCount = ( int ) transInfo.get( "TotalCount" );
            Long transBeginLSN = ( Long ) transInfo.get( "BeginLSN" );

            if ( transCount != 0 || transBeginLSN != -1 ) {
                System.out.println( "NodeName:" + nodeName + " has transCount:"
                        + transCount + ",transBeginLSN:" + transBeginLSN );
                return false;
            }
        }
        return true;
    }

    public static Boolean isTransisolationRR( Sequoiadb db ) {
        Boolean isRR = true;
        Object coordGlobTransConfig = null;
        Object dataGlobTransConfig;
        Object dataMvccConfig;
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                "{NodeName:'',globtranson:'','role':'',mvccon:''}", null );
        while ( cursor.hasNext() && isRR ) {
            BSONObject config = cursor.getNext();
            String role = ( String ) config.get( "role" );
            switch ( role ) {
            case "coord":
                coordGlobTransConfig = config.get( "globtranson" );
                if ( !coordGlobTransConfig.equals( "TRUE" ) ) {
                    isRR = false;
                }
                break;
            case "data":
                dataGlobTransConfig = config.get( "globtranson" );
                dataMvccConfig = config.get( "mvccon" );
                if ( !( dataGlobTransConfig.equals( "TRUE" )
                        && dataMvccConfig.equals( "TRUE" ) ) ) {
                    isRR = false;
                }
                break;
            case "catalog":
                break;

            }
        }
        return isRR;
    }

    public static DBCollection createCL( String clName, CollectionSpace cs,
            String option ) throws BaseException {
        DBCollection tmp = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            tmp = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( option ) );
        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    // 检查某集合是否仅含一个dest记录
    public static boolean isCollectionContainThisJSON( DBCollection cl,
            String dest ) throws BaseException {
        BSONObject bobj = ( BSONObject ) JSON.parse( dest );
        ArrayList< Object > resaults = new ArrayList<>();
        DBCursor dc = null;
        try {
            dc = cl.query( bobj, null, null, null );
            while ( dc.hasNext() ) {
                resaults.add( dc.getNext() );
            }
            if ( resaults.size() != 1 ) {
                return false;
            }
            BSONObject actual = ( BSONObject ) resaults.get( 0 );
            actual.removeField( "_id" );
            bobj.removeField( "_id" );
            if ( bobj.equals( actual ) ) {
                return true;
            } else {
                return false;
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public static String getKeyStack( Exception e, Object classObj ) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for ( int i = 0; i < stackElements.length; i++ ) {
            if ( stackElements[ i ].toString()
                    .contains( classObj.getClass().getName() ) ) {
                stackBuffer.append( stackElements[ i ].toString() )
                        .append( "\r\n" );
            }
        }
        String str = stackBuffer.toString();
        return str.substring( 0, str.length() - 2 );
    }

    public static ArrayList< String > getGroupName( Sequoiadb sdb,
            String csName, String clName ) throws BaseException {
        DBCursor dbc = null;
        ArrayList< String > resault = new ArrayList<>();
        try {
            ArrayList< String > groups = CommLib.getDataGroupNames( sdb );
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                return null;
            }
            String srcGroupName = ( String ) ( ( BSONObject ) list.get( 0 ) )
                    .get( "GroupName" );
            resault.add( srcGroupName );
            if ( groups.size() < 2 ) {
                return resault;
            }
            String destGroupName;
            if ( srcGroupName.equals( groups.get( 0 ) ) )
                destGroupName = groups.get( 1 );
            else
                destGroupName = groups.get( 0 );
            resault.add( destGroupName );
            return resault;
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    public static ArrayList< BSONObject > getReadActList( DBCursor cursor )
            throws BaseException {
        ArrayList< BSONObject > actRList = new ArrayList<>();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            actRList.add( record );
        }
        cursor.close();
        return actRList;
    }

    public static ArrayList< BSONObject > insertDatas( DBCollection cl,
            int startId, int endId, int insertValue ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            insertDatas.add( ( BSONObject ) JSON.parse(
                    "{_id:" + i + ",a:" + insertValue + ",b:" + i + "}" ) );
        }
        cl.insert( insertDatas );
        return insertDatas;
    }

    public static ArrayList< BSONObject > insertRandomDatas( DBCollection cl,
            int startId, int endId ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            BSONObject data = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ",a:" + i + ",b:" + i + "}" );
            insertDatas.add( data );
            expDatas.add( data );
        }
        Collections.shuffle( insertDatas );
        cl.insert( insertDatas );
        return expDatas;
    }

    public static ArrayList< BSONObject > insertRandomLengthRecords(
            DBCollection cl, int insertNum, int minStringLength,
            int maxStringLenth ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = 0; i < insertNum; i++ ) {
            StringBuilder sb = new StringBuilder();
            int stringLength = new Random().nextInt( maxStringLenth )
                    + minStringLength;
            for ( int j = 0; j < stringLength; j++ ) {
                sb.append( "a" );
            }
            insertDatas.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:" + i
                    + ",b:" + i + ",c:'" + sb + "'}" ) );
        }
        expDatas.addAll( insertDatas );
        Collections.shuffle( insertDatas );
        cl.insert( insertDatas );
        return expDatas;
    }

    public static ArrayList< BSONObject > insertRandomLengthRecords(
            DBCollection cl, int starId, int stopId, int minStringLength,
            int maxStringLenth ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = starId; i < stopId; i++ ) {
            StringBuilder sb = new StringBuilder();
            int stringLength = new Random().nextInt( maxStringLenth )
                    + minStringLength;
            for ( int j = 0; j < stringLength; j++ ) {
                sb.append( "a" );
            }
            insertDatas.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:" + i
                    + ",b:" + i + ",c:'" + sb + "'}" ) );
        }
        expDatas.addAll( insertDatas );
        Collections.shuffle( insertDatas );
        cl.insert( insertDatas );
        return expDatas;
    }

    public static ArrayList< BSONObject > prepareDatas( Sequoiadb db,
            DBCollection cl, int recordNums ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        int times = 0;
        int maxInsertNum = 100;
        int insertTimes = recordNums / 100;
        if ( recordNums % maxInsertNum != 0 ) {
            insertTimes++;
        }
        new Random().nextBytes( new byte[ 1024 ] );
        for ( int i = 0; i < insertTimes; i++ ) {
            insertDatas.clear();
            for ( int j = 0; j < maxInsertNum; j++ ) {
                if ( times < recordNums ) {
                    times++;
                } else {
                    break;
                }
                int currentNum = i * maxInsertNum + j;
                BSONObject data = ( BSONObject ) JSON.parse( "{_id:"
                        + currentNum + ", a:" + currentNum
                        + ", b:'test trans rr mode" + currentNum + "'}" );
                insertDatas.add( data );
                expDatas.add( data );
            }
            Collections.shuffle( insertDatas );
            if ( new Random().nextInt( 2 ) != 0 ) {
                TransUtils.beginTransaction( db );
                cl.insert( insertDatas );
                TransUtils.commitTransaction( db );
            } else {
                cl.insert( insertDatas );
            }
        }

        return expDatas;
    }

    public static ArrayList< BSONObject > getPrepareDatas( int recordNums ) {
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = 0; i < recordNums; i++ ) {
            BSONObject data = ( BSONObject ) JSON.parse( "{_id:" + i + ", a:"
                    + i + ", b:'test trans rr mode" + i + "'}" );
            expDatas.add( data );
        }
        return expDatas;
    }

    /**
     * 此方法只更新b字段
     * 
     * @param list
     * @param modify
     *            b字段新值
     * @param begin
     * @param end
     */

    public static void updateList( List< BSONObject > list, String modify,
            int begin, int end ) {
        for ( int i = begin; i < end; i++ ) {
            BSONObject data = ( BSONObject ) JSON.parse(
                    "{_id:" + i + ", a:" + i + ", b:'" + modify + "'}" );
            list.set( i, data );
        }
    }

    /**
     * 此方法更新a字段和b字段
     * 
     * @param list
     * @param inc
     *            a字段自增值
     * @param modify
     *            b字段新值
     * @param begin
     * @param end
     */

    public static void updateList( List< BSONObject > list, int inc,
            String modify, int begin, int end ) {
        for ( int i = begin; i < end; i++ ) {
            int a = ( int ) list.get( i ).get( "a" );
            BSONObject data = ( BSONObject ) JSON.parse( "{_id:" + i + ", a:"
                    + ( a + inc ) + ", b:'" + modify + "'}" );
            list.set( i, data );
        }
    }

    public static void removeList( List< BSONObject > list, int begin,
            int end ) {
        for ( int i = begin; i < end; i++ ) {
            list.remove( begin );
        }
    }

    public static boolean getReadActList( DBCursor cursor, StringBuilder expRes,
            int pos ) throws BaseException {
        String prefix = expRes.toString();
        int diff = 1;
        if ( pos >= 10 ) {
            diff = 2;
        }
        while ( cursor.hasNext() ) {
            BasicBSONObject record = ( BasicBSONObject ) cursor.getNext();
            String filedA = record.getString( "a" );
            if ( filedA.indexOf( prefix ) == -1 ) {
                return false;
            }

            if ( filedA.length() - prefix.length() != diff ) {
                return false;
            }

            if ( Integer
                    .parseInt( filedA.substring( prefix.length() ) ) != pos ) {
                return false;
            }

            pos++;
        }
        return true;
    }

    public static ArrayList< BSONObject > getUpdateDatas( int startId,
            int endId, int updateValue ) {
        ArrayList< BSONObject > updateDatas = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            updateDatas.add( ( BSONObject ) JSON.parse(
                    "{_id:" + i + ",a:" + updateValue + ",b:" + i + "}" ) );
        }
        return updateDatas;
    }

    public static ArrayList< BSONObject > getIncDatas( int startId, int endId,
            int incValue ) {
        ArrayList< BSONObject > incDatas = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            incDatas.add( ( BSONObject ) JSON.parse( "{_id:" + i + ",a:"
                    + ( incValue + i ) + ",b:" + i + "}" ) );
        }
        return incDatas;
    }

    /**
     * 构造复合索引所需要的数据 如：a:0, b:0 a:1, b:0 a:1, b:1 a:1, b:2 ... a:2, b:2 a:3, b:0
     * a:3, b:1 ... a 为偶数时，a 和 b 一致 a 为奇数时，有多条记录 a 相等，b 不相等 aStart a 的起始值，aEnd a
     * 的结束值，bStart a 为奇数时 b 的起始值，bEnd a 为奇数时 b 的结束值 返回 list 长度 为 11*(aEnd -
     * aStart)/2
     * 
     * @return
     */
    public static List< BSONObject > getCompositeRecords( int aStart, int aEnd,
            int bStart, int bEnd ) {
        int a = 0;
        int b = 0;
        int id = ( aStart / 2 ) * 11 + aStart % 2;
        List< BSONObject > records = new ArrayList<>();
        for ( int i = aStart; i < aEnd; i++ ) {
            if ( i % 2 == 0 ) {
                a = i;
                b = i;
                BSONObject object = ( BSONObject ) JSON.parse(
                        "{_id:" + id++ + ", a:" + a + ", b:" + b + "}" );
                records.add( object );
            } else {
                for ( int j = bStart; j < bEnd; j++ ) {
                    a = i;
                    b = j;
                    BSONObject object = ( BSONObject ) JSON.parse(
                            "{_id:" + id++ + ", a:" + a + ", b:" + b + "}" );
                    records.add( object );
                }
            }
        }
        Collections.shuffle( records );
        return records;
    }

    /**
     * 排序 参数 key ：true b 字段正序排序，false 逆序
     * 
     * @param records
     */
    public static void sortCompositeRecords( List< BSONObject > records,
            final boolean key ) {

        Collections.sort( records, new Comparator< BSONObject >() {
            @Override
            public int compare( BSONObject obj1, BSONObject obj2 ) {
                if ( ( int ) obj1.get( "a" ) == ( int ) obj2.get( "a" ) ) {
                    if ( key ) {
                        return ( int ) obj1.get( "b" )
                                - ( int ) obj2.get( "b" );
                    } else {
                        return -( ( int ) obj1.get( "b" )
                                - ( int ) obj2.get( "b" ) );
                    }
                } else {
                    return ( int ) obj1.get( "a" ) - ( int ) obj2.get( "a" );
                }
            }
        } );
    }

    /**
     * 创建主子表和切分表，切分表自动切分，主表attach平普通表和切分表，切分子表为自动切分，分区键为_id
     * 
     * @param sdb
     * @param csName
     * @param hashCLName
     * @param mainCLName
     * @param subCLName1
     *            子表名1
     * @param subCLName2
     *            子表名2
     * @param sep
     *            主表的切分范围为 (min - sep)(sep - max)
     */
    public static void createCLs( Sequoiadb sdb, String csName,
            String hashCLName, String mainCLName, String subCLName1,
            String subCLName2, int sep ) {
        createHashCL( sdb, csName, hashCLName );
        createMainCL( sdb, csName, mainCLName, subCLName1, subCLName2, sep );
    }

    /**
     * 创建切分表
     * 
     * @param sdb
     * @param csName
     * @param hashCLName
     */
    public static DBCollection createHashCL( Sequoiadb sdb, String csName,
            String hashCLName ) {
        DBCollection hashCL = sdb.getCollectionSpace( csName )
                .createCollection( hashCLName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{_id:1}, ShardingType:'hash', AutoSplit:true}" ) );
        return hashCL;
    }

    /**
     * 创建主子表
     * 
     * @param sdb
     * @param csName
     * @param mainCLName
     * @param subCLName1
     *            子表名1
     * @param subCLName2
     *            子表名2
     * @param sep
     *            主表的切分范围为(min - sep)(sep - max)
     */
    public static DBCollection createMainCL( Sequoiadb sdb, String csName,
            String mainCLName, String subCLName1, String subCLName2, int sep ) {
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .createCollection( mainCLName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{_id:1}, ShardingType:'range', IsMainCL:true}" ) );
        sdb.getCollectionSpace( csName ).createCollection( subCLName1 );
        sdb.getCollectionSpace( csName ).createCollection( subCLName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{_id:1}, ShardingType:'hash', AutoSplit:true}" ) );
        mainCL.attachCollection( csName + "." + subCLName1,
                ( BSONObject ) JSON
                        .parse( "{LowBound:{_id:{'$minKey':1}}, UpBound:{_id:"
                                + sep + "}}" ) );
        mainCL.attachCollection( csName + "." + subCLName2,
                ( BSONObject ) JSON.parse( "{LowBound:{_id:" + sep
                        + "}, UpBound:{_id:{'$maxKey':1}}}" ) );

        return mainCL;
    }

    /**
     * 查询记录
     * 
     * @param cl
     *            集合对象
     * @param orderBy
     *            排序规则
     * @param hint
     *            排序规则
     * @return 返回记录 BSONObject 的 List
     */
    public static List< BSONObject > queryToBSONList( DBCollection cl,
            String orderBy, String hint ) {
        return queryToBSONList( cl, null, null, orderBy, hint );
    }

    /**
     * 查询记录
     * 
     * @param cl
     *            集合对象
     * @param matcher
     *            匹配规则
     * @param selector
     *            选择规则
     * @param orderBy
     *            排序规则
     * @param hint
     *            索引条件
     * @return 返回记录 BSONObject 的 List
     */
    public static List< BSONObject > queryToBSONList( DBCollection cl,
            String matcher, String selector, String orderBy, String hint ) {
        List< BSONObject > records = null;
        DBCursor cursor = cl.query( matcher, selector, orderBy, hint );
        records = getReadActList( cursor );
        return records;
    }

    /**
     * 查询并检查记录正确性，同时校验扫描方式
     * 
     * @param cl
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String hint,
            List< BSONObject > expList ) {
        queryAndCheck( cl, null, null, null, hint, expList );
    }

    /**
     * 查询并检查记录正确性，同时校验扫描方式
     * 
     * @param cl
     * @param orderBy
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String orderBy,
            String hint, List< BSONObject > expList ) {
        queryAndCheck( cl, null, null, orderBy, hint, expList );
    }

    /**
     * 查询并检查记录正确性，同时校验扫描方式
     * 
     * @param cl
     * @param matcher
     * @param orderBy
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String matcher,
            String orderBy, String hint, List< BSONObject > expList ) {
        // 检查指定索引扫描时是否走了索引扫描
        BSONObject hintType = ( BSONObject ) JSON.parse( hint );
        if ( hintType.get( "" ) != null ) {
            checkIndexScan( cl, matcher, null, orderBy, hint );
        }
        queryAndCheck( cl, matcher, null, orderBy, hint, expList );
    }

    /**
     * 查询并检查记录正确性，不校验扫描方式
     * 
     * @param cl
     * @param matcher
     * @param orderBy
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void checkQueryResultOnly( DBCollection cl, String matcher,
            String orderBy, String hint, List< BSONObject > expList ) {
        List< BSONObject > actList = queryToBSONList( cl, matcher, null,
                orderBy, hint );
        Assert.assertEquals( actList, expList );
    }

    /**
     * 查询并检查记录正确性
     * 
     * @param cl
     * @param matcher
     * @param selector
     * @param orderBy
     * @param hint
     * @param expList
     *            预期结果，记录 BSONObject 的 List
     */
    public static void queryAndCheck( DBCollection cl, String matcher,
            String selector, String orderBy, String hint,
            List< BSONObject > expList ) {
        // 由于matchBlockingMethod只能判断一个接口，大量的用例均调用了该接口，因此先query再count！！！！
        checkRecord( cl, matcher, selector, orderBy, hint, expList );

        // 该测试点是校验count接口的，不能够删除
        if ( !( "rr".equals( SdbTestBase.testGroup )
                && ( matcher == null || matcher.equals( "" ) ) ) ) {
            checkCount( cl, matcher, orderBy, hint, expList );
        }

    }

    public static void checkIndexScan( DBCollection cl, String matcher,
            String selector, String orderBy, String hint ) {
        DBCursor cur = cl.explain( ( BSONObject ) JSON.parse( matcher ),
                ( BSONObject ) JSON.parse( selector ),
                ( BSONObject ) JSON.parse( orderBy ),
                ( BSONObject ) JSON.parse( hint ), 0, -1, 0,
                new BasicBSONObject( "Run", false ) );
        if ( !cur.hasNext() ) {
            Assert.fail( "the query access plan did not return." );
        }
        while ( cur.hasNext() ) {
            BSONObject explain = cur.getNext();
            if ( explain.containsField( "SubCollections" ) ) {
                BasicBSONList subCollections = ( BasicBSONList ) explain
                        .get( "SubCollections" );
                for ( int i = 0; i < subCollections.size(); i++ ) {
                    String scanType = ( String ) ( ( BSONObject ) subCollections
                            .get( i ) ).get( "ScanType" );
                    Assert.assertEquals( scanType, "ixscan" );
                }
            } else {
                Assert.assertEquals( explain.get( "ScanType" ), "ixscan" );
            }

        }
        cur.close();
    }

    /**
     * 校验记录
     * 
     * @param list
     */
    public static void checkRecord( DBCollection cl, String matcher,
            String selector, String orderBy, String hint,
            List< BSONObject > expList ) {
        // 检查指定索引扫描时是否走了索引扫描
        BSONObject hintType = ( BSONObject ) JSON.parse( hint );
        if ( hintType.get( "" ) != null ) {
            checkIndexScan( cl, matcher, selector, orderBy, hint );
        }

        List< BSONObject > actList = queryToBSONList( cl, matcher, selector,
                orderBy, hint );
        if ( actList.size() != expList.size() ) {
            Assert.fail( "lists don't have the same size expected ["
                    + expList.size() + "] but found [" + actList.size()
                    + "], actList: " + actList.toString() + ", expList: "
                    + expList.toString() );
        }
        for ( int i = 0; i < actList.size(); i++ ) {
            if ( !actList.get( i ).equals( expList.get( i ) ) ) {
                Assert.fail( "expected [" + expList.get( i ) + "], but found ["
                        + actList.get( i ) + "], actList: " + actList.toString()
                        + ", expList: " + expList.toString() );
            }
        }
    }

    /**
     * 校验记录数
     * 
     * @param list
     */
    public static void checkCount( DBCollection cl, String matcher,
            String orderBy, String hint, List< BSONObject > expList ) {
        BSONObject matcherBSON = ( BSONObject ) JSON.parse( matcher );
        BSONObject hintBSON = ( BSONObject ) JSON.parse( hint );
        long actCount = cl.getCount( matcherBSON, hintBSON );
        long expCount = expList.size();
        Assert.assertEquals( actCount, expCount );
    }

    /**
     * 对 List<BSONObject> 使用 _id 字段进行排序
     * 
     * @param list
     */
    public static void sortList( List< BSONObject > list ) {
        sortList( list, null );
    }

    /**
     * 对 List<BSONObject> 进行排序，sortField 为空时默认对 _id 字段排序
     * 
     * @param list
     * @param sortField
     */
    public static void sortList( List< BSONObject > list, String sortField ) {
        if ( sortField == null ) {
            sortField = "_id";
        }
        Collections.sort( list, new sortList( sortField ) );
    }

    /**
     * 创建唯一索引预期报 -38，rcuserbs 模式预期报 -334
     * 
     * @param cl
     * @param idxName
     * @param idxKey
     */
    public static void createUniIdxErr( DBCollection cl, String idxName,
            String idxKey ) {
        try {
            cl.createIndex( idxName, idxKey, true, false );
            Assert.fail( "CREATE IDX SHOULD THROW ERR" );
        } catch ( BaseException e ) {
            if ( "rcuserbs".equals( SdbTestBase.testGroup ) ) {
                if ( -334 != e.getErrorCode() ) {
                    throw e;
                }
            } else {
                if ( -38 != e.getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void dropCS( Sequoiadb db, String csName )
            throws InterruptedException {
        for ( int i = 0; i < 30; i++ ) {
            if ( db.isCollectionSpaceExist( csName ) ) {
                try {
                    db.dropCollectionSpace( csName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -147 ) {
                        throw e;
                    }

                }
            } else {
                break;
            }
            Thread.sleep( 1000 );
        }

    }

    public static boolean isLsnConsistency( Sequoiadb sdb, String groupName ) {
        boolean isConsistency;
        int eachSleepTime = 1000;
        int maxSleetTime = 600000;
        int alreadySleepTime = 0;
        List< String > nodeUrls = CommLib.getNodeAddress( sdb, groupName );

        do {
            isConsistency = true;
            long lsnOfPrevNode = -1;
            int versionOfPrevNode = 0;
            for ( String nodeUrl : nodeUrls ) {
                try ( Sequoiadb dataDB = new Sequoiadb( nodeUrl, "", "" ) ;) {
                    DBCursor cursor = dataDB.getSnapshot(
                            Sequoiadb.SDB_SNAP_DATABASE, null,
                            "{CurrentLSN:null, CompleteLSN:null}", null );
                    while ( cursor.hasNext() ) {
                        BasicBSONObject doc = ( BasicBSONObject ) cursor
                                .getNext();
                        long lsnOfCurNode = doc.getLong( "CompleteLSN" );
                        int versionOfCurNode = ( ( BasicBSONObject ) doc
                                .get( "CurrentLSN" ) ).getInt( "Version" );
                        if ( lsnOfPrevNode == -1 ) {
                            lsnOfPrevNode = lsnOfCurNode;
                            versionOfPrevNode = versionOfCurNode;
                            continue;
                        } else if ( lsnOfPrevNode != lsnOfCurNode
                                || versionOfPrevNode != versionOfCurNode ) {
                            isConsistency = false;
                            break;
                        }
                    }
                    cursor.close();
                }
            }

            if ( isConsistency ) {
                break;
            }

            if ( alreadySleepTime >= maxSleetTime ) {
                break;
            }

            try {
                Thread.sleep( eachSleepTime );
                alreadySleepTime += eachSleepTime;
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        } while ( true );

        return isConsistency;
    }

    // 获取事务ID
    public static String getTransactionID( Sequoiadb db ) {
        // 在已开启事务的会话上，执行一个无关的查询，以获取当前会话上的事务id
        String transactionID = null;
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( reservedCL );
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ) {
            cursor.getNext();
        }
        cursor.close();

        // 同一个事务只有1个事务id
        DBCursor snapshotCursor = db.getSnapshot(
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "",
                "{TransactionID:''}", "" );
        while ( snapshotCursor.hasNext() ) {
            transactionID = ( String ) snapshotCursor.getNext()
                    .get( "TransactionID" );
        }
        snapshotCursor.close();
        return transactionID;
    }

    // coord 节点到 data 节点的事务存在一定延时,因此需要增加一个超时时间,在超时时间内判断事务是否在等锁
    public static boolean isTransWaitLock( Sequoiadb db,
            String transactionID ) {
        int timeOut = 30;// 单位:秒
        int waitTime = 0;
        boolean isTransWaitLock = false;
        while ( waitTime < timeOut ) {
            DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS,
                    "{TransactionID:'" + transactionID + "'}",
                    "{WaitLock:\"\"}", "" );
            while ( cursor.hasNext() ) {
                BSONObject waitLock = ( BSONObject ) cursor.getNext()
                        .get( "WaitLock" );
                if ( !waitLock.isEmpty() ) {
                    isTransWaitLock = true;
                }
            }
            cursor.close();
            if ( isTransWaitLock ) {
                break;
            } else {
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
                waitTime++;
            }
        }
        return isTransWaitLock;
    }

    /**
     * zhaoxiaoni
     * 
     * @param records
     * @param start
     * @param end
     **/
    public static List< BSONObject > addList( List< BSONObject > records,
            int start, int end ) {
        for ( int i = start; i < end; i++ ) {
            BSONObject object = ( BSONObject ) JSON
                    .parse( "{ _id: " + i + ", a: " + i + ", b: " + i + " }" );
            records.add( object );
        }
        return records;
    }

    /**
     * zhaoxiaoni
     * 
     * @param records
     * @param start
     * @param end
     * @param updateContent
     **/
    public static List< BSONObject > updateList( List< BSONObject > records,
            int start, int end, int updateContent ) {
        for ( int i = start; i < end; i++ ) {
            BSONObject record = ( BSONObject ) JSON.parse( "{ _id: " + i
                    + ", a: " + updateContent + ", b: " + i + " }" );
            records.set( start + i, record );
        }
        return records;
    }

    public static List< BSONObject > updateList( List< BSONObject > records,
            int start, int end, String updateContent ) {
        for ( int i = start; i < end; i++ ) {
            BSONObject record = ( BSONObject ) JSON.parse( "{ _id: " + i
                    + ", a: '" + updateContent + "', b: " + i + " }" );
            records.set( start + i, record );
        }
        return records;
    }

    /**
     * zhaoxiaoni
     * 
     * @param records
     * @param start
     * @param end
     **/
    public static List< BSONObject > deleteList( List< BSONObject > records,
            int start, int end ) {
        for ( int i = start; i < end; i++ ) {
            records.remove( start );
        }
        return records;
    }

    /**
     * @description开启事务并打印事务ID
     * @param sequoiadb
     * @author luweikang
     */
    public static void beginTransaction( Sequoiadb sequoiadb ) {
        sequoiadb.beginTransaction();
        try {
            Thread.sleep( 100 );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
        // DBCollection cl = sequoiadb.getCollectionSpace( SdbTestBase.csName )
        // .getCollection( SdbTestBase.reservedCL );
        // DBCursor cur = cl.query();
        // while ( cur.hasNext() ) {
        // cur.getNext();
        // }
        // cur.close();
        // String transId = getTransactionID( sequoiadb );
        // // 输出用例名和事务ID到控制台
        // System.out.println( new Exception().getStackTrace()[ 1
        // ].getClassName()
        // + ": transID: " + transId );
        // // 写入用例事务ID到节点日志
        // sequoiadb.msg( new Exception().getStackTrace()[ 1 ].getClassName()
        // + ": transID: " + transId );
    }

    /**
     * @description提交事务后sleep 0.1s,避免从其他coord连过来的读事务，由于时间不同导致读不到记录
     * @param sequoiadb
     * @author zhaoyu
     */
    public static void commitTransaction( Sequoiadb sequoiadb ) {
        sequoiadb.commit();
        try {
            Thread.sleep( 100 );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }

    /**
     * 检查CL主备节点集合CompleteLSN一致
     * 
     * @param cl
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isCLConsistency( DBCollection cl ) throws Exception {

        Sequoiadb db = cl.getSequoiadb();
        boolean isConsistency = false;

        List< String > groupNames = getCLGroups( cl );
        for ( String groupName : groupNames ) {
            List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
            ReplicaGroup rg = db.getReplicaGroup( groupName );

            try ( Sequoiadb masterNode = rg.getMaster().connect()) {
                long completeLSN = -2;
                DBCursor cursor = masterNode.getSnapshot(
                        Sequoiadb.SDB_SNAP_SYSTEM, null, "{CompleteLSN: ''}",
                        null );
                if ( cursor.hasNext() ) {
                    BasicBSONObject snapshot = ( BasicBSONObject ) cursor
                            .getNext();
                    if ( snapshot.containsField( "CompleteLSN" ) ) {
                        completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                    }
                } else {
                    throw new Exception( masterNode.getNodeName()
                            + " can't not find system snapshot" );
                }
                cursor.close();

                for ( String nodeName : nodeNames ) {
                    if ( masterNode.getNodeName().equals( nodeName ) ) {
                        continue;
                    }
                    isConsistency = false;
                    try ( Sequoiadb nodeConn = rg.getNode( nodeName )
                            .connect()) {
                        DBCursor cur = null;
                        long checkCompleteLSN = -3;
                        for ( int i = 0; i < 600; i++ ) {
                            cur = nodeConn.getSnapshot(
                                    Sequoiadb.SDB_SNAP_SYSTEM, null,
                                    "{CompleteLSN: ''}", null );
                            if ( cur.hasNext() ) {
                                BasicBSONObject checkSnapshot = ( BasicBSONObject ) cur
                                        .getNext();
                                if ( checkSnapshot
                                        .containsField( "CompleteLSN" ) ) {
                                    checkCompleteLSN = ( long ) checkSnapshot
                                            .get( "CompleteLSN" );
                                }
                            }
                            cur.close();

                            if ( completeLSN <= checkCompleteLSN ) {
                                isConsistency = true;
                                break;
                            }
                            try {
                                Thread.sleep( 1000 );
                            } catch ( InterruptedException e ) {
                                e.printStackTrace();
                            }
                        }
                        if ( !isConsistency ) {
                            throw new Exception( "Group [" + groupName
                                    + "] node system snapshot is not the same, masterNode "
                                    + masterNode.getNodeName()
                                    + " CompleteLSN: " + completeLSN + ", "
                                    + nodeName + " CompleteLSN: "
                                    + checkCompleteLSN );
                        }
                    }
                }
            }
        }

        return isConsistency;
    }

    /**
     * 获取原始集合对应的数据组，原始集合可以是普通表、分区表
     * 
     * @param cl
     * @return List<String> 返回所有数据组
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< String > getCLGroups( DBCollection cl ) {
        List< String > groupNames = new ArrayList<>();
        Sequoiadb db = cl.getSequoiadb();
        if ( CommLib.isStandAlone( db ) ) {
            return groupNames;
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", cl.getFullName() );
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                null, null );
        HashSet< String > groupNamesSet = new HashSet<>();
        while ( cur.hasNext() ) {
            BasicBSONList bsonLists = ( BasicBSONList ) cur.getNext()
                    .get( "CataInfo" );
            for ( int i = 0; i < bsonLists.size(); i++ ) {
                BasicBSONObject obj = ( BasicBSONObject ) bsonLists.get( i );
                groupNamesSet.add( obj.getString( "GroupName" ) );
            }
        }
        groupNames.addAll( groupNamesSet );
        // groupNames数组元素排序,排序是为了cl的esIndexNames和cappedCLs能一一对应
        Collections.sort( groupNames, new Comparator< Object >() {
            @Override
            public int compare( Object o1, Object o2 ) {
                String str1 = ( String ) o1;
                String str2 = ( String ) o2;
                if ( str1.compareToIgnoreCase( str2 ) < 0 ) {
                    return -1;
                }
                return 1;
            }
        } );

        return groupNames;
    }

    /*
     * @description 获取切分任务状态
     * @param db
     * @param clFullName
     * @author zhaoyu
     */
    public static int getSplitTaskStatus( Sequoiadb db, String clFullName ) {
        int status = -1;
        BasicBSONObject matcher = new BasicBSONObject( "Name", clFullName );
        matcher.put( "TaskType", 0 ); // split task
        DBCursor cursor = db.listTasks( matcher, null, null, null );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            status = ( int ) record.get( "Status" );
        }
        cursor.close();
        return status;
    }

    /*
     * @description 通过testGroup判断是否随机获取coord节点
     * @param String testGroup
     * @author zhaoyu
     */
    public static Sequoiadb getRandomSequoiadb( String testGroup ) {
        if ( "rr".equals( SdbTestBase.testGroup )
                || "rrauto".equals( SdbTestBase.testGroup ) ) {
            return CommLib.getRandomSequoiadb();
        } else {
            return new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }
    }

}

/**
 * 
 * @description 排序类
 * @author yinzhen
 * @date 2019年7月19日
 */
class sortList implements Comparator< BSONObject > {
    private String sortField;

    sortList( String soerField ) {
        this.sortField = soerField;
    }

    @Override
    public int compare( BSONObject o1, BSONObject o2 ) {
        String field1 = String.valueOf( o1.get( sortField ) );
        String field2 = String.valueOf( o2.get( sortField ) );
        return field1.compareTo( field2 );
    }

}
