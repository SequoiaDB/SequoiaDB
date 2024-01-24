package com.sequoias3.core;

import org.bson.types.ObjectId;

import java.util.Map;

public class ObjectMeta {
    public static final String META_KEY_NAME            = "Key";
    public static final String META_BUCKET_ID           = "BucketId";
    public static final String META_CS_NAME             = "CSName";
    public static final String META_CL_NAME             = "CLName";
    public static final String META_LOB_ID              = "LobId";
    public static final String META_VERSION_ID          = "VersionId";
    public static final String META_NO_VERSION_FLAG     = "NoVersionFlag";
    public static final String META_LAST_MODIFIED       = "LastModified";
    public static final String META_SIZE                = "Size";
    public static final String META_ETAG                = "Etag";
    public static final String META_DELETE_MARKER       = "DeleteMarker";
    public static final String META_CONTENT_TYPE        = "ContentType";
    public static final String META_CONTENT_ENCODING    = "ContentEncoding";
    public static final String META_CACHE_CONTROL       = "CacheControl";
    public static final String META_CONTENT_DISPOSITION = "ContentDisposition";
    public static final String META_EXPIRES             = "Expires";
    public static final String META_CONTENT_LANGUAGE    = "ContentLanguage";
    public static final String META_LIST                = "MetaList";
    public static final String META_PARENTID1           = "ParentId1";
    public static final String META_PARENTID2           = "ParentId2";
    public static final String META_ACLID               = "AclID";

    public static final String NULL_VERSION_ID          = "null";

    public static final String INDEX_CUR_KEY            = ObjectMeta.META_BUCKET_ID + "_" + ObjectMeta.META_KEY_NAME;
    public static final String INDEX_HIS_KEY            = ObjectMeta.META_BUCKET_ID + "_" + ObjectMeta.META_KEY_NAME + "_" + ObjectMeta.META_VERSION_ID;
    public static final String INDEX_CUR_PARENTID1      = ObjectMeta.META_BUCKET_ID + "_" + ObjectMeta.META_KEY_NAME + "_" + ObjectMeta.META_PARENTID1;
    public static final String INDEX_CUR_PARENTID2      = ObjectMeta.META_BUCKET_ID + "_" + ObjectMeta.META_KEY_NAME + "_" + ObjectMeta.META_PARENTID2;

    private String key;
    private long bucketId;
    private String csName;
    private String clName;
    private ObjectId lobId;
    private long versionId;
    private Boolean noVersionFlag;
    private long lastModified;
    private long size;
    private String eTag;
    private Boolean deleteMarker = false;
    private String contentEncoding;
    private String contentType;
    private String cacheControl;
    private String contentDisposition;
    private String expires;
    private String contentLanguage;
    private Map<String, String> metaList;
    private Long parentId1;
    private Long parentId2;
    private Long aclId;

    public long getBucketId() {
        return bucketId;
    }

    public void setBucketId(long bucketId) {
        this.bucketId = bucketId;
    }

    public String getKey() {
        return key;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public void setCsName(String csName) {
        this.csName = csName;
    }

    public String getCsName() {
        return csName;
    }

    public void setClName(String clName) {
        this.clName = clName;
    }

    public String getClName() {
        return clName;
    }

    public ObjectId getLobId() {
        return lobId;
    }

    public void setLobId(ObjectId lobId) {
        this.lobId = lobId;
    }

    public long getVersionId() {
        return versionId;
    }

    public void setVersionId(long versionId) {
        this.versionId = versionId;
    }

    public Boolean getNoVersionFlag() {
        return noVersionFlag;
    }

    public void setNoVersionFlag(Boolean noVersionFlag) {
        this.noVersionFlag = noVersionFlag;
    }

    public void setLastModified(long lastModifiedTime) {
        this.lastModified = lastModifiedTime;
    }

    public long getLastModified() {
        return lastModified;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public long getSize() {
        return size;
    }

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public String geteTag() {
        return eTag;
    }

    public void setDeleteMarker(Boolean deleteMarker) {
        this.deleteMarker = deleteMarker;
    }

    public Boolean getDeleteMarker(){
        return deleteMarker;
    }

    public void setCacheControl(String cacheControl) {
        this.cacheControl = cacheControl;
    }

    public String getCacheControl() {
        return cacheControl;
    }

    public void setContentDisposition(String contentDisposition) {
        this.contentDisposition = contentDisposition;
    }

    public String getContentDisposition() {
        return contentDisposition;
    }

    public void setContentEncoding(String contentEncoding) {
        this.contentEncoding = contentEncoding;
    }

    public String getContentEncoding() {
        return contentEncoding;
    }

    public void setContentType(String contentType) {
        this.contentType = contentType;
    }

    public String getContentType() {
        return contentType;
    }

    public void setExpires(String expires) {
        this.expires = expires;
    }

    public String getExpires() {
        return expires;
    }

    public void setContentLanguage(String contentLanguage) {
        this.contentLanguage = contentLanguage;
    }

    public String getContentLanguage() {
        return contentLanguage;
    }

    public void setMetaList(Map<String, String> metaList) {
        this.metaList = metaList;
    }

    public Map<String, String> getMetaList() {
        return metaList;
    }

    public void setParentId1(Long parentId1) {
        this.parentId1 = parentId1;
    }

    public Long getParentId1() {
        return parentId1;
    }

    public void setParentId2(Long parentId2) {
        this.parentId2 = parentId2;
    }

    public Long getParentId2() {
        return parentId2;
    }

    public void setAclId(Long aclId) {
        this.aclId = aclId;
    }

    public Long getAclId() {
        return aclId;
    }
}
