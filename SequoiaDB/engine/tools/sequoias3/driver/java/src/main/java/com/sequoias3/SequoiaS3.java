package com.sequoias3;

import com.sequoias3.exception.SequoiaS3ClientException;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.ListRegionsResult;

public interface SequoiaS3 {

    /**
     * Create a region with regionName, all parameters use
     *      *  default values.
     *
     * @param regionName Set the region name
     * @throws SequoiaS3ClientException, SequoiaS3ServerException
     *   If any errors occurred  while processing the request.
     */
    void createRegion(String regionName) throws SequoiaS3ClientException, SequoiaS3ServiceException;

    /**
     * Create a region with CreateRegionRequest.
     *
     * @param request A request with region configuration
     * @throws SequoiaS3ClientException, SequoiaS3ServerException
     *   If any errors occurred  while processing the request.
     */
    void createRegion(CreateRegionRequest request) throws SequoiaS3ClientException, SequoiaS3ServiceException;

    /**
     * Delete a region and the region must be empty before it can be deleted.
     *
     * @param regionName Region name to be delete.
     * @throws SequoiaS3ClientException, SequoiaS3ServerException
     *   If any errors occurred  while processing the request.
     */
    void deleteRegion(String regionName) throws SequoiaS3ClientException, SequoiaS3ServiceException;

    /**
     * Return the region configuration and a list of buckets that
     * stored in the region.
     *
     * @param regionName Region name
     * @return Region configuration and bucket list
     * @throws SequoiaS3ClientException, SequoiaS3ServerException
     *   If any errors occurred  while processing the request.
     */
    GetRegionResult getRegion(String regionName) throws SequoiaS3ClientException, SequoiaS3ServiceException;

    /**
     *  Check whether the region exists.
     *
     * @param regionName Region name
     * @return true or false
     * @throws SequoiaS3ClientException, SequoiaS3ServerException
     *   If any errors occurred  while processing the request.
     */
    boolean headRegion(String regionName) throws SequoiaS3ClientException, SequoiaS3ServiceException;

    /**
     * Returns a list of all regions.
     *
     * @return A list of buckets.
     * @throws SequoiaS3ClientException, SequoiaS3ServerException
     *   If any errors occurred  while processing the request.
     */
    ListRegionsResult listRegions() throws SequoiaS3ClientException, SequoiaS3ServiceException;

    /**
     * Close the client.
     */
    void shutdown();
}
