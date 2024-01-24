package oracle.goldengate.handler.sequoiadb;

/**
 * Created by chen on 2017/09/20.
 */
import java.util.*;
import javax.xml.bind.DatatypeConverter;
import com.sequoiadb.base.*;

import com.sequoiadb.util.Helper;
import oracle.goldengate.datasource.adapt.Col;
import oracle.goldengate.datasource.meta.DsType;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.datasource.meta.TableName;
import oracle.goldengate.handler.sequoiadb.util.DeliveryException;
import oracle.goldengate.util.DateTimeHelper;
import org.apache.commons.lang.StringUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.Binary;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DB
{
    public static final int MAX_SIZE_OF_DOCUMENT = 16777216;
    private HandlerProperties handlerProperties;
    private Map<String, List<BSONObject>> bulkObjMap = new HashMap();
    private static final Logger logger = LoggerFactory.getLogger(DB.class);

    public void init(HandlerProperties handlerProperties)
    {
        this.handlerProperties = handlerProperties;
    }

    
    public String getTableName (String originalName) 
    {
        String tableName = "";
        String[] tableNamesArray = StringUtils.split(originalName, '.');
        if (tableNamesArray.length == 3) {
            tableName = tableNamesArray[2];
        }
        else {
            tableName = tableNamesArray[1];
        }
        if (this.handlerProperties.getChangeTableToLowCase())
            tableName = tableName.toLowerCase();
        return tableName;
    }
    
    public String getDbName (String originalName) 
    {
        String dbName = "";

        String[] tableNamesArray = StringUtils.split(originalName, '.');

        dbName = tableNamesArray[0];
        
        if (this.handlerProperties.getChangeTableToLowCase())
            dbName = dbName.toLowerCase();
        return dbName;
        
    }
    
    public String getTableName (TableMetaData  tableMetaData) 
    {
        String tableName = tableMetaData.getTableName().getOriginalShortName();
        if (this.handlerProperties.getChangeTableToLowCase())
            tableName = tableName.toLowerCase();
        return tableName;
    }
    
    public String getDbName (TableMetaData  tableMetaData)
    {
        String dbName = tableMetaData.getTableName().getOriginalSchemaName();
        if (this.handlerProperties.getChangeTableToLowCase())
            dbName = dbName.toLowerCase();
        return dbName;
    }
    
    public void enqueueOperations(TableMetaData tableMetaData, BSONObject obj)
    {
        TableName tableName = tableMetaData.getTableName();
        String originalName = tableName.getOriginalName();
        if (this.bulkObjMap.containsKey(originalName))
        {
            List<BSONObject> list = (List)this.bulkObjMap.get(originalName);
            list.add(obj);
            if (list.size() >= this.handlerProperties.getBulkSize())
            {
                if (this.handlerProperties.getIsPrintInfo())
                {
                    logger.info("bulk insert into " + originalName + ", insert " + list.size() + " records");
                }
                bulkInsert(originalName, list);
                bulkObjMap.remove(originalName);
            }
            else 
            {
                this.bulkObjMap.put (originalName, list);
            }
        }
        else
        {
            List<BSONObject> list = new ArrayList();
            list.add(obj);
            this.bulkObjMap.put(originalName, list);
        }
    }

    public void flushOperations()
            throws Exception
    {
        String originalName = "";
        try
        {
            for (String tableFullName : this.bulkObjMap.keySet())
            {
                originalName = tableFullName;

                List<BSONObject> list = (List)this.bulkObjMap.get(originalName);
                if (this.handlerProperties.getIsPrintInfo())
                {
                    logger.info("bulk insert into " + originalName + ", insert " + list.size() + " records");
                }
                bulkInsert(originalName, list);
            }
            
            for (String tableFullName : this.bulkObjMap.keySet()) 
            {
                originalName = tableFullName;
                this.bulkObjMap.remove (originalName);
            }
        }
        catch (Exception e) {}
        finally {
            this.bulkObjMap = new HashMap<String, List<BSONObject>> ();
        }
    }



    public DBCollection getCollection(Sequoiadb sdb, String dbName, String tableName)
    {
        return sdb.getCollectionSpace(dbName).getCollection(tableName);
    }

    public void updateOne(TableMetaData tableMetaData, BSONObject matcherObj, BSONObject modifierObj)
    {
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        DBCollection collection = getCollection(sdb, dbName, tableName);
        collection.update(matcherObj, modifierObj, null);
        this.handlerProperties.releaseSdbConn(sdb);
    }

    public void deleteOne(TableMetaData tableMetaData, BSONObject obj)
    {
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        DBCollection collection = getCollection(sdb, dbName, tableName);
        collection.delete(obj);
        this.handlerProperties.releaseSdbConn(sdb);
    }

    public String getShardingKey(TableMetaData tableMetaData)
    {
        String shardingKey = "";
        BSONObject matcherObj = new BasicBSONObject();
        BSONObject selectorObj = new BasicBSONObject();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        matcherObj.put("Name", dbName + "." + tableName);
        selectorObj.put("ShardingKey", null);

        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        DBCursor cursor = sdb.getSnapshot(8, matcherObj, selectorObj, null);
        if (cursor.hasNext())
        {
            BSONObject tmpObj = cursor.getNext();
            if (tmpObj.containsField("ShardingKey") && tmpObj.get("ShardingKey") != null)
            {
                BSONObject obj = (BSONObject) cursor.getCurrent().get("ShardingKey");
                for (Iterator<String> iterator = obj.keySet().iterator(); iterator.hasNext();)
                {
                    shardingKey = iterator.next();
                }
            }
        }
        cursor.close();
        this.handlerProperties.releaseSdbConn(sdb);
        return shardingKey;
    }

    public BSONObject queryOne(TableMetaData tableMetaData, BSONObject matcherObj)
    {
        BSONObject obj = null;
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        DBCollection collection = getCollection(sdb, dbName, tableName);

        BSONObject selectorObj = new BasicBSONObject();
        BSONObject tmpObj = new BasicBSONObject();
        tmpObj.put("$include", 0);
        selectorObj.put("_id", tmpObj);
        DBCursor cursor = collection.query(matcherObj, selectorObj, null, null);
        if (cursor.hasNext())
            obj = cursor.getNext();
        cursor.close();
        this.handlerProperties.releaseSdbConn(sdb);
        return obj;
    }


    public void bulkInsert (TableMetaData tableMetaData, List<BSONObject> recordList)
    {
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        DBCollection collection = getCollection(sdb, dbName, tableName);
        
        collection.insert(recordList);
        this.handlerProperties.releaseSdbConn(sdb);
        
    }


    public void bulkInsert (String originalName, List<BSONObject> recordList)
    {
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(originalName);
        String tableName = getTableName(originalName);
        DBCollection collection = getCollection(sdb, dbName, tableName);
        collection.insert(recordList);
        this.handlerProperties.releaseSdbConn(sdb);

    }
    
    public void insertOne(TableMetaData tableMetaData, BSONObject obj)
    {
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        DBCollection collection = getCollection(sdb, dbName, tableName);
        collection.insert(obj);
        this.handlerProperties.releaseSdbConn(sdb);
    }
    
    public void truncate(TableMetaData tableMetaData)
    {
        Sequoiadb sdb = this.handlerProperties.getSequoiadbConn();
        String dbName = getDbName(tableMetaData);
        String tableName = getTableName(tableMetaData);
        DBCollection collection = getCollection(sdb, dbName, tableName);
        collection.truncate();
        this.handlerProperties.releaseSdbConn(sdb);
    }

    public void checkMaxRowSizeLimit(BSONObject obj)
    {
        if (this.handlerProperties.getCheckMaxRowSizeLimit() == true)
        {
            byte[] docBytes = Helper.encodeBSONObj(obj);
            int sizeOfDocument = docBytes.length;
            
            if (sizeOfDocument > MAX_SIZE_OF_DOCUMENT) {
                throw new DeliveryException("Unable to process the current document with size: " + sizeOfDocument + " .The maximum document size allowed by SequoiqDB is " + 16777216);
            }
        }
    }

    public static Object getColumnValue(Col col, boolean beforeValue)
    {
        if (beforeValue)
        {
            if (col.getBefore().isValueNull()) {
                return null;
            }
        }
        else if (col.getAfter().isValueNull()) {
            return null;
        }
        DsType dsType = col.getDataType();
        String stringVal = beforeValue == true ? col.getBeforeValue() : col.getAfterValue();
        byte[] binaryVal = null;
        if (dsType.getGGDataSubType() == DsType.GGSubType.GG_SUBTYPE_BINARY) {
            if (beforeValue)
            {
                if (col.getBefore().hasBinaryValue()) {
                    binaryVal = col.getBefore().getBinary();
                }
            }
            else if (col.getAfter().hasBinaryValue()) {
                binaryVal = col.getAfter().getBinary();
            }
        }
//            logger.info("getColumnValue");
//            logger.info("dsType: " + dsType);
//            logger.info("ggType : " + col.getDataType().getGGDataType());
//            logger.info("ggType : " + col.getDataType().getGGDataSubType());
//            logger.info("String val: " + stringVal);
        if (binaryVal != null)
        {
            logger.info("Binary value: " + DatatypeConverter.printHexBinary(binaryVal));
        }
        return getMappedSequoiadbColumnObject(stringVal, binaryVal, dsType);
    }



    public static Object getMappedSequoiadbColumnObject(String val, byte[] binaryVal, DsType meta)
    {
        Object ret = null;
        switch (meta.getGGDataType())
        {
            case GG_16BIT_S:
            case GG_16BIT_U:
            case GG_32BIT_S:
            case GG_32BIT_U:
                ret = Integer.valueOf(Integer.parseInt(val));
                break;
            case GG_64BIT_S:
                if (meta.getScale() > 0) {
                    ret = Double.valueOf(Double.parseDouble(val));
                } else {
                    ret = Long.valueOf(Long.parseLong(val));
                }
                int precision = (int)meta.getPrecision();
                int scale = meta.getScale();
                if (scale > 30) {
                    scale = 30;
                }

                BSONDecimal tmpVal = new BSONDecimal(val, precision, scale);
                ret = tmpVal;
                break;
            case GG_64BIT_U:
                ret = Double.valueOf(Double.parseDouble(val));
                break;
            case GG_REAL:
            case GG_IEEE_REAL:
            case GG_IEEE_DOUBLE:
            case GG_DEC_U:
            case GG_DEC_LSS:
            case GG_DEC_LSE:
            case GG_DEC_TSS:
            case GG_DEC_TSE:
            case GG_DEC_PACKED:
            case GG_DOUBLE:
            case GG_DOUBLE_F:
            case GG_DOUBLE_V:
                ret = Double.valueOf(Double.parseDouble(val));
                break;
            case GG_DATETIME:
            case GG_DATETIME_V: {
                int point = val.lastIndexOf(".");
//                logger.info("meta jdbctype = " + meta.getJDBCType());
//                logger.info("meta GGDataType  = " + meta.getGGDataType().getName());
//                logger.info("meta toString = " + meta.toString());
//                logger.info("meta GGSubDataType = " + meta.getGGDataSubType().getName());
                // timestmap
                if (point != -1) {
                    Date tmpDate = DateTimeHelper.getDate(val);
                    String incStr = val.substring(point + 1);
                    BSONTimestamp tmpTimestamp = new BSONTimestamp((int)(tmpDate.getTime()/1000), Integer.parseInt(incStr));
                    ret = tmpTimestamp;
                }
                // date
                else {
                    ret = DateTimeHelper.getDate(val);
                }
                break;
            }
            case GG_ASCII_V:
            case GG_ASCII_V_UP:
            case GG_ASCII_F:
            case GG_ASCII_F_UP:
//                logger.info("meta jdbctype = " + meta.getJDBCType());
//                logger.info("meta GGDataType  = " + meta.getGGDataType().getName());
//                logger.info("meta toString = " + meta.toString());
//                logger.info("meta GGSubDataType = " + meta.getGGDataSubType().getName());
                if (meta.getGGDataSubType() == DsType.GGSubType.GG_SUBTYPE_BINARY) {
                    ret = new Binary(binaryVal);
                } else if ((meta.getGGDataSubType() == DsType.GGSubType.GG_SUBTYPE_FLOAT) ||
                        (meta.getGGDataSubType() == DsType.GGSubType.GG_SUBTYPE_FIXED_PREC)) {
                    ret = Double.valueOf(Double.parseDouble(val));
                } else {
                    ret = val;
                }
                break;
            default:

                ret = val;
        }
        return ret;
    }
}