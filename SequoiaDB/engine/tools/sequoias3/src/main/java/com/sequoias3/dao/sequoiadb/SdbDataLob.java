package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.core.Range;
import com.sequoias3.dao.DataLob;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.ServletOutputStream;
import java.io.IOException;

public class SdbDataLob implements DataLob {
    private static final Logger logger = LoggerFactory.getLogger(SdbDataLob.class);

    private static int ONCE_WRITE_BYTES  = 1024 * 1024;       //1MB

    private Sequoiadb sdb;
    private DBLob dbLob;

    public SdbDataLob(Sequoiadb sdb, DBLob dbLob){
        this.sdb = sdb;
        this.dbLob = dbLob;
    }

    @Override
    public long getSize() {
        return dbLob.getSize();
    }

    @Override
    public void read(ServletOutputStream outputStream, Range range) throws S3ServerException {
        try {
            if (null == range || range.getContentLength() == dbLob.getSize()){
                dbLob.read(outputStream);
                return;
            }

            byte[] buffer    = new byte[ONCE_WRITE_BYTES];
            long readLength  = range.getContentLength();

            dbLob.seek(range.getStart(), DBLob.SDB_LOB_SEEK_SET);
            int size = dbLob.read(buffer, 0,
                    readLength > ONCE_WRITE_BYTES ? ONCE_WRITE_BYTES: (int)readLength);
            while (size > 0) {
                outputStream.write(buffer, 0, size);

                readLength -= size;
                size = dbLob.read(buffer, 0,
                        readLength > ONCE_WRITE_BYTES ? ONCE_WRITE_BYTES : (int)readLength);
            }
        }catch (BaseException e){
            logger.error("read lob failed. error:"+e.getMessage());
            throw e;
        } catch (IOException e){
            logger.error("read lob failed.");
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "IOException. error:"+e.getMessage(), e);
        } catch (Exception e){
            logger.error("read lob failed."+e.getMessage());
            throw e;
        }
    }

    public Sequoiadb getSdb() {
        return sdb;
    }

    public void setSdb(Sequoiadb sdb) {
        this.sdb = sdb;
    }

    public void setDbLob(DBLob dbLob) {
        this.dbLob = dbLob;
    }

    public DBLob getDbLob() {
        return dbLob;
    }
}
