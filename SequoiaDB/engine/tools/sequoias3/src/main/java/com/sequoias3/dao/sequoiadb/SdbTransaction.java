package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.Transaction;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository
public class SdbTransaction implements Transaction {
    private static final Logger logger = LoggerFactory.getLogger(SdbTransaction.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Override
    public void begin(ConnectionDao connection) throws S3ServerException {
        Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
        try {
            sdb.beginTransaction();
        }catch (Exception e){
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            throw new S3ServerException(S3Error.DAO_TRANSACTION_BEGIN_ERROR, "startTransaction failed", e);
        }
    }

    @Override
    public void commit(ConnectionDao connection) throws S3ServerException {
        SdbConnectionDao sdbConnection = (SdbConnectionDao)connection;
        Sequoiadb sdb = sdbConnection.getConnection();
        if (sdb == null){
            logger.error("commit sdb is null.");
            return;
        }
        try {
            sdb.commit();
        }catch (Exception e){
            throw new S3ServerException(S3Error.DAO_TRANSACTION_COMMIT_FAILED, "commit transaction failed", e);
        }
    }

    @Override
    public void rollback(ConnectionDao connection) {
        SdbConnectionDao sdbConnection = (SdbConnectionDao)connection;
        Sequoiadb sdb = sdbConnection.getConnection();
        if (sdb == null){
            return;
        }
        try {
            sdb.rollback();
        }catch (Exception e){
            logger.error("rollback failed.", e);
        }
    }
}
