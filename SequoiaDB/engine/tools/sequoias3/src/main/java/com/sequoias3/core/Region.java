package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

@JacksonXmlRootElement(localName = "RegionConfiguration")
public class Region {
    public static final String REGION_NAME          = "Name";
    public static final String REGION_CREATERTIME   = "CreateTime";
    public static final String DATA_CS_SHARDINGTYPE = "DataCSShardingType";
    public static final String DATA_CL_SHARDINGTYPE = "DataCLShardingType";
    public static final String DATA_CS_RANGE        = "DataCSRange";
    public static final String DATA_DOMAIN          = "DataDomain";
    public static final String DATA_LOBPAGESIZE     = "DataLobPageSize";
    public static final String DATA_REPLSIZE        = "DataReplSize";
    public static final String META_DOMAIN          = "MetaDomain";
    public static final String DATA_RANGE           = "DataRange";
    public static final String DATA_LOCATION        = "DataLocation";
    public static final String META_LOCATION        = "MetaLocation";
    public static final String META_HIS_LOCATION    = "MetaHisLocation";

    public static final String DATA_CS_LOCATION     = "DataCSLocation";
    public static final String DATA_CL_LOCATION     = "DataCLLocation";
    public static final String META_CS_LOCATION     = "MetaCSLocation";
    public static final String META_CL_LOCATION     = "MetaCLLocation";
    public static final String META_HIS_CS_LOCATION = "MetaHisCSLocation";
    public static final String META_HIS_CL_LOCATION = "MetaHisCLLocation";

    @JsonProperty(REGION_NAME)
    private String name;

    @JsonIgnore
    private long createTime;

    @JsonProperty(DATA_CS_SHARDINGTYPE)
    private String dataCSShardingType;

    @JsonProperty(DATA_CL_SHARDINGTYPE)
    private String dataCLShardingType;

    @JsonProperty(DATA_CS_RANGE)
    private Integer dataCSRange;

    @JsonProperty(DATA_DOMAIN)
    private String dataDomain;

    @JsonProperty(DATA_LOBPAGESIZE)
    private Integer dataLobPageSize;

    @JsonProperty(DATA_REPLSIZE)
    private Integer dataReplSize;

    @JsonProperty(META_DOMAIN)
    private String metaDomain;

    @JsonProperty(DATA_LOCATION)
    private String dataLocation;

    @JsonProperty(META_LOCATION)
    private String metaLocation;

    @JsonProperty(META_HIS_LOCATION)
    private String metaHisLocation;

    @JsonIgnore
    private String dataCSLocation;

    @JsonIgnore
    private String dataCLLocation;

    @JsonIgnore
    private String metaCSLocation;

    @JsonIgnore
    private String metaCLLocation;

    @JsonIgnore
    private String metaHisCSLocation;

    @JsonIgnore
    private String metaHisCLLocation;

    public Region(){}

    public Region(Region region){
        this.name = region.getName();
        this.createTime = region.getCreateTime();
        this.dataCSShardingType = region.getDataCSShardingType();
        this.dataCLShardingType = region.getDataCLShardingType();
        this.dataCSRange        = region.getDataCSRange();
        this.dataDomain         = region.getDataDomain();
        this.dataLobPageSize    = region.getDataLobPageSize();
        this.dataReplSize       = region.getDataReplSize();
        this.metaDomain         = region.getMetaDomain();
        this.dataLocation       = region.getDataLocation();
        this.metaLocation       = region.getMetaLocation();
        this.metaHisLocation    = region.getMetaHisLocation();

        this.dataCSLocation     = region.getDataCSLocation();
        this.dataCLLocation     = region.getDataCLLocation();
        this.metaCSLocation     = region.getMetaCSLocation();
        this.metaCLLocation     = region.getMetaCLLocation();
        this.metaHisCSLocation  = region.getMetaHisCSLocation();
        this.metaHisCLLocation  = region.getMetaHisCLLocation();
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setCreateTime(long createTime) {
        this.createTime = createTime;
    }

    public long getCreateTime() {
        return createTime;
    }

    public void setDataCSShardingType(String dataCSShardingType) {
        this.dataCSShardingType = dataCSShardingType;
    }

    public String getDataCSShardingType() {
        return dataCSShardingType;
    }

    public void setDataCLShardingType(String dataCLShardingType) {
        this.dataCLShardingType = dataCLShardingType;
    }

    public String getDataCLShardingType() {
        return dataCLShardingType;
    }

    public void setDataCSRange(Integer dataCSRange) {
        this.dataCSRange = dataCSRange;
    }

    public Integer getDataCSRange() {
        return dataCSRange;
    }

    public void setDataDomain(String dataDomain) {
        this.dataDomain = dataDomain;
    }

    public String getDataDomain() {
        return dataDomain;
    }

    public void setDataLobPageSize(Integer dataLobPageSize) {
        this.dataLobPageSize = dataLobPageSize;
    }

    public Integer getDataLobPageSize() {
        return dataLobPageSize;
    }

    public void setDataReplSize(Integer dataReplSize) {
        this.dataReplSize = dataReplSize;
    }

    public Integer getDataReplSize() {
        return dataReplSize;
    }

    public void setMetaDomain(String metaDomain) {
        this.metaDomain = metaDomain;
    }

    public String getMetaDomain() {
        return metaDomain;
    }

    public void setDataLocation(String dataLocation) {
        this.dataLocation = dataLocation;
    }

    public String getDataLocation() {
        return dataLocation;
    }

    public void setMetaLocation(String metaLocation) {
        this.metaLocation = metaLocation;
    }

    public String getMetaLocation() {
        return metaLocation;
    }

    public void setMetaHisLocation(String metaHisLocation) {
        this.metaHisLocation = metaHisLocation;
    }

    public String getMetaHisLocation() {
        return metaHisLocation;
    }

    public void setDataCSLocation(String dataCSLocation) {
        this.dataCSLocation = dataCSLocation;
    }

    public String getDataCSLocation() {
        return dataCSLocation;
    }

    public void setDataCLLocation(String dataCLLocation) {
        this.dataCLLocation = dataCLLocation;
    }

    public String getDataCLLocation() {
        return dataCLLocation;
    }

    public void setMetaCSLocation(String metaCSLocation) {
        this.metaCSLocation = metaCSLocation;
    }

    public String getMetaCSLocation() {
        return metaCSLocation;
    }

    public void setMetaCLLocation(String metaCLLocation) {
        this.metaCLLocation = metaCLLocation;
    }

    public String getMetaCLLocation() {
        return metaCLLocation;
    }

    public void setMetaHisCSLocation(String metaHisCSLocation) {
        this.metaHisCSLocation = metaHisCSLocation;
    }

    public String getMetaHisCSLocation() {
        return metaHisCSLocation;
    }

    public void setMetaHisCLLocation(String metaHisCLLocation) {
        this.metaHisCLLocation = metaHisCLLocation;
    }

    public String getMetaHisCLLocation() {
        return metaHisCLLocation;
    }

    public BSONObject toBson(){
        BSONObject newRegion = new BasicBSONObject();
        newRegion.put(Region.REGION_NAME, this.name);
        newRegion.put(Region.REGION_CREATERTIME, this.createTime);
        newRegion.put(Region.DATA_CS_SHARDINGTYPE, this.dataCSShardingType);
        newRegion.put(Region.DATA_CL_SHARDINGTYPE, this.dataCLShardingType);
        newRegion.put(Region.DATA_CS_RANGE, this.dataCSRange);
        newRegion.put(Region.DATA_DOMAIN, this.dataDomain);
        newRegion.put(Region.DATA_LOBPAGESIZE, this.dataLobPageSize);
        newRegion.put(Region.DATA_REPLSIZE, this.dataReplSize);
        newRegion.put(Region.META_DOMAIN, this.metaDomain);
        newRegion.put(Region.DATA_LOCATION, this.dataLocation);
        newRegion.put(Region.META_LOCATION, this.metaLocation);
        newRegion.put(Region.META_HIS_LOCATION, this.metaHisLocation);
        newRegion.put(Region.DATA_CS_LOCATION, this.dataCSLocation);
        newRegion.put(Region.DATA_CL_LOCATION, this.dataCLLocation);
        newRegion.put(Region.META_CS_LOCATION, this.metaCSLocation);
        newRegion.put(Region.META_CL_LOCATION, this.metaCLLocation);
        newRegion.put(Region.META_HIS_CS_LOCATION, this.metaHisCSLocation);
        newRegion.put(Region.META_HIS_CL_LOCATION, this.metaHisCLLocation);
        return newRegion;
    }
}
