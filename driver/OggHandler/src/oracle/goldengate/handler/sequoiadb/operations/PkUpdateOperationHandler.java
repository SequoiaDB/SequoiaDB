package oracle.goldengate.handler.sequoiadb.operations;

/**
 * Created by chen on 2017/09/20.
 */

import java.util.*;

import oracle.goldengate.datasource.adapt.Col;
import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.DB;
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PkUpdateOperationHandler
        extends OperationHandler {
    private static final Logger logger = LoggerFactory.getLogger(UpdateOperationHandler.class);
    private Map<String, String> shardingKeyMap = new HashMap<String, String>();

    public PkUpdateOperationHandler(HandlerProperties handlerProperties) {
        super(handlerProperties);
    }

    public void process(TableMetaData tableMetaData, Op op, DB db)
            throws Exception {

        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("PkUpdateOperationHandler's process");
        }
        BSONObject matcherObj = getMatchFormattedData(op);
        BSONObject _modifierObj = getFormattedData(tableMetaData, op, db);
        BSONObject modifierObj = new BasicBSONObject(this.handlerProperties.UPDATE_KEY, _modifierObj);

        String dbName = db.getDbName(tableMetaData);
        String tableName = db.getTableName(tableMetaData);
        String shardingKeyField = "";
        if (!shardingKeyMap.containsKey(dbName + "." + tableName))
        {
            shardingKeyField = db.getShardingKey(tableMetaData);
            if(this.handlerProperties.getIsPrintInfo()) {
                logger.info("shardingKey = ****" + shardingKeyField + "****");
            }
            if (!shardingKeyField.equals(""))
            {
                shardingKeyMap.put(dbName + "." + tableName, shardingKeyField);
            }
        }
        else
            shardingKeyField = shardingKeyMap.get(dbName + "." + tableName);

        boolean priKeyIsSharingKey = chechPriKeyIsShardingKey(matcherObj, shardingKeyField);

        if (priKeyIsSharingKey)
        {
            if(this.handlerProperties.getIsPrintInfo()) {
                logger.info("PKUpdate Class, modifier field is shardingkey, so will delete old data and insert new one");
            }
            deleteAndInsert(tableMetaData, op, db, matcherObj, _modifierObj);
        }
        else
        {
            if(this.handlerProperties.getIsPrintInfo()) {
                logger.info("PKUpdate Class, modifier field is not shardingkey, so will exec update function");
                logger.info("matcherObj : " + matcherObj.toString());
                logger.info("modifierObj : " + modifierObj.toString());
            }
            db.updateOne(tableMetaData, matcherObj, modifierObj);
        }


//        String beforeKeyValue = getPrimaryKey(tableMetaData, op, true);
//        String afterKeyValue = getPrimaryKey(tableMetaData, op, false);


    }


    private void deleteAndInsert(TableMetaData tableMetaData, Op op, DB db, BSONObject matcherObj, BSONObject _modifierObj)
    {
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("PKUpdate class, deleteAndInsert function");
        }
        BSONObject record = db.queryOne(tableMetaData, matcherObj);

        for (Iterator<String> iterator = _modifierObj.keySet().iterator(); iterator.hasNext();)
        {
            String key = iterator.next();
            if (this.handlerProperties.getChangeFieldToLowCase())
                record.put(key.toLowerCase(), _modifierObj.get(key));
            else
                record.put(key, _modifierObj.get(key));
        }

        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("delete record, matcher = " + matcherObj.toString());
        }
        db.deleteOne(tableMetaData, matcherObj);
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("insert record = " + record.toString());
        }
        if (handlerProperties.getCheckMaxRowSizeLimit()) {
            db.checkMaxRowSizeLimit(record);
        }
        db.insertOne(tableMetaData, record);
    }

    public boolean chechPriKeyIsShardingKey (BSONObject matcherObj, String shardingKeyField)
    {
        boolean priKeyIsSharingKey = false;
        if (matcherObj.containsField(shardingKeyField))
            priKeyIsSharingKey = true;
        return priKeyIsSharingKey;
    }

    public BSONObject getFormattedData(TableMetaData tableMetaData, Op op, DB db) {
        Set<String> foundColumns = new HashSet();
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext(); ) {
            Col c = (Col) localListIterator.next();
            if (c.hasAfterValue()) {
                Object columnValue = DB.getColumnValue(c, false);
                if (this.handlerProperties.getChangeFieldToLowCase())
                    obj.put(c.getOriginalName().toLowerCase(), columnValue);
                else
                    obj.put(c.getOriginalName(), columnValue);

                foundColumns.add(c.getOriginalName().toLowerCase());
            }
        }
        return obj;
    }


    private BSONObject getMatchFormattedData(Op op) {
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext(); ) {
            Col c = (Col) localListIterator.next();
            if (c.getMeta().isKeyCol()) {
                Object columnValue;
                if (c.hasBeforeValue()) {
                    columnValue = DB.getColumnValue(c, true);
                    obj.put(c.getOriginalName().toLowerCase(), columnValue);
                }
            }
        }
        return obj;
    }

}