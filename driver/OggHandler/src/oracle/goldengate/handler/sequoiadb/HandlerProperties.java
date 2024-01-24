package oracle.goldengate.handler.sequoiadb;

import java.util.ArrayList;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import oracle.goldengate.datasource.OGGPrimaryKey;
import oracle.goldengate.datasource.OGGPrimaryKeyImpl;
import oracle.goldengate.handler.sequoiadb.util.DeliveryException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class HandlerProperties
{
    private static final Logger logger = LoggerFactory.getLogger(HandlerProperties.class);

    private String hosts = "localhost:11810";
    private String userName = "";
    private String password = "";

    private boolean bulkWrite = true;
    private int bulksize = 1024;
    private boolean checkMaxRowSizeLimit = false;
    private boolean ignoreMissingColumns = true;
    private boolean changeFieldToLowCase = true;
    private boolean changeTableToLowCase = true;
    private boolean isPrintInfo              = false;
    private String catalogDelimiter = "_";
    public static final String UPDATE_KEY = "$set";

    private SequoiadbDatasource sdbds;
    private ConfigOptions nwOpt = new ConfigOptions();
    private DatasourceOptions dsOpt = new DatasourceOptions();
    private Sequoiadb sdb;

    private OGGPrimaryKey oggPrimaryKey = new OGGPrimaryKeyImpl(":");

    public OGGPrimaryKey getOggPrimaryKey()
    {
        return this.oggPrimaryKey;
    }

    private void printProperties()
    {
        logger.info("hosts = " + this.hosts);
        logger.info("userName = " + this.userName);
        logger.info("password = " + this.password);
        logger.info("bulkWrite = " + this.bulkWrite);
        logger.info("bulksize = " + this.bulksize);
        logger.info("checkMaxRowSizeLimit = " + this.checkMaxRowSizeLimit);
        logger.info("ignoreMissingColumns = " + this.ignoreMissingColumns);
        logger.info("changeFieldToLowCase = " + this.changeFieldToLowCase);
        logger.info("changeTableToLowCase = " + this.changeTableToLowCase);
    }

    public HandlerProperties()
    {
    }

    public void init () {
        
        if (this.getIsPrintInfo())
            this.printProperties();
        
        // 设置网络参数
        nwOpt.setConnectTimeout(500);                      // 建连超时时间为500ms。
        nwOpt.setMaxAutoConnectRetryTime(0);               // 建连失败后重试时间为0ms。

        // 设置连接池参数
        dsOpt.setMaxCount(500);                            // 连接池最多能提供500个连接。
        dsOpt.setDeltaIncCount(5);                        // 每次增加20个连接。
        dsOpt.setMaxIdleCount(20);                         // 连接池空闲时，保留20个连接。
        dsOpt.setKeepAliveTimeout(0);                      // 池中空闲连接存活时间。单位:毫秒。
                                                             // 0表示不关心连接隔多长时间没有收发消息。
        dsOpt.setCheckInterval(60 * 1000);                 // 每隔60秒将连接池中多于
                                                            // MaxIdleCount限定的空闲连接关闭，
                                                            // 并将存活时间过长（连接已停止收发
                                                            // 超过keepAliveTimeout时间）的连接关闭。
        dsOpt.setSyncCoordInterval(0);                     // 向catalog同步coord地址的周期。单位:毫秒>。0表示不同步。

        dsOpt.setValidateConnection(false);                // 连接出池时，是否检测连接的可用性，默认不检测。
        dsOpt.setConnectStrategy(ConnectStrategy.BALANCE); // 默认使用coord地址负载均衡的策略获取连接>。

        ArrayList<String> addrs = new ArrayList<String> ();
        
        String[] hostArr = hosts.split(",");
        for (int i=0; i<hostArr.length; ++i) {
            String host = hostArr[i];
            addrs.add (host);
        }

        this.sdbds =new SequoiadbDatasource(addrs, userName, password, nwOpt, dsOpt);
    }
    
    public void destroy () {
        this.sdbds.close();
    }

    public Sequoiadb getSequoiadbConn()
    {
        try {
            this.sdb = this.sdbds.getConnection();
        } catch (BaseException e) {
            e.printStackTrace();
            throw new DeliveryException("Reference to  SequoiaDB client is invalid");
        } catch (InterruptedException e) {
            e.printStackTrace();
            throw new DeliveryException("Reference to  SequoiaDB client is invalid");
        }
        return this.sdb;
    }
    public void releaseSdbConn (Sequoiadb sdb) {
        this.sdbds.releaseConnection(sdb);
    }

    public boolean getIgnoreMissingColumns()
    {
        return this.ignoreMissingColumns;
    }

    public void setIgnoreMissingColumns(boolean ignoreMissingColumns)
    {
        this.ignoreMissingColumns = ignoreMissingColumns;
    }
    public void setChangeFieldToLowCase(boolean changeFieldToLowCase)
    {
        this.changeFieldToLowCase = changeFieldToLowCase;
    }
    public boolean getChangeFieldToLowCase ()
    {
        return this.changeFieldToLowCase;
    }
    public void setChangeTableToLowCase(boolean changeTableToLowCase)
    {
        this.changeTableToLowCase = changeTableToLowCase;
    }
    public boolean getChangeTableToLowCase ()
    {
        return this.changeTableToLowCase;
    }


    public void setHosts(String hosts)
    {
        this.hosts = hosts;
    }

    public String getHosts()
    {
        return this.hosts;
    }

    public void setUserName(String userName)
    {
        this.userName = userName;
    }

    public void setPassword(String password)
    {
        this.password = password;
    }

    public String getUserName()
    {
        return this.userName;
    }

    public String getPassword()
    {
        return this.password;
    }

    public void setBulkWrite(boolean bulkWrite)
    {
        this.bulkWrite = bulkWrite;
    }

    public boolean getBulkWrite()
    {
        return this.bulkWrite;
    }
    public void setBulkSize(int bulksize)
    {
        this.bulksize = bulksize;
    }
    public int getBulkSize()
    {
        return this.bulksize;
    }

    public void setCheckMaxRowSizeLimit(boolean checkMaxRowSizeLimit)
    {
        this.checkMaxRowSizeLimit = checkMaxRowSizeLimit;
    }

    public boolean getCheckMaxRowSizeLimit()
    {
        return this.checkMaxRowSizeLimit;
    }

    public void setPrintInfo(boolean isPrintinfo)
    {
        this.isPrintInfo = isPrintinfo;
    }
    public boolean getIsPrintInfo()
    {
        return this.isPrintInfo;
    }

}
