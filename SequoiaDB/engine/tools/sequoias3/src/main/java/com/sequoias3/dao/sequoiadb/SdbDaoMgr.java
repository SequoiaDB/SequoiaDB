package com.sequoias3.dao.sequoiadb;

import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoMgr;
import com.sequoias3.exception.S3ServerException;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository
public class SdbDaoMgr implements DaoMgr {

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Override
    public ConnectionDao getConnectionDao() throws S3ServerException {
        return new SdbConnectionDao(sdbDatasourceWrapper);
    }

    @Override
    public void releaseConnectionDao(ConnectionDao connection){
        SdbConnectionDao sdbConnection = (SdbConnectionDao)connection;
        sdbDatasourceWrapper.releaseSequoiadb(sdbConnection.getConnection());
        sdbConnection.setSdb(null);
    }
}
