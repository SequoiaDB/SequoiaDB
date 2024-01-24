/*
 * Copyright 2011-2014 the original author or authors.
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
package org.springframework.data.sequoiadb.core.index;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.ApplicationListener;
import org.springframework.data.mapping.PersistentEntity;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.context.MappingContextEvent;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.index.SequoiadbPersistentEntityIndexResolver.IndexDefinitionHolder;
import org.springframework.data.sequoiadb.core.mapping.Document;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.util.Assert;

/**
 * Component that inspects {@link SequoiadbPersistentEntity} instances contained in the given {@link SequoiadbMappingContext}
 * for indexing metadata and ensures the indexes to be available.
 *
 */
public class SequoiadbPersistentEntityIndexCreator implements
		ApplicationListener<MappingContextEvent<SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty>> {

	private static final Logger LOGGER = LoggerFactory.getLogger(SequoiadbPersistentEntityIndexCreator.class);

	private final Map<Class<?>, Boolean> classesSeen = new ConcurrentHashMap<Class<?>, Boolean>();
	private final SequoiadbFactory sequoiadbFactory;
	private final SequoiadbMappingContext mappingContext;
	private final IndexResolver indexResolver;

	/**
	 * Creats a new {@link SequoiadbPersistentEntityIndexCreator} for the given {@link SequoiadbMappingContext} and
	 * {@link SequoiadbFactory}.
	 * 
	 * @param mappingContext must not be {@literal null}.
	 * @param sequoiadbFactory must not be {@literal null}.
	 */
	public SequoiadbPersistentEntityIndexCreator(SequoiadbMappingContext mappingContext, SequoiadbFactory sequoiadbFactory) {
		this(mappingContext, sequoiadbFactory, new SequoiadbPersistentEntityIndexResolver(mappingContext));
	}

	/**
	 * Creats a new {@link SequoiadbPersistentEntityIndexCreator} for the given {@link SequoiadbMappingContext} and
	 * {@link SequoiadbFactory}.
	 * 
	 * @param mappingContext must not be {@literal null}.
	 * @param sequoiadbFactory must not be {@literal null}.
	 * @param indexResolver must not be {@literal null}.
	 */
	public SequoiadbPersistentEntityIndexCreator(SequoiadbMappingContext mappingContext, SequoiadbFactory sequoiadbFactory,
												 IndexResolver indexResolver) {

		Assert.notNull(sequoiadbFactory);
		Assert.notNull(mappingContext);
		Assert.notNull(indexResolver);

		this.sequoiadbFactory = sequoiadbFactory;
		this.mappingContext = mappingContext;
		this.indexResolver = indexResolver;

		for (SequoiadbPersistentEntity<?> entity : mappingContext.getPersistentEntities()) {
			checkForIndexes(entity);
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.context.ApplicationListener#onApplicationEvent(org.springframework.context.ApplicationEvent)
	 */
	public void onApplicationEvent(MappingContextEvent<SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> event) {

		if (!event.wasEmittedBy(mappingContext)) {
			return;
		}

		PersistentEntity<?, ?> entity = event.getPersistentEntity();

		// Double check type as Spring infrastructure does not consider nested generics
		if (entity instanceof SequoiadbPersistentEntity) {
			checkForIndexes(event.getPersistentEntity());
		}
	}

	private void checkForIndexes(final SequoiadbPersistentEntity<?> entity) {

		Class<?> type = entity.getType();

		if (!classesSeen.containsKey(type)) {

			this.classesSeen.put(type, Boolean.TRUE);

			if (LOGGER.isDebugEnabled()) {
				LOGGER.debug("Analyzing class " + type + " for index information.");
			}

			checkForAndCreateIndexes(entity);
		}
	}

	private void checkForAndCreateIndexes(SequoiadbPersistentEntity<?> entity) {

		if (entity.findAnnotation(Document.class) != null) {
			for (IndexDefinitionHolder indexToCreate : indexResolver.resolveIndexForClass(entity.getType())) {
				createIndex(indexToCreate);
			}
		}
	}

	private void createIndex(IndexDefinitionHolder indexDefinition) {
		sequoiadbFactory.getDb().getCollection(indexDefinition.getCollection())
				.createIndex(indexDefinition.getIndexKeys(), indexDefinition.getIndexOptions());
	}

	/**
	 * Returns whether the current index creator was registered for the given {@link MappingContext}.
	 * 
	 * @param context
	 * @return
	 */
	public boolean isIndexCreatorFor(MappingContext<?, ?> context) {
		return this.mappingContext.equals(context);
	}
}
