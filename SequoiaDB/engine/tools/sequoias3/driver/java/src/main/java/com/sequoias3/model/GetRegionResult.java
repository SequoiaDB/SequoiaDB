package com.sequoias3.model;

import java.util.List;

public class GetRegionResult {
    private Region region;

    private List<String> Buckets;

    public GetRegionResult(){}

    /**
     *
     * @param region set region configuration
     */
    public void setRegion(Region region) {
        this.region = region;
    }

    /**
     *
     * @return get region configuration
     */
    public Region getRegion() {
        return region;
    }

    /**
     * Get the summary of buckets in the region.
     *
     * @return
     *   A list of buckets.
     */
    public List<String> getBuckets() {
        return Buckets;
    }

    /**
     *
     * @param buckets Set bucket list
     */
    public void setBuckets(List<String> buckets) {
        Buckets = buckets;
    }
}
