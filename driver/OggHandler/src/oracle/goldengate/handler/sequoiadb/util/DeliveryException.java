package oracle.goldengate.handler.sequoiadb.util;

/**
 * Created by chen on 2017/09/20.
 */
public class DeliveryException
        extends RuntimeException
{
    public DeliveryException(String message)
    {
        super(message);
    }

    public DeliveryException(String message, Exception exception)
    {
        super(message, exception);
    }
}
