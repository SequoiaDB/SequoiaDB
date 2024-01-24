package com.sequoiadb.fulltext.utils;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.springframework.core.io.InputStreamResource;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.RestTemplate;

import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName FullTextRest.java
 * @Author luweikang
 * @Date 2019年5月30日
 */
public class FullTextRest {
    private HttpHeaders requestHeaders;
    private String addr = "http://" + SdbTestBase.esHostName + ":"
            + SdbTestBase.esServiceName;
    private String url;
    private HttpMethod requestMethod;
    private HttpEntity< ? > requestEntity;
    private static RestTemplate rest;
    private Class< ? > responseType;
    private Object uriVariables[];
    private String api;
    private Object resquestBody = null;
    private MultiValueMap< Object, Object > param;
    private InputStreamResource resource = null;

    static {
        HttpComponentsClientHttpRequestFactory factory = new HttpComponentsClientHttpRequestFactory();
        factory.setConnectionRequestTimeout( 10000 );
        factory.setConnectTimeout( 10000 );
        factory.setBufferRequestBody( false );
        factory.setReadTimeout( 180000 );
        rest = new RestTemplate( factory );
    }

    public FullTextRest() {
        super();
        this.requestHeaders = new HttpHeaders();
        this.param = new LinkedMultiValueMap<>();
    }

    public FullTextRest reset() {
        this.url = null;
        this.requestHeaders = null;
        return this;
    }

    public FullTextRest setRequestMethod( HttpMethod method ) {
        this.requestMethod = method;
        return this;
    }

    public FullTextRest setRequestBody( Object body ) {
        this.resquestBody = body;
        return this;
    }

    public Object getRequestBody() {
        return this.resquestBody;
    }

    public FullTextRest setApi( String api ) {
        this.api = api;
        return this;
    }

    private String setUrl( String api ) {
        this.url = addr + api;
        return url;
    }

    private HttpHeaders getRequestHeaders() {
        return this.requestHeaders;
    }

    public FullTextRest setResponseType( Class< ? > responseType ) {
        this.responseType = responseType;
        return this;
    }

    public FullTextRest setRequestHeaders( String headerName,
            String headerValue ) {
        requestHeaders.add( headerName, headerValue );
        return this;
    }

    public FullTextRest setParameter( Object key, Object value ) {
        param.set( key, value );
        return this;
    }

    public FullTextRest setUriVariables( Object[] uriVariables ) {
        this.uriVariables = uriVariables;
        return this;
    }

    private String getUrl( String api ) {
        setUrl( api );
        return url;
    }

    public ResponseEntity< ? > exec() throws InterruptedException {
        ResponseEntity< ? > response = null;
        boolean runFlag = false;
        int avoid503 = 0;
        do {
            runFlag = false;
            try {
                if ( null != this.resource ) {
                    requestEntity = new HttpEntity<>( this.resource,
                            this.getRequestHeaders() );
                } else {
                    if ( resquestBody != null ) {
                        requestEntity = new HttpEntity<>( this.getRequestBody(),
                                this.getRequestHeaders() );
                    } else {
                        requestEntity = new HttpEntity<>( this.param,
                                this.getRequestHeaders() );
                    }
                }
                if ( this.uriVariables != null ) {
                    response = rest.exchange( this.getUrl( this.api ),
                            this.requestMethod, this.requestEntity,
                            this.responseType, this.uriVariables );
                } else {
                    response = rest.exchange( this.getUrl( this.api ),
                            this.requestMethod, this.requestEntity,
                            this.responseType );
                }
            } catch ( HttpClientErrorException e ) {
                throw e;
            } catch ( HttpServerErrorException e ) {
                if ( 503 != e.getStatusCode().value() ) {
                    throw e;
                }
                runFlag = true;
                if ( avoid503++ >= 120 ) {
                    throw e;
                }
                Thread.sleep( 1000 );
            } catch ( Exception e ) {
                throw e;
            } finally {
                this.reset();
            }
        } while ( runFlag );
        return response;
    }

    /**
     * 获取elasticsearch端全文索引的总记录数
     * 
     * @param esIndexName
     * @return
     * @throws Exception
     */
    public int getCount( String esIndexName ) throws Exception {
        ResponseEntity< ? > response = null;
        try {
            response = this.setApi( "/" + esIndexName + "/_count" )
                    .setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
        } catch ( Exception e ) {
            // 404是索引不存在的错误，转化该错误用于上层调用判断是否需要重试
            System.err.println( addr + "/" + esIndexName + "/_count" );
            if ( e.getMessage().equals( "404 Not Found" ) ) {
                throw new Exception( "no such index" );
            } else {
                throw e;
            }
        }
        String body = response.getBody().toString();
        BSONObject bodyObj = ( BSONObject ) JSON.parse( body );

        // 减去SDBCOMMITID记录
        return ( int ) bodyObj.get( "count" ) - 1;
    }

    /**
     * 获取elasticsearch端的SDBCOMMIT记录下的逻辑ID值
     * 
     * @param esIndexName
     * @return
     * @throws Exception
     */
    @SuppressWarnings("unchecked")
    public int getCommitID( String esIndexName ) throws Exception {

        int commitID = -1;

        ResponseEntity< ? > response = null;
        try {
            response = this
                    .setApi( "/" + esIndexName + "/_search?q=_id:SDBCOMMIT" )
                    .setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
        } catch ( Exception e ) {
            System.err.println(
                    addr + "/" + esIndexName + "/_search?q=_id:SDBCOMMIT" );
            if ( e.getMessage().equals( "404 Not Found" ) ) {
                throw new Exception( "no such index" );
            } else {
                throw e;
            }
        }
        String body = response.getBody().toString();
        BSONObject bodyObj = ( BSONObject ) JSON.parse( body );

        BSONObject hitss = ( BSONObject ) bodyObj.get( "hits" );
        if ( ( int ) hitss.get( "total" ) == 0 ) {
            throw new Exception( "no such _id=SDBCOMMIT record" );
        }
        BSONObject hits = ( ( List< BSONObject > ) hitss.get( "hits" ) )
                .get( 0 );
        BSONObject source = ( BSONObject ) hits.get( "_source" );
        commitID = ( int ) source.get( "_lid" );

        return commitID;
    }

    /**
     * 获取elasticsearch端的SDBCOMMIT记录下的原始集合逻辑ID值
     * 
     * @param esIndexName
     * @return
     * @throws Exception
     */
    @SuppressWarnings("unchecked")
    public int getCommitCLLID( String esIndexName ) throws Exception {
        int commitCLLID = -1;

        ResponseEntity< ? > response = null;
        try {
            response = this
                    .setApi( "/" + esIndexName + "/_search?q=_id:SDBCOMMIT" )
                    .setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
        } catch ( Exception e ) {
            System.err.println(
                    addr + "/" + esIndexName + "/_search?q=_id:SDBCOMMIT" );
            if ( e.getMessage().equals( "404 Not Found" ) ) {
                throw new Exception( "no such index" );
            } else {
                throw e;
            }
        }
        String body = response.getBody().toString();
        BSONObject bodyObj = ( BSONObject ) JSON.parse( body );
        BSONObject hitss = ( BSONObject ) bodyObj.get( "hits" );
        if ( ( int ) hitss.get( "total" ) == 0 ) {
            throw new Exception( "no such index" );
        }
        BSONObject hits = ( ( List< BSONObject > ) hitss.get( "hits" ) )
                .get( 0 );
        BSONObject source = ( BSONObject ) hits.get( "_source" );
        commitCLLID = ( int ) source.get( "_cllid" );

        return commitCLLID;
    }

    /**
     * 获取elasticsearch端的SDBCOMMIT记录下的全文索引的逻辑ID值
     * 
     * @param esIndexName
     * @return
     * @throws Exception
     */
    @SuppressWarnings("unchecked")
    public int getCommitIDXLID( String esIndexName ) throws Exception {
        int commitIDXLID = -1;

        ResponseEntity< ? > response = null;
        try {
            response = this
                    .setApi( "/" + esIndexName + "/_search?q=_id:SDBCOMMIT" )
                    .setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
        } catch ( Exception e ) {
            System.err.println(
                    addr + "/" + esIndexName + "/_search?q=_id:SDBCOMMIT" );
            if ( e.getMessage().equals( "404 Not Found" ) ) {
                throw new Exception( "no such index" );
            } else {
                throw e;
            }
        }
        String body = response.getBody().toString();
        BSONObject bodyObj = ( BSONObject ) JSON.parse( body );
        BSONObject hitss = ( BSONObject ) bodyObj.get( "hits" );
        if ( ( int ) hitss.get( "total" ) == 0 ) {
            throw new Exception( "no such index" );
        }
        BSONObject hits = ( ( List< BSONObject > ) hitss.get( "hits" ) )
                .get( 0 );
        BSONObject source = ( BSONObject ) hits.get( "_source" );
        commitIDXLID = ( int ) source.get( "_idxlid" );

        return commitIDXLID;
    }

    /**
     * 判断elasticsearch端的全文索引名是否存在，用于检查在创建阶段索引名是否映射到elasticsearch端
     * 
     * @param esIndexName
     * @return
     * @throws Exception
     */
    public boolean isExist( String esIndexName ) throws Exception {
        boolean indexExist = false;
        ResponseEntity< ? > response = null;
        try {
            response = this.setApi( "/" + esIndexName )
                    .setRequestMethod( HttpMethod.GET )
                    .setResponseType( String.class ).exec();
        } catch ( Exception e ) {
            System.err.println( addr + "/" + esIndexName );
            if ( e.getMessage().equals( "404 Not Found" ) ) {
                indexExist = false;
            } else {
                throw e;
            }
        }
        if ( response != null && response.getStatusCodeValue() == 200 ) {
            indexExist = true;
        }

        return indexExist;
    }
}
