package com.sequoias3.dao;

import com.sequoias3.core.DataAttr;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;
import org.bson.types.ObjectId;

import java.io.InputStream;

public interface DataDao {
    DataAttr insertObjectData(String csName, String clName,
                              InputStream data, Region region) throws S3ServerException;

    DataAttr insertObjectData(String csName, String clName, InputStream data,
                              Region region, ObjectId lobId, long offset, long length) throws S3ServerException;

    DataAttr createNewData(String csName, String clName, Region region)
            throws S3ServerException;

    DataAttr copyObjectData(String destCSName, String destCLName, ObjectId destLobId,
                            long destoffset, String sourceCSName, String sourceCLName,
                            ObjectId sourceLobId, long sourceOffset, long readSize)
            throws S3ServerException;

    DataLob getDataLobForRead(String csName, String clName, ObjectId lobId) throws S3ServerException;

    void releaseDataLob(DataLob dataLob);

    void deleteObjectDataByLobId(ConnectionDao connectionDao, String csName,
                                 String clName, ObjectId lobId) throws S3ServerException;

    void completeDataLobWithOffset(String csName, String clName, ObjectId lobId,
                                   long writeOffset) throws S3ServerException;
}
