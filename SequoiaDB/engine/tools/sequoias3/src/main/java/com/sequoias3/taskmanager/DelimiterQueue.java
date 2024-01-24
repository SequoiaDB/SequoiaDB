package com.sequoias3.taskmanager;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@Component("DelimiterQueue")
public class DelimiterQueue {
    private static final Logger logger = LoggerFactory.getLogger(DelimiterQueue.class);
    private Lock taskLock = new ReentrantLock();

    private LinkedHashSet<String> bucketList = new LinkedHashSet();

    public void addBucketName(String bucketName){
        taskLock.lock();
        try{
            bucketList.add(bucketName);
            logger.info("add deleting delimiter. bucketName={}, after queue.size={}", bucketName, bucketList.size());
        }finally {
            taskLock.unlock();
        }
    }

    public String getBucketName(){
        taskLock.lock();
        try {
            if (bucketList.size() > 0) {
                Iterator it = bucketList.iterator();
                String bucketName;
                if (it.hasNext()){
                    bucketName = (String)(it.next());
                    it.remove();
                }else {
                    return null;
                }

//                String bucketName = bucketList.(0);
//                bucketList.remove(0);
                logger.info("get deleting delimiter. bucketName={}, after queue.size={}", bucketName, bucketList.size());
                return bucketName;
            } else {
                return null;
            }
        }finally {
            taskLock.unlock();
        }
    }
}
