package com.sequoias3.service;

import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.ListRegionsResult;

public interface RegionService {

    void putRegion(Region regionCon) throws S3ServerException;

    GetRegionResult getRegion(String regionName) throws S3ServerException;

    ListRegionsResult ListRegions() throws S3ServerException;

    void deleteRegion(String regionName) throws S3ServerException;

    void headRegion(String regionName) throws S3ServerException;
}
