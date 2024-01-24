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


public class TruncateOperationHandler
        extends OperationHandler
{
    private static final Logger logger = LoggerFactory.getLogger(TruncateOperationHandler.class);

    public TruncateOperationHandler(HandlerProperties handlerProperties)
    {
        super(handlerProperties);
    }


    public void process(TableMetaData tableMetaData, Op op, DB db)
            throws Exception
    {
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("TruncateOperationHandler's process");
        }
        String tableName = tableMetaData.getTableName().getOriginalShortName();
        if(this.handlerProperties.getIsPrintInfo()) {
            logger.info("truncate table = " + tableName);
        }
        db.truncate(tableMetaData);
        
    }

    public BSONObject getFormattedData(TableMetaData tableMetaData, Op op, DB db)
    {
        return null;
    }
}
