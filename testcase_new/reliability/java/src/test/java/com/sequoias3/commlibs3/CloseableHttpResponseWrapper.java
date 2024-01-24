package com.sequoias3.commlibs3;

import java.io.IOException;
import java.util.Locale;

import org.apache.http.Header;
import org.apache.http.HeaderIterator;
import org.apache.http.HttpEntity;
import org.apache.http.ProtocolVersion;
import org.apache.http.StatusLine;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.params.HttpParams;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@SuppressWarnings("deprecation")
public class CloseableHttpResponseWrapper implements CloseableHttpResponse {
    private static final Logger logger = LoggerFactory
            .getLogger( CloseableHttpResponseWrapper.class );
    private CloseableHttpResponse resp;
    private CloseableHttpClient tmpHttpClient;

    public CloseableHttpResponseWrapper( CloseableHttpResponse resp,
            CloseableHttpClient tmpClient ) {
        this.resp = resp;
        this.tmpHttpClient = tmpClient;
    }

    @Override
    public StatusLine getStatusLine() {
        return resp.getStatusLine();
    }

    @Override
    public void setStatusLine( StatusLine statusline ) {
        resp.setStatusLine( statusline );
    }

    @Override
    public void setStatusLine( ProtocolVersion ver, int code ) {
        resp.setStatusLine( ver, code );
    }

    @Override
    public void setStatusLine( ProtocolVersion ver, int code, String reason ) {
        resp.setStatusLine( ver, code, reason );
    }

    @Override
    public void setStatusCode( int code ) throws IllegalStateException {
        resp.setStatusCode( code );
    }

    @Override
    public void setReasonPhrase( String reason ) throws IllegalStateException {
        resp.setReasonPhrase( reason );
    }

    @Override
    public HttpEntity getEntity() {
        return resp.getEntity();
    }

    @Override
    public void setEntity( HttpEntity entity ) {
        resp.setEntity( entity );
    }

    @Override
    public Locale getLocale() {
        return resp.getLocale();
    }

    @Override
    public void setLocale( Locale loc ) {
        resp.setLocale( loc );
    }

    @Override
    public ProtocolVersion getProtocolVersion() {
        return resp.getProtocolVersion();
    }

    @Override
    public boolean containsHeader( String name ) {
        return resp.containsHeader( name );
    }

    @Override
    public Header[] getHeaders( String name ) {
        return resp.getHeaders( name );
    }

    @Override
    public Header getFirstHeader( String name ) {
        return resp.getFirstHeader( name );
    }

    @Override
    public Header getLastHeader( String name ) {
        return resp.getLastHeader( name );
    }

    @Override
    public Header[] getAllHeaders() {
        return getAllHeaders();
    }

    @Override
    public void addHeader( Header header ) {
        resp.addHeader( header );
    }

    @Override
    public void addHeader( String name, String value ) {
        resp.addHeader( name, value );

    }

    @Override
    public void setHeader( Header header ) {
        resp.setHeader( header );
    }

    @Override
    public void setHeader( String name, String value ) {
        resp.setHeader( name, value );
    }

    @Override
    public void setHeaders( Header[] headers ) {
        resp.setHeaders( headers );
    }

    @Override
    public void removeHeader( Header header ) {
        resp.removeHeader( header );
    }

    @Override
    public void removeHeaders( String name ) {
        resp.removeHeaders( name );
    }

    @Override
    public HeaderIterator headerIterator() {
        return resp.headerIterator();
    }

    @Override
    public HeaderIterator headerIterator( String name ) {
        return resp.headerIterator( name );
    }

    @Override
    public HttpParams getParams() {
        return resp.getParams();
    }

    @Override
    public void setParams( HttpParams params ) {
        resp.setParams( params );
    }

    @Override
    public void close() throws IOException {
        try {
            resp.close();
        } catch ( Exception e ) {
            logger.warn( "close response failed", e );
        }

        tmpHttpClient.close();
        logger.debug( "close tempHttpClient:" + tmpHttpClient );
    }
}