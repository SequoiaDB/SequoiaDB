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
package org.springframework.data.mongodb.core.mapping.event;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.ApplicationListener;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * {@link ApplicationListener} for Mongo mapping events logging the events.
 * 
 * @author Jon Brisbin
 * @author Martin Baumgartner
 */
public class LoggingEventListener extends AbstractMongoEventListener<Object> {

	private static final Logger LOGGER = LoggerFactory.getLogger(LoggingEventListener.class);

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onBeforeConvert(java.lang.Object)
	 */
	@Override
	public void onBeforeConvert(Object source) {
		LOGGER.info("onBeforeConvert: {}", source);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onBeforeSave(java.lang.Object, org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public void onBeforeSave(Object source, DBObject dbo) {
		LOGGER.info("onBeforeSave: {}, {}", source, dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onAfterSave(java.lang.Object, org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public void onAfterSave(Object source, DBObject dbo) {
		LOGGER.info("onAfterSave: {}, {}", source, dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onAfterLoad(org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public void onAfterLoad(DBObject dbo) {
		LOGGER.info("onAfterLoad: {}", dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onAfterConvert(org.springframework.data.mongodb.assist.DBObject, java.lang.Object)
	 */
	@Override
	public void onAfterConvert(DBObject dbo, Object source) {
		LOGGER.info("onAfterConvert: {}, {}", dbo, source);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onAfterDelete(org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public void onAfterDelete(DBObject dbo) {
		LOGGER.info("onAfterDelete: {}", dbo);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.event.AbstractMongoEventListener#onBeforeDelete(org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public void onBeforeDelete(DBObject dbo) {
		LOGGER.info("onBeforeDelete: {}", dbo);
	}
}
