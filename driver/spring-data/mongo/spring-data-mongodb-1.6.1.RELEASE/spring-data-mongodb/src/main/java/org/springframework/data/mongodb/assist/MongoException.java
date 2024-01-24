package org.springframework.data.mongodb.assist;

import com.sequoiadb.exception.BaseException;

/**
 * Created by tanzhaobo on 2017/9/1.
 */
public class MongoException extends BaseException {
    // TODO: to see which constructor in BaseException is more similar with the current situation.
    // TODO: add proxy method of com.mongo.MongoException in current
    public MongoException(int errCode, String detail) {
        super(errCode, detail);
    }

}
