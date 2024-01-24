/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.flink.common.util;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.TimeUnit;

/**
 * RetryUtil is to accept the specified Task and retry according to the
 * given retry times and retry duration.
 */
public class RetryUtil {

    private static final Logger LOG = LoggerFactory.getLogger(RetryUtil.class);

    /**
     * max retry times
     */
    public static final int DEFAULT_MAX_RETRY_TIMES = 3;

    /**
     * default retry duration (ms)
     */
    public static final int DEFAULT_RETRY_DURATION = 1000;

    /**
     * Retry the given task {@link RetryContent} by maxRetryTimes and duration
     *
     * @param content given task need to retry
     * @param maxRetryTimes max retry times
     * @param duration wait time after each retry
     * @param throwsIfFailed whether throws exception when all retries are failed
     * @return
     * @param <T>
     */
    public static <T> T retryWhenRuntimeException(
            RetryContent<T> content, long maxRetryTimes, long duration, boolean throwsIfFailed) {
        int times = 0;
        while (times < maxRetryTimes) {
            try {
                return content.retry();
            } catch (RuntimeException ex) {
                ++times;
                try {
                    TimeUnit.MILLISECONDS.sleep(duration);
                } catch (InterruptedException ignored) {}

                LOG.warn("{}, retry {} time(s), duration {} ms.",
                        ex.getMessage(),
                        times,
                        duration);

                if (times == maxRetryTimes && throwsIfFailed) {
                    throw ex;
                }
            }
        }
        return null;
    }

    @FunctionalInterface
    public interface RetryContent<T> {
        T retry();
    }

}
