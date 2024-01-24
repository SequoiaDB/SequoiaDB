package oracle.goldengate.handler.sequoiadb.operations;

/**
 * Created by chen on 2017/09/20.
 */

import java.util.ListIterator;
import oracle.goldengate.datasource.adapt.Col;
import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.DB;
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class UpdateOperationHandler
        extends OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(UpdateOperationHandler.class);

    public UpdateOperationHandler(HandlerProperties handlerProperties)
    {
        super(handlerProperties);
    }

    
    public void process(TableMetaData tableMetaData, Op op, DB db)
            throws Exception
    {
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("UpdateOperationHandler's process");
        }
        BSONObject matcherObj = getMatchFormattedData (op);
        BSONObject _modifierObj = getFormattedData(tableMetaData, op, db) ;
        BSONObject modifierObj = new BasicBSONObject (this.handlerProperties.UPDATE_KEY, _modifierObj);

        logger.info("matcherObj : " + matcherObj.toString());
        logger.info("modifierObj : " + modifierObj.toString());
        if (handlerProperties.getCheckMaxRowSizeLimit()) {
            db.checkMaxRowSizeLimit(matcherObj);
            db.checkMaxRowSizeLimit(modifierObj);
        }
        
        db.updateOne(tableMetaData, matcherObj, modifierObj);
    }

    public BSONObject getFormattedData(TableMetaData tableMetaData, Op op, DB db)
    {
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext();)
        {
            Col c = (Col)localListIterator.next();
            if (c.hasAfterValue())
            {
                Object columnValue = DB.getColumnValue(c, false);
                if (this.handlerProperties.getChangeFieldToLowCase())
                    obj.put (c.getOriginalName().toLowerCase(), columnValue);
                else
                    obj.put (c.getOriginalName(), columnValue);
            }
        }

        return obj;
    }
    
    private BSONObject getMatchFormattedData (Op op)
    {
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext();)
        {
            Col c = (Col)localListIterator.next();
            if (c.getMeta().isKeyCol())
            {
                Object columnValue;
                if (c.hasBeforeValue()) {
                    columnValue = DB.getColumnValue(c, true);
                    if (this.handlerProperties.getChangeFieldToLowCase())
                        obj.put (c.getOriginalName().toLowerCase(), columnValue);
                    else
                        obj.put (c.getOriginalName(), columnValue);
                } 
            }
        }
        return obj; 
    }
}