package org.springframework.data.mongodb.assist;

import com.sequoiadb.exception.BaseException;

/**
 * Created by tanzhaobo on 2017/9/1.
 */
public class MongoException extends BaseException {
    public MongoException(int errCode, String detail) {
        super(errCode, detail);
    }

}
