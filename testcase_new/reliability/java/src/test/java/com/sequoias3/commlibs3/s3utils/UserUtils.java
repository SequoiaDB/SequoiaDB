package com.sequoias3.commlibs3.s3utils;

import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;

import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestRest;

public class UserUtils extends S3TestBase {
    public final static String ACCESS_KEYS = "AccessKeys";
    public final static String ACCESS_KEY_ID = "AccessKeyID";
    public final static String SECRET_ACCESS_KEY = "SecretAccessKey";
    public final static String AUTHORIZATION = "Authorization";
    public final static String AUTH_VAL_PRE = "Credential=";

    public static String[] createUser( String name, String type )
            throws HttpClientErrorException {
        JSONObject userJson = createUser( name, type,
                S3TestBase.s3AccessKeyId );
        JSONObject subJson = userJson.getJSONObject( ACCESS_KEYS );
        String accessKeyID = subJson.getString( ACCESS_KEY_ID );
        String secretAccessKey = subJson.getString( SECRET_ACCESS_KEY );
        return new String[] { accessKeyID, secretAccessKey };
    }

    public static JSONObject createUser( String name, String type,
            String accessKeyId ) throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=CreateUser&UserName="
                        + name + "&Role=" + type )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( AUTHORIZATION,
                        AUTH_VAL_PRE + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        String body = response.getBody().toString();
        return XML.toJSONObject( body );
    }

    public static String deleteUser( String name )
            throws HttpClientErrorException {
        return deleteUser( name, S3TestBase.s3AccessKeyId, true );
    }

    public static String deleteUser( String name, String accessKeyId )
            throws HttpClientErrorException {
        return deleteUser( name, accessKeyId, true );
    }

    public static String deleteUser( String name, String accessKeyId,
            boolean force ) throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=DeleteUser&UserName=" + name
                        + "&Force=" + force )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( AUTHORIZATION,
                        AUTH_VAL_PRE + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        return response.getHeaders().toString();
    }

    public static JSONObject updateUser( String name, String accessKeyId )
            throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=CreateAccessKey&UserName=" + name )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( AUTHORIZATION,
                        AUTH_VAL_PRE + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        String body = response.getBody().toString();
        return XML.toJSONObject( body );
    }

    public static JSONObject getUser( String name){
        return getUser( name,S3TestBase.s3AccessKeyId );
    }

    public static JSONObject getUser( String name, String accessKeyId )
            throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=GetAccessKey&UserName=" + name )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( AUTHORIZATION,
                        AUTH_VAL_PRE + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        String body = response.getBody().toString();
        return XML.toJSONObject( body );
    }
}
