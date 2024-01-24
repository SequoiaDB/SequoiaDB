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

public class InsertOperationHandler
        extends OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(InsertOperationHandler.class);

    public InsertOperationHandler(HandlerProperties handlerProperties)
    {
        super(handlerProperties);
    }

    public void process(TableMetaData tableMetaData, Op op, DB db)
            throws Exception
    {
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("InsertOperationHandler's process");
        }
        BSONObject obj = getFormattedData(tableMetaData, op, db);

        if (handlerProperties.getCheckMaxRowSizeLimit()) {
            db.checkMaxRowSizeLimit(obj);
        }

        if (!this.handlerProperties.getBulkWrite()) {
            db.insertOne(tableMetaData, obj);
        } 
        else {
            db.enqueueOperations (tableMetaData, obj);
        }
    }

    @Override
    public BSONObject getFormattedData(TableMetaData tableMetaData, Op op, DB db)
    {
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext();)
        {
            Col c = (Col)localListIterator.next();
            if (!c.isMissing())
            {
                Object columnValue = DB.getColumnValue(c, false);
                if (columnValue != null || !this.handlerProperties.getIgnoreMissingColumns()) {
                    if (this.handlerProperties.getChangeFieldToLowCase())
                        obj.put(c.getOriginalName().toLowerCase(), columnValue);
                    else
                        obj.put(c.getOriginalName(), columnValue);
                }
            }
        }
        return obj;
    }
}
