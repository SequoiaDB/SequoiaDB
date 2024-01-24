package oracle.goldengate.handler.sequoiadb.operations;

/**
 * Created by chen on 2017/09/20.
 */

import java.util.HashMap;
import java.util.ListIterator;
import java.util.Map;
import oracle.goldengate.datasource.adapt.Col;
import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.DB;
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DeleteOperationHandler
        extends OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(DeleteOperationHandler.class);
    private String tableName;

    public DeleteOperationHandler(HandlerProperties handlerProperties)
    {
        super(handlerProperties);
    }

    public void process(TableMetaData tableMetaData, Op op, DB db)
            throws Exception
    {
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("DeleteOperationHandler's process");
        }
        this.tableName = tableMetaData.getTableName().getOriginalShortName();
        BSONObject obj = getFormattedData(tableMetaData, op, db);
        if (handlerProperties.getCheckMaxRowSizeLimit()) {
            db.checkMaxRowSizeLimit(obj);
        }
        db.deleteOne(tableMetaData, obj);
    }

    public BSONObject getFormattedData(TableMetaData tableMetaData, Op op, DB db)
    {
        Map<String, Object> map = new HashMap();
        BSONObject obj = new BasicBSONObject();
        for (ListIterator localListIterator = op.iterator(); localListIterator.hasNext();)
        {
            Col c = (Col)localListIterator.next();
            if (c.getMeta().isKeyCol())
            {
                Object columnValue = DB.getColumnValue(c, true);
//                keyDocument.append(c.getOriginalName(), columnValue);
                if (this.handlerProperties.getChangeFieldToLowCase())
                    obj.put (c.getOriginalName().toLowerCase(), columnValue);
                else
                    obj.put (c.getOriginalName(), columnValue);
                    
            }
        }
        return obj;
    }
}
