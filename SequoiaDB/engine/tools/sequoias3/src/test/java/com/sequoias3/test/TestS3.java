package com.sequoias3.test;

import java.util.ArrayList;
import java.util.List;

import org.apache.http.HttpEntity;
import org.apache.http.NameValuePair;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.impl.conn.PoolingHttpClientConnectionManager;
import org.apache.http.message.BasicNameValuePair;
import org.apache.http.util.EntityUtils;

import com.sequoias3.test.client.RestClient;

public class TestS3 {

    private CloseableHttpClient client;

    public TestS3() {
        client = createHttpClient();
    }

    private CloseableHttpClient createHttpClient() {
        PoolingHttpClientConnectionManager connMgr = new PoolingHttpClientConnectionManager();
        connMgr.setMaxTotal(2);
        connMgr.setDefaultMaxPerRoute(2);

        RequestConfig reqConf = RequestConfig.custom().setConnectionRequestTimeout(1).build();

        return HttpClients.custom().setConnectionManager(connMgr).setDefaultRequestConfig(reqConf)
                .build();
    }

    public void test() throws Exception {
        String userName = "u1";
        String role = "r1";
        HttpPost request = new HttpPost("http://192.168.10.76:8002//users/" + userName);

        List<NameValuePair> params = new ArrayList<>(2);
        params.add(new BasicNameValuePair("role", role));
        HttpEntity entity;
        entity = new UrlEncodedFormEntity(params, "utf-8");
        request.setHeader("Authorization", "admin/adminpasswd");
        request.setEntity(entity);
        CloseableHttpResponse resp = RestClient.sendRequest(client, request);
        String respStr = EntityUtils.toString(resp.getEntity(), "utf-8");
        System.out.println(respStr);
    }

    public void testBucket() throws Exception {
        HttpPost request = new HttpPost("http://192.168.10.76:8002/bucketName/xxx");

        CloseableHttpResponse resp = RestClient.sendRequest(client, request);
        String respStr = EntityUtils.toString(resp.getEntity(), "utf-8");
        System.out.println(respStr);
    }

    public static void main(String[] args) throws Exception {
        TestS3 t = new TestS3();
        t.test();
        t.testBucket();
    }
}
