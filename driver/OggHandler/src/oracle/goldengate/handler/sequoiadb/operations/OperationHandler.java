package oracle.goldengate.handler.sequoiadb.operations;

/**
 * Created by chen on 2017/09/20.
 */

import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.DB;
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(OperationHandler.class);
    protected HandlerProperties handlerProperties = null;

    public OperationHandler(HandlerProperties handlerProperties)
    {
        this.handlerProperties = handlerProperties;
    }

    public abstract void process(TableMetaData paramTableMetaData, Op paramOp, DB paramDB)
            throws Exception;

    public abstract BSONObject getFormattedData(TableMetaData paramTableMetaData, Op paramOp, DB paramDB)
            throws Exception; 


    public String getPrimaryKey(TableMetaData tableMetaData, Op op, boolean isBefore)
    {
        return this.handlerProperties.getOggPrimaryKey().getKey(tableMetaData, op, isBefore);
    }
}