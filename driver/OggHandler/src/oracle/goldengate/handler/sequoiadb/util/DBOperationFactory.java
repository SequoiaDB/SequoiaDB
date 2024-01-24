package oracle.goldengate.handler.sequoiadb.util;

/**
 * Created by chen on 2017/09/20.
 */
import oracle.goldengate.handler.sequoiadb.HandlerProperties;
import oracle.goldengate.datasource.DsOperation;
import oracle.goldengate.handler.sequoiadb.operations.*;
import oracle.goldengate.util.ConfigException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DBOperationFactory
{
    private static final Logger logger = LoggerFactory.getLogger(DBOperationFactory.class);
    public OperationHandler insertOperationHandler;
    public OperationHandler updateOperationHandler;
    public OperationHandler deleteOperationHandler;
    public OperationHandler pkUpdateOperationHandler;
    public OperationHandler truncateOperationHandler;

    public void init(HandlerProperties handlerProperties)
    {
        this.insertOperationHandler = new InsertOperationHandler(handlerProperties);
        this.updateOperationHandler = new UpdateOperationHandler(handlerProperties);
        this.deleteOperationHandler = new DeleteOperationHandler(handlerProperties);
        this.pkUpdateOperationHandler = new PkUpdateOperationHandler(handlerProperties);
        this.truncateOperationHandler = new TruncateOperationHandler(handlerProperties);
    }

    public OperationHandler getInstance(DsOperation.OpType opType)
    {
        if (opType.isInsert()) {
            return this.insertOperationHandler;
        }
        if (opType.isPkUpdate()) {
            return this.pkUpdateOperationHandler;
        }
        if (opType.isUpdate()) {
            return this.updateOperationHandler;
        }
        if (opType.isDelete()) {
            return this.deleteOperationHandler;
        }
        if (opType.isTruncate())
        {
            return this.truncateOperationHandler;
        }
        return null;
    }
}
