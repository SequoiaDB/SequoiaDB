package com.sequoias3;

public class SequoiaS3ClientBuilder {
    private String accessKeyId;
    private String secretKeyId;
    private String endpoint;
    private int connectionRequestTimeout;
    private int connectTimeout;
    private int readTimeout;
    private boolean requestBody;

    private SequoiaS3ClientBuilder() {
        connectionRequestTimeout = 10000;
        connectTimeout = 10000;
        readTimeout = 10000;
        requestBody = false;
    }

    /**
     *
     * @return Create new instance of builder with all defaults set.
     */
    public static SequoiaS3ClientBuilder standard(){
        return new SequoiaS3ClientBuilder();
    }

    /**
     *
     * @param endpoint
     *   Sets the service endpoint. If not specified, client build fails.
     * @return SequoiaS3ClientBuilder
     */
    public SequoiaS3ClientBuilder withEndpoint(String endpoint){
        this.endpoint = endpoint;
        return this;
    }

    /**
     * Set accessKey and secretKey. If not specified , client build fails.
     *
     * @param accessKey Set accessKey.
     * @param secretKey Set secreteKey.
     * @return SequoiaS3ClientBuilder
     */
    public SequoiaS3ClientBuilder withAccessKeys(String accessKey,
                                                 String secretKey){
        this.accessKeyId = accessKey;
        this.secretKeyId = secretKey;
        return this;
    }

    /**
     *
     * @param connectionRequestTimeout
     *  Sets the amount of time to wait (in milliseconds) when requesting
     *  a connection from the connection manager.
     * @return SequoiaS3ClientBuilder
     */
    public SequoiaS3ClientBuilder withConnectionRequestTimeout(int connectionRequestTimeout) {
        this.connectionRequestTimeout = connectionRequestTimeout;
        return this;
    }

    /**
     *
     * @param connectTimeout
     *   Sets the amount of time to wait (in milliseconds) when initially
     *   establishing a connection before giving up and timing out.
     *
     * @return SequoiaS3ClientBuilder
     */
    public SequoiaS3ClientBuilder withConnectTimeout(int connectTimeout){
        this.connectTimeout = connectTimeout;
        return this;
    }

    /**
     *
     * @param socketTimeout
     *   The amount of time to wait (in milliseconds) for data to be transfered
     *   over an established open connection before the connection is timed out.
     *
     * @return SequoiaS3ClientBuilder
     */
    public SequoiaS3ClientBuilder withSocketTimeout(int socketTimeout){
        this.readTimeout = socketTimeout;
        return this;
    }

    String getAccessKeyId() {
        return accessKeyId;
    }

    String getEndpoint() {
        return endpoint;
    }

    String getSecretKeyId() {
        return secretKeyId;
    }

    int getConnectionRequestTimeout() {
        return connectionRequestTimeout;
    }

    int getConnectTimeout() {
        return connectTimeout;
    }

    int getReadTimeout() {
        return readTimeout;
    }

    boolean isRequestBody() {
        return requestBody;
    }

    /**
     *
     * @return Create a SequoiaS3Client
     */
    public SequoiaS3Client build() {
        if (this.endpoint == null){
            throw new IllegalArgumentException("Need endpoint.");
        }
        if (this.accessKeyId == null || this.secretKeyId == null){
            throw new IllegalArgumentException("Need keys.");
        }
        return new SequoiaS3Client(this);
    }
}
