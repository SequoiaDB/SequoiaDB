package com.sequoias3.service.impl;

import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.InitAdminUserDefine;
import com.sequoias3.common.UserParamDefine;
import com.sequoias3.core.IDGenerator;
import com.sequoias3.dao.IDGeneratorDao;
import com.sequoias3.model.AccessKeys;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.User;
import com.sequoias3.dao.BucketDao;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.*;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.UserService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.regex.Pattern;

import static com.sequoias3.utils.IDUtils.getAccessKeyID;
import static com.sequoias3.utils.IDUtils.getSecretKey;

@Service
public class UserServiceImpl implements UserService {
    private static final Logger logger = LoggerFactory.getLogger(UserServiceImpl.class);

    @Autowired
    private UserDao userDao;

    @Autowired
    private BucketDao bucketDao;

    @Autowired
    private BucketService bucketService;

    @Autowired
    IDGeneratorDao idGeneratorDao;

    @Override
    public AccessKeys createUser(String newUserName,
                                 String role) throws S3ServerException {
        int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;

        while (tryTime > 0) {
            tryTime--;
            try {
                //       1.check username & role
                if (!isValidUsername(newUserName)) {
                    throw new S3ServerException(S3Error.USER_CREATE_NAME_INVALID, "Username is invalid. username=" + newUserName);
                }
                String userName = newUserName.toLowerCase();
                if (!isValidRole(role)) {
                    throw new S3ServerException(S3Error.USER_CREATE_ROLE_INVALID, "role is invalid. role=" + role);
                }
                if (null != userDao.getUserByName(userName)) {
                    throw new S3ServerException(S3Error.USER_CREATE_EXIST, "The username is exist. username = " + userName);
                }

                //       3.generate accesskey etc.
                String accessKeyID = getAccessKeyID();
                String secretAccessKey = getSecretKey();

                //       4.set user attribute
                User u = new User();
                u.setUserName(userName);
                u.setUserId(idGeneratorDao.getNewId(IDGenerator.TYPE_USER));
                u.setRole(null == role ? UserParamDefine.ROLE_NORMAL : role);
                u.setAccessKeyID(accessKeyID);
                u.setSecretAccessKey(secretAccessKey);
                userDao.insertUser(u);
                return new AccessKeys(accessKeyID, secretAccessKey);

            }catch (S3ServerException e) {
                logger.warn("Create user failed. ", e);
                if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTime > 0) {
                    continue;
                } else {
                    throw e;
                }
            } catch (Exception e) {
                throw new S3CreateUserException("create user failed:username="+ newUserName, e);
            }
        }
        throw new S3ServerException(S3Error.USER_CREATE_FAILED, "IDs duplicate too times.username=" + newUserName);
    }

    @Override
    public AccessKeys updateUser(String updateUserName) throws S3ServerException {
        try {
            //       1.check username
            String userName = updateUserName.toLowerCase();
            if (null == userDao.getUserByName(userName)) {
                throw new S3ServerException(S3Error.USER_NOT_EXIST, "The username is not exit.username="+ updateUserName);
            }

            //       2.generate new keys
            String accessKeyID = getAccessKeyID();
            if (null != userDao.getUserByAccessKeyID(accessKeyID)){
                accessKeyID = getAccessKeyID();
            }
            String secretAccessKey = getSecretKey();

            userDao.updateUserKeys(userName, accessKeyID, secretAccessKey);

            AccessKeys accessKeys = new AccessKeys(accessKeyID, secretAccessKey);
            return accessKeys;
        } catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3UpdateUserException("update user failed: username="+updateUserName, e);
        }
    }

    @Override
    public AccessKeys getUser(String userName)
            throws S3ServerException {
        try {
            User user = userDao.getUserByName(userName.toLowerCase());
            if (null == user) {
                throw new S3ServerException(S3Error.USER_NOT_EXIST, "The username is not exist.username=" + userName);
            }

            return new AccessKeys(user.getAccessKeyID(), user.getSecretAccessKey());
        } catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.USER_GET_FAILED, "get user failed:username="+userName);
        }
    }

    @Override
    public void deleteUser(String deleteUserName, Boolean forceDelete)
            throws S3ServerException {
        try {
            //       1.check username
            String userName = deleteUserName.toLowerCase();
            User user = userDao.getUserByName(userName);
            if (null == user) {
//                logger.info("The username is not exit. username=" + deleteUserName);
                throw new S3ServerException(S3Error.USER_NOT_EXIST,
                        "The username is not exit. username=" + deleteUserName);
            }

            if (userName.equals(InitAdminUserDefine.ADMIN_NAME)) {
//                logger.info("Init admin user cannot be delete.");
                throw new S3ServerException(S3Error.USER_DELETE_INIT_ADMIN,
                        "Last admin user cannot be delete. username=" + deleteUserName);
            }

            //       2.delete buckets
            List<Bucket> bucketList = bucketDao.getBucketListByOwnerID(user.getUserId());
            if (forceDelete) {
                for (int i = 0; i < bucketList.size(); i++) {
                    try {
                        bucketService.deleteBucketForce(bucketList.get(i));
                    } catch (S3ServerException e) {
                        throw new S3ServerException(e.getError(),
                                "clean bucket failed, bucket=" + bucketList.get(i).getBucketName(), e);
                    } catch (Exception e) {
                        throw new S3ServerException(S3Error.USER_DELETE_CLEAN_FAILED,
                                "clean bucket failed, bucket=" + bucketList.get(i).getBucketName(), e);
                    }
                }
            }else if(bucketList.size() > 0 ){
                throw new S3ServerException(S3Error.USER_DELETE_RELEASE_RESOURCE,
                        "Please delete your bucket before delete user, or delete user force.");
            }

            //       3.delete user
            userDao.deleteUser(userName);
        } catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3DeleteUserException("delete user failed. username=" + deleteUserName, e);
        }
    }

    private Boolean isValidUsername(String userName) {
        if (userName.length() > 64) return false;
        return Pattern.compile("^[a-zA-Z0-9+=_,.@\\-]+$").matcher(userName).matches();
    }

    private Boolean isValidRole(String role) {
        if (null == role) return true;

        if (role.equals(UserParamDefine.ROLE_ADMIN) || role.equals(UserParamDefine.ROLE_NORMAL)) {
            return true;
        } else {
            return false;
        }
    }

}
