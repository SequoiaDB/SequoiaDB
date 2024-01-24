package com.sequoiadb.metaopr.commons;

import java.io.ByteArrayOutputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.logging.Logger;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.BeforeSuite;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.index.IndexBean;
import com.sequoiadb.lob.LobBean;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */
public class MyUtil {

    private static Logger log = Logger.getLogger( MyUtil.class.getName() );

    /**
     * 批量产生名字
     *
     * @param preName
     * @param num
     * @return
     */
    public static List< String > createNames( String preName, int num ) {
        List< String > names = new ArrayList<>( num + num / 10 );
        for ( int i = 0; i < num; i++ ) {
            names.add( preName + i );
        }
        return names;
    }

    /**
     * 打印开始时间
     *
     * @param obj
     */
    public static void printBeginTime( Object obj ) {
        System.out.println( obj.getClass().getName() + " begin at:"
                + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss" )
                        .format( new Date() ) );
    }

    /**
     * 打印结束时间
     *
     * @param obj
     */
    public static void printEndTime( Object obj ) {
        System.out.println( obj.getClass().getName() + " end at:"
                + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss" )
                        .format( new Date() ) );
    }

    /**
     * 获取连到编目节点的sdb连接
     *
     * @return
     */
    static MySequoiadb getMySdb() {
        SequoiadbDatasource ds = MyDataSource.getDataSource();
        try {
            MySequoiadb db = new MySequoiadb( ds.getConnection(), ds );
            return db;
        } catch ( InterruptedException e ) {
            return null;
        }
    }

    public static Sequoiadb getSdb() {
        return new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    /**
     * 关闭连接
     *
     * @param db
     */
    public static void closeDb( Sequoiadb db ) {
        if ( db.isClosed() == false ) {
            db.closeAllCursors();
            db.close();
        }
    }

    /**
     * 检查主备节点是否同步
     */
    public static boolean isCatalogGroupSync() throws ReliabilityException {
        GroupMgr mgr = GroupMgr.getInstance();
        GroupWrapper catalogGroup = mgr.getGroupByName( "SYSCatalogGroup" );
        boolean result = catalogGroup.checkInspect( 60 );
        return result;
    }

    /**
     * 插入给定数量的简单数据到cl
     *
     * @param csName
     * @param clName
     * @param number
     */
    public static void insertSimpleDataIntoCl( String csName, String clName,
            int number ) {
        try ( MySequoiadb db = getMySdb()) {
            List< BSONObject > list = new ArrayList<>( number );
            for ( int i = 0; i < number; i++ ) {
                list.add( new BasicBSONObject( "a", i ) );
            }
            db.getCollectionSpace( csName ).getCollection( clName )
                    .insert( list );
        }
    }

    /**
     * 获取编目主节点
     *
     * @return
     * @throws ReliabilityException
     */
    public static NodeWrapper getMasterNodeOfCatalog()
            throws ReliabilityException {
        GroupMgr mgr = GroupMgr.getInstance();
        NodeWrapper master = mgr.getGroupByName( "SYSCatalogGroup" )
                .getMaster();
        // mgr.close();//这里不能close
        return master;
    }

    public static GroupWrapper getCataGroup() throws ReliabilityException {
        GroupMgr mgr = GroupMgr.getInstance();
        return mgr.getGroupByName( "SYSCatalogGroup" );
    }

    private static List< String > groupNames = null;

    @BeforeSuite
    public static List< String > getDataGroupNames()
            throws ReliabilityException {
        if ( groupNames != null )
            return new ArrayList<>( groupNames );
        GroupMgr mgr = GroupMgr.getInstance();
        groupNames = mgr.getAllDataGroupName();
        return new ArrayList<>( groupNames );
    }

    /**
     * 获取编目备节点
     *
     * @return
     * @throws ReliabilityException
     */
    public static NodeWrapper getSlaveNodeOfCatalog()
            throws ReliabilityException {
        GroupMgr mgr = GroupMgr.getInstance();
        NodeWrapper slave = mgr.getGroupByName( "SYSCatalogGroup" ).getSlave();
        // mgr.close();这里不能close
        return slave;
    }

    /**
     * 在多个cs上都创建一个cl
     *
     * @param csnames
     * @param clname
     * @return
     */
    public static int createClInManyCs( List< String > csnames,
            String clname ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            for ( String name : csnames ) {
                try {
                    db.getCollectionSpace( name ).createCollection( clname );
                    count++;
                } catch ( BaseException e ) {
                    e.printStackTrace();
                }
            }
        }
        return count;
    }

    /**
     * 在一个cs上批量创建cl
     *
     * @param csname
     * @param clNames
     * @return
     */
    public static int createClInSingleCs( String csname,
            List< String > clNames ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            CollectionSpace cs = db.getCollectionSpace( csname );
            for ( String name : clNames ) {
                try {
                    cs.createCollection( name );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        }
        return count;
    }

    /**
     * 创建单个cl
     *
     * @param csName
     * @param clName
     * @return
     */
    public static int createCl( String csName, String clName ) {
        try ( MySequoiadb db = getMySdb()) {
            db.getCollectionSpace( csName ).createCollection( clName );
            return 1;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return 0;
        }
    }

    public static int createCl( String csName, String clName,
            BSONObject option ) {
        try ( MySequoiadb db = getMySdb()) {
            db.getCollectionSpace( csName ).createCollection( clName, option );
            return 1;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return 0;
        }
    }

    /**
     * 批量创建cs
     *
     * @param names
     * @return
     */
    public static int createCS( List< String > names ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            for ( String name : names ) {
                try {
                    db.createCollectionSpace( name );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        }
        return count;
    }

    /**
     * 创建cs，并指定domain
     *
     * @param csName
     * @param domainName
     * @return
     */
    public static int createCS( String csName, String domainName ) {
        try ( MySequoiadb db = getMySdb()) {
            BSONObject options = ( BSONObject ) JSON
                    .parse( "{'Domain':'" + domainName + "'}" );
            db.createCollectionSpace( csName, options );
            return 1;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return 0;
        }
    }

    public static boolean createCS( String name ) {
        try ( MySequoiadb db = getMySdb()) {
            db.createCollectionSpace( name );
            return true;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return false;
        }
    }

    /**
     * 创建domain，包含groupName1,groupName2
     *
     * @param domainName
     * @param groupName1
     * @param groupName2
     * @return
     */
    public static int createDomain( String domainName, String groupName1,
            String groupName2 ) {
        try ( MySequoiadb db = getMySdb()) {
            BSONObject options = ( BSONObject ) JSON.parse(
                    "{'Groups':['" + groupName1 + "','" + groupName2 + "']}" );
            db.createDomain( domainName, options );
            return 1;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return 0;
        }
    }

    /**
     * 创建domain，包含groupName1,groupName2,并指定autosplit为true
     *
     * @param domainName
     * @param groupName1
     * @param groupName2
     * @return
     */
    public static int createDomainAutoSplit( String domainName,
            String groupName1, String groupName2 ) {
        try ( MySequoiadb db = getMySdb()) {
            BSONObject options = ( BSONObject ) JSON
                    .parse( "{'Groups':['" + groupName1 + "','" + groupName2
                            + "'],'AutoSplit':true})" );
            db.createDomain( domainName, options );
            return 1;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return 0;
        }
    }

    /**
     * 修改domain属性
     *
     * @param domain
     * @param groupName1
     * @param groupName2
     */
    public static void alterDomain( Domain domain, String groupName1,
            String groupName2 ) {
        BSONObject options = ( BSONObject ) JSON.parse( "{'Groups':['"
                + groupName1 + "','" + groupName2 + "'],'AutoSplit':true})" );
        domain.alterDomain( options );
    }

    /**
     * 修改domain属性
     *
     * @param domainName
     * @param groupName1
     * @param groupName2
     */
    public static boolean alterDomain( String domainName, String groupName1,
            String groupName2 ) {
        try ( MySequoiadb db = getMySdb()) {
            Domain domain = db.getDomain( domainName );
            BSONObject options = ( BSONObject ) JSON
                    .parse( "{'Groups':['" + groupName1 + "','" + groupName2
                            + "'],'AutoSplit':true})" );
            domain.alterDomain( options );
            return true;
        } catch ( BaseException e ) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * 批量创建domain
     *
     * @param domainNames
     * @return
     */
    public static int createDomains( List< String > domainNames ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            List< String > groupNames = getDataGroupNames();
            String groupName1 = groupNames.get( 0 );
            String groupName2 = groupNames.get( 1 );
            BSONObject options = ( BSONObject ) JSON.parse(
                    "{'Groups':['" + groupName1 + "','" + groupName2 + "']}" );
            for ( String name : domainNames ) {
                try {
                    db.createDomain( name, options );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        } finally {
            return count;
        }
    }

    /**
     * 该方法适用于删除多个cs有相同cl name的情况
     *
     * @param csNames
     * @param clName
     * @return
     */
    public static int dropSingleClManyCs( List< String > csNames,
            String clName ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            for ( String name : csNames ) {
                try {
                    db.getCollectionSpace( name ).dropCollection( clName );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        }
        return count;
    }

    public static int dropCS( String csName ) {
        List< String > list = new ArrayList<>( 1 );
        list.add( csName );
        return dropCS( list );
    }

    public static int dropDomain( String domainName ) {
        try ( MySequoiadb db = getMySdb()) {
            db.dropDomain( domainName );
            return 1;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return 0;
        }
    }

    public static int dropDomains( List< String > domains ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            for ( String name : domains ) {
                try {
                    db.dropDomain( name );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        }
        return count;
    }

    /**
     * 删除cs
     *
     * @param csNames
     * @return
     */
    public static int dropCS( List< String > csNames ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            for ( String name : csNames ) {
                try {
                    db.dropCollectionSpace( name );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        }
        return count;
    }

    /**
     * 给定的domain是否已经全部删除
     *
     * @param domains
     * @return
     */
    public static boolean isDomainsDeleted( List< String > domains ) {
        try ( MySequoiadb db = getMySdb()) {
            for ( String domain : domains ) {
                if ( db.isDomainExist( domain ) ) {
                    log.severe( domain );
                    return false;
                }
            }
        }
        return true;
    }

    public static boolean isDomainAllCreated( List< String > domains ) {
        try ( MySequoiadb db = getMySdb()) {
            for ( String domain : domains ) {
                if ( db.isDomainExist( domain ) == false ) {
                    log.severe( domain );
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * 在指定cs上删除一个或多个cl，返回删除的个数
     *
     * @param csName
     * @param clNames
     * @return
     */
    public static int dropCls( String csName, List< String > clNames ) {
        int count = 0;
        try ( MySequoiadb db = getMySdb()) {
            CollectionSpace cs = db.getCollectionSpace( csName );
            for ( String clName : clNames ) {
                try {
                    cs.dropCollection( clName );
                    count++;
                } catch ( BaseException e ) {
                    // e.printStackTrace();
                }
            }
        }
        return count;
    }

    /**
     * 检查数组里的cs是否全部被删除
     *
     * @param csNames
     * @return
     */
    public static boolean isCsAllDeleted( List< String > csNames ) {
        try ( MySequoiadb db = getMySdb()) {
            for ( String name : csNames ) {
                boolean isExist = db.isCollectionSpaceExist( name );
                if ( isExist == true ) {
                    log.severe( name );
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * 检查数组的cs是否全部被创建
     *
     * @param csNames
     * @return
     */
    public static boolean isCsAllCreated( List< String > csNames ) {
        boolean falg = true;
        try ( MySequoiadb db = getMySdb()) {
            for ( String csName : csNames ) {
                if ( db.isCollectionSpaceExist( csName ) == false ) {
                    System.out.println(
                            "MyUtil.isCsAllCreated:this cs not exist: "
                                    + csName );
                    falg = false;
                }
            }
            return falg;
        }
    }

    public static boolean isCsExisted( String csName ) {
        boolean flag = false;
        try ( MySequoiadb db = getMySdb()) {
            DBCursor cursor = db.listCollectionSpaces();
            while ( cursor.hasNext() )
                if ( cursor.getNext().get( "Name" ).equals( csName ) )
                    flag = true;
        } finally {
            return flag;
        }
    }

    /**
     * 检查当前集群环境是否可用
     *
     * @return
     */
    public static boolean checkBusiness() {
        GroupMgr groupMgr = null;
        try {
            groupMgr = GroupMgr.getInstance();
            if ( groupMgr.checkBusiness() == true )
                return true;
            else {
                System.out.println(
                        "当前环境异常，GroupMgr.checkBusiness()==false，跳过该用例。" );
                throw new SkipException(
                        "当前环境异常，GroupMgr.checkBusiness()==false，跳过该用例。" );
            }
        } catch ( ReliabilityException e ) {
            System.out
                    .println( "当前环境异常，GroupMgr.checkBusiness()==false，跳过该用例。" );
            throw new SkipException(
                    "当前环境异常，GroupMgr.checkBusiness()==false，跳过该用例。" );
        }
    }

    /**
     * 把集合里记录清空
     *
     * @param csName
     * @param clName
     * @return
     */
    public static boolean deleteAllInCl( String csName, String clName ) {
        try ( MySequoiadb db = getMySdb()) {
            db.getCollectionSpace( csName ).getCollection( clName )
                    .delete( ( BSONObject ) null );
            return true;
        } catch ( BaseException e ) {
            // e.printStackTrace();
            return false;
        }
    }

    /**
     * 直连指定数据组的主节点，获取指定cl的记录数量。 当有异常抛出时，返回0。调用时需要注意，这样的处理是否符合你程序的处理逻辑。
     *
     * @param groupName
     * @param csName
     * @param clName
     * @return
     * @throws ReliabilityException
     */
    public static long getClCountFromGroupMaster( String groupName,
            String csName, String clName ) {
        GroupMgr mgr = null;
        Sequoiadb db = null;
        try {
            mgr = GroupMgr.getInstance();
            GroupWrapper groupWrapper = mgr.getGroupByName( groupName );
            NodeWrapper node = groupWrapper.getMaster();
            if ( node == null )
                return 0;
            db = node.connect();
            return db.getCollectionSpace( csName ).getCollection( clName )
                    .getCount();
        } catch ( BaseException e ) {
            e.printStackTrace();
            return 0;
        } catch ( ReliabilityException e ) {
            return 0;
        } finally {
            if ( db != null )
                db.close();
        }
    }

    public static long getClCountFromNode( String csName, String clName,
            NodeWrapper node ) {
        GroupMgr mgr = null;
        Sequoiadb db = null;
        try {
            db = node.connect();
            return db.getCollectionSpace( csName ).getCollection( clName )
                    .getCount();
        } catch ( BaseException e ) {
            e.printStackTrace();
            return 0;
        } finally {
            if ( db != null )
                db.close();
        }

    }

    /**
     * 判断数组里的cl是否全部删除
     *
     * @param csName
     * @param clNames
     * @return
     */
    public static boolean isClAllDeleted( String csName,
            List< String > clNames ) {
        try ( MySequoiadb db = getMySdb()) {
            CollectionSpace cs = db.getCollectionSpace( csName );
            for ( String clName : clNames ) {
                if ( cs.isCollectionExist( clName ) ) {
                    log.severe( clName );
                    return false;
                }
            }
            return true;
        }
    }

    /**
     * 判断数组的cl是否全部创建
     *
     * @return
     */
    public static boolean isClAllCreated( String csName,
            List< String > clNames ) {
        try ( MySequoiadb db = getMySdb()) {
            CollectionSpace cs = db.getCollectionSpace( csName );
            for ( String name : clNames ) {
                if ( cs.isCollectionExist( name ) == false ) {
                    log.severe( name );
                    return false;
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public static ObjectId createLob( String csName, String clName,
            byte[] bytes ) {
        try ( MySequoiadb db = getMySdb()) {
            return createLob( csName, clName, bytes, db.getSequoiadb() );
        }
    }

    public static void createLobs( String csName, String clName,
            List< LobBean > lobs ) {
        if ( lobs == null )
            throw new IllegalArgumentException( "lobs can not be null" );
        try ( MySequoiadb db = getMySdb()) {
            for ( LobBean lob : lobs ) {
                createLob( csName, clName, lob.getContent(),
                        db.getSequoiadb() );
                lob.setInSdb( true );
            }
        }
    }

    private static ObjectId createLob( String csName, String clName,
            byte[] bytes, Sequoiadb db ) {
        DBLob lob = null;
        ObjectId id = null;
        try {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            lob = cl.createLob();
            lob.write( bytes );
            lob.close();
            id = lob.getID();
        } finally {
            if ( lob != null )
                lob.close();
            return id;
        }
    }

    public static Map< ObjectId, byte[] > createLob( String csName,
            String clName, List< byte[] > data ) {
        Map< ObjectId, byte[] > map = new HashMap<>();
        try ( MySequoiadb db = getMySdb()) {
            for ( byte[] datum : data ) {
                ObjectId id = createLob( csName, clName, datum,
                        db.getSequoiadb() );
                map.put( id, datum );
            }
        }
        return map;
    }

    public static byte[] readLob( String csName, String clName,
            ObjectId lobID ) {
        DBLob lob = null;
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            return readLob( cl, lobID );
        }
    }

    public static byte[] readLob( DBCollection cl, ObjectId id ) {
        if ( cl == null || id == null )
            throw new IllegalArgumentException();
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        try {
            DBLob lob = cl.openLob( id );
            lob.read( outputStream );
            lob.close();
        } catch ( BaseException e ) {
            e.printStackTrace();
            if ( e.getErrorCode() != -269 )
                throw e;
            else {
                return "-269".getBytes();
            }
        }
        return outputStream.toByteArray();
    }

    public static byte[] createRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    @Deprecated
    public static boolean compareMd5( byte[] rawByte, byte[] targetMd5Value ) {
        return Arrays.equals( getMd5( rawByte ), targetMd5Value );
    }

    public static byte[] getMd5( byte[] bytes ) {
        MessageDigest md5 = null;
        try {
            md5 = MessageDigest.getInstance( "MD5" );
            return md5.digest( bytes );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            return null;
        }
    }

    public static void deleteAllLobs( String csName, String clName ) {
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.listLobs();
            while ( cursor.hasNext() ) {
                BSONObject bson = cursor.getNext();
                ObjectId id = ( ObjectId ) bson.get( "Oid" );
                try {
                    cl.removeLob( id );
                } catch ( BaseException e ) {
                    e.printStackTrace();
                    if ( e.getErrorCode() != -269 )
                        throw e;
                }
            }
        }
    }

    public static int getNumOfLobFromDataNode( String csName, String clname,
            NodeWrapper node ) {
        int count = 0;
        try ( Sequoiadb db = node.connect()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clname );
            DBCursor cursor = cl.listLobs();
            while ( cursor.hasNext() ) {
                cursor.getNext();
                count++;
            }
        }
        return count;
    }

    public static boolean isLobNumInspectInGroup( String csName, String clName,
            String groupName ) throws ReliabilityException {
        GroupMgr groupMgr = GroupMgr.getInstance();
        groupMgr.checkBusiness();
        GroupWrapper group = groupMgr.getGroupByName( groupName );
        int num = getNumOfLobFromDataNode( csName, clName, group.getMaster() );
        for ( NodeWrapper nodeWrapper : group.getNodes() ) {
            int numInNode = getNumOfLobFromDataNode( csName, clName,
                    nodeWrapper );
            if ( num != numInNode ) {
                log.severe( "num:" + String.valueOf( num ) + " numInNode"
                        + String.valueOf( numInNode ) );
                return false;
            }
        }
        return true;
    }

    public static boolean isLobMd5InspectInGroup( String csName, String clName,
            String groupName, List< LobBean > lobs )
            throws ReliabilityException {
        Map< ObjectId, LobBean > targetLob = new HashMap<>();
        Set< String > targetMd5 = new HashSet<>();

        for ( LobBean lob : lobs ) {
            targetLob.put( lob.getId(), lob );
            targetMd5.add( Arrays.toString( lob.getContentMd5() ) );
        }

        GroupMgr groupMgr = GroupMgr.getInstance();
        GroupWrapper groupWrapper = groupMgr.getGroupByName( groupName );

        for ( NodeWrapper node : groupWrapper.getNodes() ) {
            try ( Sequoiadb db = node.connect()) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor cursor = cl.listLobs();
                while ( cursor.hasNext() ) {
                    BSONObject bson = cursor.getNext();
                    ObjectId id = ( ObjectId ) bson.get( "Oid" );
                    byte[] bytes = readLob( cl, id );
                    if ( Arrays.equals( "-269".getBytes(), bytes ) )
                        continue;
                    if ( targetMd5.contains(
                            Arrays.toString( getMd5( bytes ) ) ) == false ) {
                        log.severe( "oid: " + id.toString() + " should be "
                                + targetLob.get( id ) );
                        return false;
                    }
                }
            }
        }
        return true;
    }

    public static boolean isLobsAllDelete( String csName, String clName,
            List< LobBean > lobs ) {
        Set< ObjectId > idSet = new HashSet<>();
        for ( LobBean lob : lobs ) {
            if ( lob.isInSdb() == false )
                idSet.add( lob.getId() );
        }
        return isLobsAllDelete( csName, clName, idSet );
    }

    public static boolean isLobsAllDelete( String csName, String clName,
            Set< ObjectId > lobsId ) {
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.listLobs();
            while ( cursor.hasNext() ) {
                ObjectId id = ( ObjectId ) cursor.getNext().get( "Oid" );
                if ( lobsId.contains( id ) ) {
                    try {
                        cl.openLob( id );
                    } catch ( BaseException e ) {
                        e.printStackTrace();
                        if ( e.getErrorCode() == -296 )
                            continue;

                        log.severe( "OID: " + id.toString() );
                        log.severe( e.getMessage() );
                        return false;
                    }
                    log.severe( "OID: " + id.toString() );
                    return false;
                }
            }
            return true;
        }
    }

    public static boolean isLobsAllCreated( String csName, String clName,
            List< LobBean > lobs ) {
        List< ObjectId > ids = new ArrayList<>();
        for ( LobBean lob : lobs ) {
            if ( lob.isInSdb() )
                ids.add( lob.getId() );
        }
        return isLobsAllCreated2( csName, clName, ids );
    }

    private static boolean isLobsAllCreated2( String csName, String clName,
            List< ObjectId > createdLobIds ) {
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.listLobs();
            Map< ObjectId, String > listLobsMap = new HashMap<>();
            while ( cursor.hasNext() ) {
                ObjectId id = ( ObjectId ) cursor.getNext().get( "Oid" );
                listLobsMap.put( id, "" );
            }
            for ( ObjectId id : createdLobIds ) {
                if ( listLobsMap.containsKey( id ) == false ) {
                    log.severe( id.toString() );
                    return false;
                }
            }
            return true;
        }
    }

    @Deprecated
    public static void throwSkipException( String msg ) {
        System.out.println( msg );
        throw new SkipException( msg );
    }

    @Deprecated
    public static void throwSkipExeWithoutFaultEnv() {
        // throwSkipException("没遇上异常环境");
    }

    public static void createIndex( DBCollection cl, IndexBean index ) {
        cl.createIndex( index.getName(), index.getIndexDef(), index.isUnique(),
                index.isEnforced(), index.getSortBufferSize() );
        index.setCreated( true );
    }

    public static void createIndexs( String csName, String clName,
            List< IndexBean > indexList ) {
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( IndexBean indexBean : indexList ) {
                createIndex( cl, indexBean );
            }
        }
    }

    private static void removeIndex( DBCollection cl, String indexName ) {
        cl.dropIndex( indexName );
    }

    public static void removeIndex( DBCollection cl, IndexBean index ) {
        removeIndex( cl, index.getName() );
        index.setDeleted( true );
    }

    public static void removeIndexes( String csName, String clName,
            List< IndexBean > indexBeans ) {
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( IndexBean indexBean : indexBeans ) {
                removeIndex( cl, indexBean );
            }
        }
    }

    public static boolean isIndexAllCreated( String csName, String clName,
            List< IndexBean > indexBeanList ) {
        boolean flag;
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            flag = isIndexAllCreated( cl, indexBeanList );
        }
        return flag;
    }

    public static boolean isIndexAllCreated( DBCollection cl,
            List< IndexBean > indexBeanList ) {
        Set< String > sdbIndexes = new HashSet<>();
        DBCursor cursor = cl.getIndexes();
        while ( cursor.hasNext() ) {
            Object def = cursor.getNext().get( "IndexDef" );
            String name = ( ( BSONObject ) def ).get( "name" ).toString();
            sdbIndexes.add( name );
        }
        for ( IndexBean indexBean : indexBeanList ) {
            if ( indexBean.isCreated() ) {
                if ( sdbIndexes.contains( indexBean.getName() ) == false ) {
                    log.severe( "can not find index: " + indexBean.getName() );
                    return false;
                }
            }
        }
        return true;
    }

    public static boolean isIndexAllDeleted( String csName, String clName,
            List< IndexBean > indexBeanList ) {
        boolean flag;
        try ( MySequoiadb db = getMySdb()) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            flag = isIndexAllDeleted( cl, indexBeanList );
        }
        return flag;
    }

    public static boolean isIndexAllDeleted( DBCollection cl,
            List< IndexBean > indexBeanList ) {
        Set< String > deletedIndex = new HashSet<>();
        for ( IndexBean indexBean : indexBeanList ) {
            if ( indexBean.isDeleted() )
                deletedIndex.add( indexBean.getName() );
        }
        DBCursor cursor = cl.getIndexes();
        while ( cursor.hasNext() ) {
            Object def = cursor.getNext().get( "IndexDef" );
            String name = ( ( BSONObject ) def ).get( "name" ).toString();
            if ( deletedIndex.contains( name ) ) {
                log.severe( "index was not deleted:" + name );
                return false;
            }
        }
        return true;
    }

    public static List< String > getDataGroupCanUse( String brokenHost,
            int maxTime ) {
        if ( maxTime < 1 )
            throw new IllegalArgumentException( "maxTime must > 1" );

        List< String > groupList = null;
        int count = 0;
        try {
            GroupMgr mgr = GroupMgr.getInstance();
            while ( true ) {
                if ( ++count > maxTime ) {
                    throw new RuntimeException( "切主失败" );
                }
                groupList = getDataGroupCanUseOnce( brokenHost, mgr );
                if ( groupList.size() == 0 ) {
                    for ( String name : MyUtil.getDataGroupNames() ) {
                        try {
                            mgr.getGroupByName( name ).changePrimary( 10 );
                        } catch ( ReliabilityException e ) {
                            log.severe( e.getMessage() );
                        }
                    }
                } else {
                    break;
                }
            }
        } catch ( ReliabilityException e ) {
            log.severe( e.getMessage() );
        }

        if ( groupList == null )
            return new ArrayList<>( 1 );
        else
            return groupList;
    }

    private static List< String > getDataGroupCanUseOnce( String brokenHost,
            GroupMgr mgr ) throws ReliabilityException {
        List< String > groupList = new ArrayList<>( 10 );
        for ( String name : MyUtil.getDataGroupNames() ) {
            String masterHost = mgr.getGroupByName( name ).getMaster()
                    .hostName();
            if ( brokenHost.equals( masterHost ) )
                continue;
            else
                groupList.add( name );
        }
        return groupList;
    }

    public static void checkListCS( Sequoiadb db, String csNameBase,
            int csNum ) {
        // get expect cs name list
        List< BSONObject > expCSNames = new ArrayList< BSONObject >();
        for ( int i = 0; i < csNum; i++ ) {
            BSONObject nameBSON = new BasicBSONObject();
            String csName = csNameBase + "_" + i;
            nameBSON.put( "Name", csName );
            expCSNames.add( nameBSON );
        }

        // get actual cs name list
        DBCursor cursor = db.listCollectionSpaces();
        List< BSONObject > actCSNames = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject result = cursor.getNext();
            String csName = ( String ) result.get( "Name" );
            if ( csName.indexOf( csNameBase ) != -1 ) {
                actCSNames.add( result );
            }
        }
        cursor.close();

        // compare them
        sortByName( actCSNames );
        sortByName( expCSNames );
        if ( !actCSNames.equals( expCSNames ) ) {
            System.out.println( "actCSNames: " + actCSNames );
            System.out.println( "expCSNames: " + expCSNames );
            Assert.fail(
                    "listCollectionSpaces() is not the expected. see details on console" );
        }
    }

    public static void checkListCL( Sequoiadb db, String csName,
            String clNameBase, int clNum ) {
        // get expect cl name list
        List< BSONObject > expCLNames = new ArrayList< BSONObject >();
        for ( int i = 0; i < clNum; i++ ) {
            BSONObject nameBSON = new BasicBSONObject();
            String clFullName = csName + "." + clNameBase + "_" + i;
            nameBSON.put( "Name", clFullName );
            expCLNames.add( nameBSON );
        }

        // get actual cl name list
        DBCursor cursor = db.listCollections();
        List< BSONObject > actCLNames = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject result = cursor.getNext();
            String fullName = ( String ) result.get( "Name" );
            if ( fullName.indexOf( clNameBase ) != -1 ) {
                actCLNames.add( result );
            }
        }
        cursor.close();

        // compare them
        sortByName( actCLNames );
        sortByName( expCLNames );
        if ( !actCLNames.equals( expCLNames ) ) {
            System.out.println( "actCSNames: " + actCLNames );
            System.out.println( "expCSNames: " + expCLNames );
            Assert.fail(
                    "listCollections() is not the expected. see details on console" );
        }
    }

    public static void sortByName( List< BSONObject > list ) {
        Collections.sort( list, new Comparator< BSONObject >() {
            @Override
            public int compare( BSONObject a, BSONObject b ) {
                String aName = ( String ) a.get( "Name" );
                String bName = ( String ) b.get( "Name" );
                return aName.compareTo( bName );
            }
        } );
    }
}
