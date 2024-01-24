package com.sequoiadb.om.plugin.om;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.om.plugin.common.Crypto;
import com.sequoiadb.om.plugin.config.SequoiaSQLConfig;
import com.sequoiadb.util.SdbDecrypt;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.json.JSONArray;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.context.embedded.EmbeddedServletContainerInitializedEvent;
import org.springframework.context.ApplicationListener;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Component;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.web.client.RestTemplate;

import java.util.Collections;
import java.util.Date;

@Component
public class OMClient implements ApplicationListener<EmbeddedServletContainerInitializedEvent> {

    private final Logger logger = LoggerFactory.getLogger(OMClient.class);
    private String svcname;
    @Autowired
    private OMInfo omInfo;

    @Autowired
    private AutoRegister register;

    private Sequoiadb db;

    @Override
    public void onApplicationEvent(EmbeddedServletContainerInitializedEvent event) {
        svcname = Integer.toString(event.getEmbeddedServletContainer().getPort());
    }

    public boolean registerPlugin(SequoiaSQLConfig config) {

        if (config.getIsRegister()) {
            return true;
        }

        String url = "http://" + omInfo.getHostName() + ":" + omInfo.getHttpname();

        String pluginPublicKey = Crypto.randomGeneratePublicKey();

        LinkedMultiValueMap<String, String> para = new LinkedMultiValueMap<String, String>();
        para.put("cmd", Collections.singletonList("register plugin"));
        para.put("Name", Collections.singletonList(config.getName()));
        para.put("HostName", Collections.singletonList(config.getHostName()));
        para.put("ServiceName", Collections.singletonList(svcname));
        para.put("Role", Collections.singletonList(config.getRole()));
        para.put("Type", Collections.singletonList(config.getType()));
        para.put("PublicKey", Collections.singletonList(pluginPublicKey));

        HttpHeaders httpHeaders = new HttpHeaders();
        httpHeaders.setContentType(MediaType.APPLICATION_FORM_URLENCODED);

        HttpEntity<LinkedMultiValueMap<String, String>> httpEntity = new HttpEntity<LinkedMultiValueMap<String, String>>(para, httpHeaders);

        try {
            RestTemplate restTemp = new RestTemplate();
            String response = restTemp.postForObject(url, httpEntity, String.class);
            omInfo.setRegisterTime(new Date().getTime() / 1000);
            JSONArray arr = new JSONArray(response);
            JSONObject result = arr.getJSONObject(0);
            int errno = result.getInt("errno");
            if (errno != 0) {
                logger.error("Failed to register with om svc, detail: " + result.getString("detail"));
            } else {
                omInfo.setSvcname(result.getString("svcname"));
                omInfo.setUser(Crypto.decrypt(pluginPublicKey, result.getString("user")));
                omInfo.setPasswd(Crypto.decrypt(pluginPublicKey, result.getString("passwd")));
                omInfo.setLeaseTime(result.getLong("leaseTime"));
                logger.info("Event: " + config.getType() + " plugin success");
                //logger.info("%s %s %s %d %d\r\n", omSvcname, omUser, omPasswd, registerTime, leaseTime);
                config.setIsRegister(true);
                return true;
            }
        } catch (Exception e) {
            logger.error("Failed to access om svc, detail: " + e);
        }
        return false;
    }

    public synchronized SequoiaSQLNode getSsqlInfo(String clusterName, String businessName) {
        connect();

        SequoiaSQLNode node = new SequoiaSQLNode();

        CollectionSpace cs = db.getCollectionSpace("SYSDEPLOY");
        DBCollection cl = cs.getCollection("SYSCONFIGURE");

        BSONObject queryCondition = new BasicBSONObject();
        queryCondition.put("ClusterName", clusterName);
        queryCondition.put("BusinessName", businessName);

        DBCursor cur = cl.query(queryCondition, null, null, null);
        try {
            if (cur.hasNext()) {
                BSONObject record = cur.getNext();
                node.setHostName((String) record.get("HostName"));
                BasicBSONList configs = (BasicBSONList) record.get("Config");
                if (configs.size() > 0) {
                    BSONObject config = (BSONObject) configs.get(0);
                    node.setSvcName((String) config.get("port"));
                }
            }
        } finally {
            cur.close();
        }

        return node;
    }

    public synchronized NodeAuth getSsqlAccountInfo(String clusterName, String businessName, String defaultUser) {
        connect();

        NodeAuth auth = new NodeAuth();

        CollectionSpace cs = db.getCollectionSpace("SYSDEPLOY");
        DBCollection cl = cs.getCollection("SYSBUSINESSAUTH");

        BSONObject queryCondition = new BasicBSONObject();
        queryCondition.put("BusinessName", businessName);

        DBCursor cur = cl.query(queryCondition, null, null, null);
        try {
            if (cur.hasNext()) {
                BSONObject record = cur.getNext();
                String passwd = (String) record.get("Passwd");

                if (record.containsField("Encryption") && passwd.length() > 0) {
                    Integer encryption = (Integer) record.get("Encryption");

                    if (encryption == 1) {
                        passwd = new SdbDecrypt().decryptPasswd(passwd, null);
                    }
                }

                auth.setUser((String) record.get("User"));
                auth.setPasswd(passwd);
                auth.setDefaultDb((String) record.get("DbName"));
            }
        } finally {
            cur.close();
        }

        if (auth.getUser() == null || auth.getUser().length() == 0) {
            if (defaultUser == null || defaultUser.length() == 0) {
                auth.setUser(getClusterInfo(clusterName));
            } else {
                auth.setUser(defaultUser);
            }
        }
        return auth;
    }

    private String getClusterInfo(String clusterName) {
        CollectionSpace cs = db.getCollectionSpace("SYSDEPLOY");
        DBCollection cl = cs.getCollection("SYSCLUSTER");

        BSONObject queryCondition = new BasicBSONObject();
        queryCondition.put("ClusterName", clusterName);

        String username = null;

        DBCursor cur = cl.query(queryCondition, null, null, null);
        try {
            if (cur.hasNext()) {
                BSONObject record = cur.getNext();
                username = (String) record.get("SdbUser");
            }
        } finally {
            cur.close();
        }

        return username;
    }

    private void connect() throws BaseException {
        if (db == null || db.isValid() == false) {

            if (db != null) {
                try {
                    db.disconnect();
                    db = null;
                } catch (BaseException e) {
                    logger.warn(e.getMessage());
                }
            }

            for (int i = 0; ; ++i) {
                try {
                    String hostName = omInfo.getHostName();
                    String svcname = omInfo.getSvcname();
                    String user = omInfo.getUser();
                    String pwd = omInfo.getPasswd();
                    db = new Sequoiadb(hostName + ":" + svcname, user, pwd);
                    break;
                } catch (BaseException e) {
                    if (e.getErrorCode() == SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode()) {
                        if (i < 5) {
                            register.register(true);
                            continue;
                        } else {
                            logger.warn(e.getMessage());
                            throw e;
                        }
                    } else {
                        logger.warn(e.getMessage());
                        throw e;
                    }
                }
            }
        }
    }

    protected void finalize() {
        if (db != null) {
            try {
                db.disconnect();
                db = null;
            } catch (BaseException e) {
                logger.warn(e.getMessage());
            }
        }
    }

}
