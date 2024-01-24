package com.sequoias3;

import com.sequoias3.common.InitAdminUserDefine;
import com.sequoias3.config.ServiceInfo;
import com.sequoias3.core.IDGenerator;
import com.sequoias3.core.User;
import com.sequoias3.dao.IDGeneratorDao;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

@Component
@Order(2)
public class InitAdminUserConfig implements ApplicationRunner {
    private static final Logger logger = LoggerFactory.getLogger(InitAdminUserConfig.class);
    @Autowired
    private UserDao userDao;

    @Autowired
    IDGeneratorDao idGeneratorDao;

    @Autowired
    ServiceInfo serviceInfo;

    @Override
    public void run(ApplicationArguments applicationArguments)
            throws Exception {
        try {
            if (userDao.getUserByName(InitAdminUserDefine.ADMIN_NAME) == null) {
                User u = new User();
                u.setUserName(InitAdminUserDefine.ADMIN_NAME);
                u.setUserId(idGeneratorDao.getNewId(IDGenerator.TYPE_USER));
                u.setRole(InitAdminUserDefine.ADMIN_ROLE);
                u.setAccessKeyID(InitAdminUserDefine.ADMIN_ACCESSKEYID);
                u.setSecretAccessKey(InitAdminUserDefine.ADMIN_SECRETACCESSKEY);
                userDao.insertUser(u);
                logger.info("Insert init administrator into db.");
            }
        } catch (S3ServerException e){
            if (e.getError().getErrIndex() != S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                logger.error("Init admin user failed.");
                throw e;
            }
        }
        catch (Exception e) {
            logger.error("Init admin user failed.");
            throw e;
        }
    }
}
