/*
 * Copyright 2011-2013 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.core.mapping.event;

import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.ApplicationListener;



/**
 * {@link ApplicationListener} for Sdb mapping events logging the events.
 * 


 */
public class LoggingEventListener extends AbstractSequoiadbEventListener<Object> {

	private static final Logger LOGGER = LoggerFactory.getLogger(LoggingEventListener.class);

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onBeforeConvert(java.lang.Object)
	 */
	@Override
	public void onBeforeConvert(Object source) {
		LOGGER.info("onBeforeConvert: {}", source);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onBeforeSave(java.lang.Object, BSONObject)
	 */
	@Override
	public void onBeforeSave(Object source, BSONObject dbo) {
		LOGGER.info("onBeforeSave: {}, {}", source, dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onAfterSave(java.lang.Object, BSONObject)
	 */
	@Override
	public void onAfterSave(Object source, BSONObject dbo) {
		LOGGER.info("onAfterSave: {}, {}", source, dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onAfterLoad(BSONObject)
	 */
	@Override
	public void onAfterLoad(BSONObject dbo) {
		LOGGER.info("onAfterLoad: {}", dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onAfterConvert(BSONObject, java.lang.Object)
	 */
	@Override
	public void onAfterConvert(BSONObject dbo, Object source) {
		LOGGER.info("onAfterConvert: {}, {}", dbo, source);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onAfterDelete(BSONObject)
	 */
	@Override
	public void onAfterDelete(BSONObject dbo) {
		LOGGER.info("onAfterDelete: {}", dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.event.AbstractSequoiadbEventListener#onBeforeDelete(BSONObject)
	 */
	@Override
	public void onBeforeDelete(BSONObject dbo) {
		LOGGER.info("onBeforeDelete: {}", dbo);
	}
}
