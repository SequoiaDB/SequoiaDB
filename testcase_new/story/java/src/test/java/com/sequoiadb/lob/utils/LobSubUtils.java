package com.sequoiadb.lob.utils;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class LobSubUtils {

    /**
     * 创建主表并挂载一个子表，默认分区键格式为YYYYMMDD，分区范围默认为min-max
     * 
     * @param db
     * @param csName
     *            集合空间名
     * @param mainCLName
     *            主表名
     * @param subCLName
     *            子表名
     * @return mainCL 返回主表
     */
    public static DBCollection createMainCLAndAttachCL( Sequoiadb db,
            String csName, String mainCLName, String subCLName ) {
        CollectionSpace cs = null;
        if ( db.isCollectionSpaceExist( csName ) ) {
            cs = db.getCollectionSpace( csName );
        } else {
            cs = db.createCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection mainCL = cs.createCollection( mainCLName, options );

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "AutoSplit", true );
        cs.createCollection( subCLName, clOptions );

        BSONObject bound = new BasicBSONObject();
        bound.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        bound.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        mainCL.attachCollection( csName + "." + subCLName, bound );
        return mainCL;
    }

    /**
     * 创建主表并挂载子表
     * 
     * @param db
     * @param csName
     *            集合空间名
     * @param mainCLName
     *            主表名
     * @param subCLNames
     *            子表名列表
     * @param shardingFormat
     *            分区键格式、YYYYMMDD/YYYYMM/YYYY
     * @param beginBound
     *            分区范围开启起始边界值
     * @param scope
     *            分区范围
     * @return mainCL 返回主表
     */
    public static DBCollection createMainCLAndAttachCL( Sequoiadb db,
            String csName, String mainCLName, List< String > subCLNames,
            String shardingFormat, String beginBound, int scope ) {
        switch ( shardingFormat ) {
        case "YYYYMM":
            beginBound = beginBound.substring( 0, 6 );
            break;
        case "YYYY":
            beginBound = beginBound.substring( 0, 4 );
            break;
        }
        CollectionSpace cs = null;
        if ( db.isCollectionSpaceExist( csName ) ) {
            cs = db.getCollectionSpace( csName );
        } else {
            cs = db.createCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", shardingFormat );
        DBCollection mainCL = cs.createCollection( mainCLName, options );

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "AutoSplit", true );
        for ( int i = 0; i < subCLNames.size(); i++ ) {
            String subName = subCLNames.get( i );
            cs.createCollection( subName, clOptions );

            BSONObject bound = new BasicBSONObject();
            bound.put( "LowBound", new BasicBSONObject( "date",
                    Integer.valueOf( beginBound ) + i * scope + "" ) );
            bound.put( "UpBound", new BasicBSONObject( "date",
                    Integer.valueOf( beginBound ) + ( i + 1 ) * scope + "" ) );
            mainCL.attachCollection( csName + "." + subName, bound );
        }

        return mainCL;
    }

    /**
     * 创建lob， 默认插入10个lob，每个lob间隔1天
     * 
     * @param cl
     * @param data
     * @return List<ObjectId> lobIds
     */
    public static List< ObjectId > createAndWriteLob( DBCollection cl,
            byte[] data ) {
        Calendar cal = Calendar.getInstance();
        cal.set( Calendar.DATE, 1 );
        Date date = cal.getTime();
        SimpleDateFormat sf = new SimpleDateFormat( "yyyyMMdd" );
        return createAndWriteLob( cl, data, "YYYYMMDD", 10, 1,
                sf.format( date ) );
    }

    /**
     * 创建lob
     * 
     * @param cl
     *            集合
     * @param data
     *            数据
     * @param format
     *            主表分区键日期格式
     * @param lobNum
     *            创建的lob数
     * @param timeInterval
     *            lob间隔时间
     * @param beginDate
     *            构造lob的时间起始值
     * @return List<ObjectId> lobIds
     */
    public static List< ObjectId > createAndWriteLob( DBCollection cl,
            byte[] data, String format, int lobNum, int timeInterval,
            String beginDate ) {
        List< ObjectId > idList = new ArrayList< ObjectId >();
        int year = Integer.valueOf( beginDate.substring( 0, 4 ) );
        int month = Integer.valueOf( beginDate.substring( 4, 6 ) );
        int day = Integer.valueOf( beginDate.substring( 6, 8 ) );
        for ( int i = 0; i < lobNum; i++ ) {
            Calendar cal = Calendar.getInstance();
            switch ( format ) {
            case "YYYY":
                cal.set( Calendar.YEAR, year + i );
                cal.set( Calendar.MONTH, month - 1 );
                cal.set( Calendar.DATE, day );
                break;
            case "YYYYMM":
                cal.set( Calendar.YEAR, year );
                cal.set( Calendar.MONTH, month + i - 1 );
                cal.set( Calendar.DATE, day );
                break;
            default:
                cal.set( Calendar.YEAR, year );
                cal.set( Calendar.MONTH, month - 1 );
                cal.set( Calendar.DATE, day + i );
                break;
            }
            Date date = cal.getTime();
            ObjectId lodID = cl.createLobID( date );
            DBLob lob = cl.createLob( lodID );
            lob.write( data );
            lob.close();
            idList.add( lodID );
        }
        return idList;
    }

    /**
     * 获取集合中的lob id
     * 
     * @param cl
     * @param matcher
     * @return List<ObjectId> lobIds
     */
    public static List< ObjectId > getLobIDs( DBCollection cl,
            BSONObject matcher ) {
        List< ObjectId > idList = new ArrayList< ObjectId >();
        DBCursor cursor = cl.listLobs( matcher, null,
                new BasicBSONObject( "Oid", 1 ), null, 0, -1 );
        while ( cursor.hasNext() ) {
            idList.add( ( ObjectId ) cursor.getNext().get( "Oid" ) );
        }
        cursor.close();
        return idList;
    }

    /**
     * 检查lob md5是否正确
     * 
     * @param cl
     * @param lobIds
     * @param expData
     * @return booelan
     */
    public static boolean checkLobMD5( DBCollection cl, List< ObjectId > lobIds,
            byte[] expData ) {
        String expMD5 = LobOprUtils.getMd5( expData );
        for ( ObjectId lobId : lobIds ) {
            byte[] data = new byte[ expData.length ];
            DBLob lob = cl.openLob( lobId );
            lob.read( data );
            lob.close();
            String actMD5 = LobOprUtils.getMd5( data );
            if ( !actMD5.equals( expMD5 ) ) {
                throw new BaseException( 0, "check lob: " + lobId
                        + " md5 error, exp: " + expMD5 + ", act: " + actMD5 );
            }
        }
        return true;
    }
}
