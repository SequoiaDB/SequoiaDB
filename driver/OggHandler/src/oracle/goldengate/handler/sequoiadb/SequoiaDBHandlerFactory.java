package oracle.goldengate.handler.sequoiadb;

/**
 * Created by chen on 2017/09/20.
 */
import oracle.goldengate.datasource.DataSourceListener;
import oracle.goldengate.datasource.HandlerFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SequoiaDBHandlerFactory
        extends HandlerFactory
{
    private static final Logger logger = LoggerFactory.getLogger(SequoiaDBHandlerFactory.class);

    public DataSourceListener instantiateHandler()
    {
        return new SequoiaDBHandler();
    }
}
