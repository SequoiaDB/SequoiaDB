package oracle.goldengate.handler.sequoiadb;

/**
 * Created by chen on 2017/09/20.
 */


import oracle.goldengate.datasource.AbstractHandler;
import oracle.goldengate.datasource.DsConfiguration;
import oracle.goldengate.datasource.DsEvent;
import oracle.goldengate.datasource.DsOperation;
import oracle.goldengate.datasource.DsTransaction;
import oracle.goldengate.datasource.adapt.Op;
import oracle.goldengate.datasource.meta.DsMetaData;
import oracle.goldengate.datasource.meta.TableMetaData;
import oracle.goldengate.handler.sequoiadb.util.DBOperationFactory;
import oracle.goldengate.handler.sequoiadb.operations.*;
import oracle.goldengate.util.ConfigException;
import oracle.goldengate.datasource.GGDataSource;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SequoiaDBHandler
        extends AbstractHandler
{
    private static final Logger logger = LoggerFactory.getLogger(SequoiaDBHandler.class);
    private final HandlerProperties handlerProperties;
    private final DBOperationFactory dbOperationFactory;
    private DB db = new DB();

    public SequoiaDBHandler()
    {
        this.handlerProperties = new HandlerProperties();
        this.dbOperationFactory = new DBOperationFactory();
    }

    @Override
    public void init(DsConfiguration conf, DsMetaData mData)
    {
        super.init(conf, mData);

        this.dbOperationFactory.init(this.handlerProperties);
        try
        {
            this.handlerProperties.init();
        }
        catch (Exception e)
        {
            throw new ConfigException("Unable to connect to SequoiaDB instance. Configured Server address list ", e);
        }
        setMode("op");

        this.db.init(this.handlerProperties);
    }

    @Override
    public GGDataSource.Status operationAdded(DsEvent e, DsTransaction tx, DsOperation dsOperation)
    {
        GGDataSource.Status status = super.operationAdded(e, tx, dsOperation);
        Op op = new Op(dsOperation, dsOperation.getTableMetaData(), getConfig());
        TableMetaData tableMetaData = dsOperation.getTableMetaData();

        OperationHandler operationHandler = this.dbOperationFactory.getInstance(dsOperation.getOperationType());
        if (operationHandler != null)
        {
            try
            {
                operationHandler.process(tableMetaData, op, this.db);
            }
            catch (Exception e1)
            {
                status = GGDataSource.Status.ABEND;
                logger.error("Unable to process operation.", e1);
            }
        }
        else
        {
            status = GGDataSource.Status.ABEND;
            logger.error("Unable to instantiate operation handler for " + dsOperation.getOperationType().toString());
        }
        return status;
    }

    @Override
    public GGDataSource.Status transactionBegin(DsEvent e, DsTransaction tx)
    {
        return super.transactionBegin(e, tx);
    }

    @Override
    public GGDataSource.Status transactionCommit(DsEvent e, DsTransaction tx)
    {
        GGDataSource.Status status = super.transactionCommit(e, tx);
        if (!this.handlerProperties.getBulkWrite()) {
            return status;
        }
        try
        {
            this.db.flushOperations();
        }
        catch (Exception ex)
        {
            logger.error("Error flushing records ", ex);
            status = GGDataSource.Status.ABEND;
        }
        finally
        {
        }
        return status;
    }

    @Override
    public String reportStatus()
    {
        return "";
    }

    public void destroy()
    {
        super.destroy();
        this.handlerProperties.destroy();
    }


    public void setHosts(String hosts)
    {
        handlerProperties.setHosts(hosts);
    }

    public void setUsername(String userName)
    {
        this.handlerProperties.setUserName(userName);
    }

    public void setPassword(String password)
    {
        this.handlerProperties.setPassword(password);
    }

    public void setBulkWrite(String value)
    {
        this.handlerProperties.setBulkWrite(Boolean.parseBoolean(value));
    }

    public void setChangeFieldToLowCase(String changeFieldToLowCase)  {
        this.handlerProperties.setChangeFieldToLowCase(Boolean.parseBoolean(changeFieldToLowCase));
    }
    public void setChangeTableToLowCase(String changeTableToLowCase)
    {
        this.handlerProperties.setChangeTableToLowCase(Boolean.parseBoolean(changeTableToLowCase));
    }

    public void setIsPrintInfo(String isPrintInfo)
    {
        this.handlerProperties.setPrintInfo(Boolean.parseBoolean(isPrintInfo));
    }

    public void setCheckMaxRowSizeLimit(String value)
    {
        this.handlerProperties.setCheckMaxRowSizeLimit(Boolean.parseBoolean(value));
    }

    public void setIgnoreMissingColumns(String ignoreMissingColumns)
    {
        this.handlerProperties.setIgnoreMissingColumns(Boolean.parseBoolean(ignoreMissingColumns));
    }

    public void setBulkSize(String bulksize)
    {
        handlerProperties.setBulkSize(Integer.parseInt(bulksize));
    }

}
