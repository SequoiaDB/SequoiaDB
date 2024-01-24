package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Repository;

@Repository("SdbBaseOperation")
public class SdbBaseOperation {
    private static final Logger logger = LoggerFactory.getLogger(SdbBaseOperation.class);

    private static int SDB_IXM_CREATING = -387;         //DB error code
    private static int SDB_IXM_COVER_CREATING = -389;   //DB error code

    public void releaseDBCursor(DBCursor cursor) {
        if (null == cursor) {
            return;
        }

        try {
            cursor.close();
        } catch (Exception e) {
            logger.warn("release connection failed:sdb={}" + cursor, e);
        }
    }

    public static int createCS(Sequoiadb sdb, String csName, BSONObject options)
            throws S3ServerException {
        try {
            logger.info("creating cs:csName={}", csName);
            sdb.createCollectionSpace(csName, options);
            logger.info("creating cs success :csName={}",csName);
            return DBParamDefine.CREATE_OK;
        }
        catch (BaseException e) {
            logger.error("creating cs failed: csName={}, db error:{}",csName, e.getErrorCode());
            if (e.getErrorCode() != SDBError.SDB_DMS_CS_EXIST.getErrorCode()) {
                throw new S3ServerException(S3Error.DAO_DB_ERROR,
                        "create cs failed. cs=" + csName, e);
            }
            return DBParamDefine.CREATE_EXIST;
        }
        catch (Exception e) {
            logger.error("creating cs failed: csName={}, error:{}",csName, e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR,
                    "create cs failed. cs=" + csName, e);
        }
    }

    public static int createCL(Sequoiadb sdb, String csName, String clName, BSONObject options)
            throws S3ServerException {
        try {
            logger.info("creating cl:clName={}.{}",csName, clName);
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            cs.createCollection(clName, options);
            logger.info("creating cl success :clName={}.{}",csName, clName);
            return DBParamDefine.CREATE_OK;
        } catch (BaseException e) {
            logger.error("creating cl failed: clName={}.{}, db error:{}",csName, clName, e.getErrorCode());
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new S3ServerException(S3Error.DAO_DB_ERROR,
                        "create cl failed. cs=" + csName + ",cl=" + clName, e);
            } else {
                return DBParamDefine.CREATE_EXIST;
            }
        }
        catch (Exception e) {
            logger.error("creating cl failed: clName={}.{}, error:{}",csName, clName, e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR,
                    "create cl failed. cs=" + csName + ",cl=" + clName, e);
        }
    }

    public static void dropCL(Sequoiadb sdb, String csName, String clName)
            throws S3ServerException{
        try{
            logger.info("drop cl:clName={}.{}",csName, clName);
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            cs.dropCollection(clName);
        }catch (BaseException e) {
            logger.error("drop cl failed: clName={}.{}, db error:{}",csName, clName, e.getErrorCode());
            if (e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                throw new S3ServerException(S3Error.DAO_DB_ERROR,
                        "drop cl failed. cs=" + csName + ",cl=" + clName, e);
            }
        }
        catch (Exception e) {
            logger.error("creating cl failed: clName={}.{}, error:{}",csName, clName, e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR,
                    "create cl failed:cs=" + csName + ",cl=" + clName, e);
        }
    }

    public static void createIndex(Sequoiadb sdb, String csName, String clName,
                                   String indexName, BSONObject key, boolean isUnique,
                                   boolean enforced)throws S3ServerException {
        try {
            logger.info("creating cl index :clName= {}.{}, indexName={}", csName, clName, indexName);
            CollectionSpace cs = sdb.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);
            cl.createIndex(indexName, key, isUnique, enforced);
            logger.info("creating index success :clName={}.{}, indexName= {}",csName, clName, indexName);
        }
        catch (BaseException e) {
            logger.error("creating index failed: clName={}.{}, db error:{}",csName, clName, e.getErrorCode());
            if (e.getErrorCode() != SDBError.SDB_IXM_REDEF.getErrorCode()
                    && e.getErrorCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE.getErrorCode()
                    && e.getErrorCode() != SDB_IXM_CREATING
                    && e.getErrorCode() != SDB_IXM_COVER_CREATING) {
                throw new S3ServerException(S3Error.DAO_DB_ERROR,
                        "create Index failed. cs=" + csName + ",cl=" + clName +
                                ",indexName=" + indexName, e);
            }
        }
        catch (Exception e) {
            logger.error("creating Index failed: clName={}.{}, error:{}",csName, clName, e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR,
                    "create Index failed:cs=" + csName + ",cl=" + clName +
                            ",indexName = " + indexName, e);
        }
    }

    public static void dropCS(Sequoiadb sdb, String csName)throws S3ServerException{
        try{
            logger.info("drop cs. csName:{}", csName);
            sdb.dropCollectionSpace(csName);
        }catch (BaseException e) {
            logger.error("drop cs failed: cs={}, db error:{}",csName, e.getErrorCode());
            if (e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                throw new S3ServerException(S3Error.DAO_DB_ERROR,
                        "drop cs failed. cs=" + csName, e);
            }
        }
        catch (Exception e) {
            logger.error("drop cs failed: cs={}, error:{}",csName, e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR,
                    "drop cs failed. cs=" + csName, e);
        }
    }
}
