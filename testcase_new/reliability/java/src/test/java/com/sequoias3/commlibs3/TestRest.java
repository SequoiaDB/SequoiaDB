package com.sequoias3.commlibs3;

import org.springframework.core.io.InputStreamResource;
import org.springframework.http.*;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.RestTemplate;

import java.io.InputStream;

public class TestRest extends S3TestBase {
    private HttpHeaders requestHeaders;
    private String addr = "http://" + S3TestBase.s3HostName + ":"
            + S3TestBase.s3Port;
    private String url = "";
    private HttpMethod requestMethod;
    private HttpEntity< ? > requestEntity;
    private Class< ? > responseType;
    private Object uriVariables[];
    private MultiValueMap< Object, Object > param;
    private Object resquestBody = null;
    private static RestTemplate rest;
    private String api;
    private InputStreamResource resource = null;

    static {
        HttpComponentsClientHttpRequestFactory factory = new HttpComponentsClientHttpRequestFactory();
        factory.setConnectionRequestTimeout( 10000 );
        factory.setConnectTimeout( 60000 );
        factory.setBufferRequestBody( false );
        factory.setReadTimeout( 180000 );
        rest = new RestTemplate( factory );
    }

    public TestRest() {
        super();
        this.requestHeaders = new HttpHeaders();
        MediaType type = MediaType.parseMediaType( "text/xml;charset=UTF-8" );
        requestHeaders.setContentType( type );
        this.param = new LinkedMultiValueMap<>();
    }

    public TestRest( MediaType type ) {
        super();
        this.requestHeaders = new HttpHeaders();
        requestHeaders.setContentType( type );
    }

    public TestRest reset() {
        this.url = null;
        this.requestMethod = null;
        this.requestEntity = null;
        this.uriVariables = null;
        this.resource = null;
        this.requestHeaders.clear();
        this.param = new LinkedMultiValueMap<>();
        return this;
    }

    public TestRest setRequestMethod( HttpMethod method ) {
        this.requestMethod = method;
        return this;
    }

    public TestRest setRequestBody( Object body ) {
        this.resquestBody = body;
        return this;
    }

    public Object getRequestBody() {
        return this.resquestBody;
    }

    public TestRest setInputStream( InputStream is ) {
        this.resource = new InputStreamResource( is );
        return this;
    }

    public TestRest setApi( String api ) {
        this.api = api;
        return this;
    }

    private String setUrl( String api ) {
        this.url = addr + api;
        return url;
    }

    public TestRest setRequestHeaders( String headerName, String headerValue ) {
        requestHeaders.add( headerName, headerValue );
        return this;
    }

    public TestRest setParameter( Object key, Object value ) {
        param.set( key, value );
        return this;
    }

    public TestRest setUriVariables( Object[] uriVariables ) {
        this.uriVariables = uriVariables;
        return this;
    }

    public TestRest setResponseType( Class< ? > responseType ) {
        this.responseType = responseType;
        return this;
    }

    private HttpHeaders getRequestHeaders() {
        return this.requestHeaders;
    }

    public void setRequestHeaders( HttpHeaders requestHeaders ) {
        this.requestHeaders = requestHeaders;
    }

    private String getUrl( String api ) {
        setUrl( api );
        return url;
    }

    public ResponseEntity< ? > exec() {
        ResponseEntity< ? > response = null;
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
            throw e;
        } catch ( Exception e ) {
            throw e;
        } finally {
            this.reset();
        }
        return response;
    }
}
