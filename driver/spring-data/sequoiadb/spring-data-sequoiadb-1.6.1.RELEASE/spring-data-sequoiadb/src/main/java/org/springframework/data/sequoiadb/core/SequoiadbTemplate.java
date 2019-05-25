/*
 * Copyright 2010-2014 the original author or authors.
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
package org.springframework.data.sequoiadb.core;

import static org.springframework.data.sequoiadb.core.query.Criteria.*;
import static org.springframework.data.sequoiadb.core.query.SerializationUtils.*;

import java.io.IOException;
import java.util.*;
import java.util.Map.Entry;

import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.bson.util.JSONParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.springframework.context.ApplicationEventPublisher;
import org.springframework.context.ApplicationEventPublisherAware;
import org.springframework.context.ApplicationListener;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.core.convert.ConversionService;
import org.springframework.core.io.Resource;
import org.springframework.core.io.ResourceLoader;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.InvalidDataAccessApiUsageException;
import org.springframework.dao.OptimisticLockingFailureException;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.data.annotation.Id;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.convert.EntityReader;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.GeoResult;
import org.springframework.data.geo.GeoResults;
import org.springframework.data.geo.Metric;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.model.BeanWrapper;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.aggregation.Aggregation;
import org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext;
import org.springframework.data.sequoiadb.core.aggregation.AggregationResults;
import org.springframework.data.sequoiadb.core.aggregation.Fields;
import org.springframework.data.sequoiadb.core.aggregation.TypeBasedAggregationOperationContext;
import org.springframework.data.sequoiadb.core.aggregation.TypedAggregation;
import org.springframework.data.sequoiadb.core.convert.*;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.core.index.SequoiadbMappingEventPublisher;
import org.springframework.data.sequoiadb.core.index.SequoiadbPersistentEntityIndexCreator;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbSimpleTypes;
import org.springframework.data.sequoiadb.core.mapping.event.AfterConvertEvent;
import org.springframework.data.sequoiadb.core.mapping.event.AfterDeleteEvent;
import org.springframework.data.sequoiadb.core.mapping.event.AfterLoadEvent;
import org.springframework.data.sequoiadb.core.mapping.event.AfterSaveEvent;
import org.springframework.data.sequoiadb.core.mapping.event.BeforeConvertEvent;
import org.springframework.data.sequoiadb.core.mapping.event.BeforeDeleteEvent;
import org.springframework.data.sequoiadb.core.mapping.event.BeforeSaveEvent;
import org.springframework.data.sequoiadb.core.mapping.event.SequoiadbMappingEvent;
import org.springframework.data.sequoiadb.core.mapreduce.GroupBy;
import org.springframework.data.sequoiadb.core.mapreduce.GroupByResults;
import org.springframework.data.sequoiadb.core.mapreduce.MapReduceOptions;
import org.springframework.data.sequoiadb.core.mapreduce.MapReduceResults;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.NearQuery;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.core.query.Update;
import org.springframework.jca.cci.core.ConnectionCallback;
import org.springframework.util.Assert;
import org.springframework.util.CollectionUtils;
import org.springframework.util.ResourceUtils;

/**
 * Primary implementation of {@link SequoiadbOperations}.
 *
 */
@SuppressWarnings("deprecation")
public class SequoiadbTemplate implements SequoiadbOperations, ApplicationContextAware {

	private static final Logger LOGGER = LoggerFactory.getLogger(SequoiadbTemplate.class);
	private static final String ID_FIELD = "_id";
	private static final WriteResultChecking DEFAULT_WRITE_RESULT_CHECKING = WriteResultChecking.NONE;
	private static final Collection<String> ITERABLE_CLASSES;

	static {

		Set<String> iterableClasses = new HashSet<String>();
		iterableClasses.add(List.class.getName());
		iterableClasses.add(Collection.class.getName());
		iterableClasses.add(Iterator.class.getName());

		ITERABLE_CLASSES = Collections.unmodifiableCollection(iterableClasses);
	}

	private final SequoiadbConverter sequoiadbConverter;
	private final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;
	private final SequoiadbFactory sequoiadbFactory;
	private final PersistenceExceptionTranslator exceptionTranslator;
	private final QueryMapper queryMapper;
	private final UpdateMapper updateMapper;

	private WriteConcern writeConcern;
	private WriteConcernResolver writeConcernResolver = DefaultWriteConcernResolver.INSTANCE;
	private WriteResultChecking writeResultChecking = WriteResultChecking.NONE;
	private ReadPreference readPreference;
	private ApplicationEventPublisher eventPublisher;
	private ResourceLoader resourceLoader;
	private SequoiadbPersistentEntityIndexCreator indexCreator;

	/**
	 * Constructor used for a basic template configuration
	 * 
	 * @param sdb must not be {@literal null}.
	 * @param databaseName must not be {@literal null} or empty.
	 */
	public SequoiadbTemplate(Sdb sdb, String databaseName) {
		this(new SimpleSequoiadbFactory(sdb, databaseName), null);
	}

	/**
	 * Constructor used for a template configuration with user credentials in the form of
	 * {@link org.springframework.data.authentication.UserCredentials}
	 * 
	 * @param sdb must not be {@literal null}.
	 * @param databaseName must not be {@literal null} or empty.
	 * @param userCredentials
	 */
	public SequoiadbTemplate(Sdb sdb, String databaseName, UserCredentials userCredentials) {
		this(new SimpleSequoiadbFactory(sdb, databaseName, userCredentials));
	}

	/**
	 * Constructor used for a basic template configuration.
	 * 
	 * @param sequoiadbFactory must not be {@literal null}.
	 */
	public SequoiadbTemplate(SequoiadbFactory sequoiadbFactory) {
		this(sequoiadbFactory, null);
	}

	/**
	 * Constructor used for a basic template configuration.
	 * 
	 * @param sequoiadbFactory must not be {@literal null}.
	 * @param sequoiadbConverter
	 */
	public SequoiadbTemplate(SequoiadbFactory sequoiadbFactory, SequoiadbConverter sequoiadbConverter) {

		Assert.notNull(sequoiadbFactory);

		this.sequoiadbFactory = sequoiadbFactory;
		this.exceptionTranslator = sequoiadbFactory.getExceptionTranslator();
		this.sequoiadbConverter = sequoiadbConverter == null ? getDefaultSequoiadbConverter(sequoiadbFactory) : sequoiadbConverter;
		this.queryMapper = new QueryMapper(this.sequoiadbConverter);
		this.updateMapper = new UpdateMapper(this.sequoiadbConverter);

		mappingContext = this.sequoiadbConverter.getMappingContext();
		if (null != mappingContext && mappingContext instanceof SequoiadbMappingContext) {
			indexCreator = new SequoiadbPersistentEntityIndexCreator((SequoiadbMappingContext) mappingContext, sequoiadbFactory);
			eventPublisher = new SequoiadbMappingEventPublisher(indexCreator);
			if (mappingContext instanceof ApplicationEventPublisherAware) {
				((ApplicationEventPublisherAware) mappingContext).setApplicationEventPublisher(eventPublisher);
			}
		}
	}

	/**
	 * Configures the {@link WriteResultChecking} to be used with the template. Setting {@literal null} will reset the
	 * default of {@value #DEFAULT_WRITE_RESULT_CHECKING}.
	 * 
	 * @param resultChecking
	 */
	public void setWriteResultChecking(WriteResultChecking resultChecking) {
		this.writeResultChecking = resultChecking == null ? DEFAULT_WRITE_RESULT_CHECKING : resultChecking;
	}

	/**
	 * Configures the {@link WriteConcern} to be used with the template. If none is configured the {@link WriteConcern}
	 * configured on the {@link SequoiadbFactory} will apply. If you configured a {@link Sdb} instance no
	 * {@link WriteConcern} will be used.
	 * 
	 * @param writeConcern
	 */
	public void setWriteConcern(WriteConcern writeConcern) {
		this.writeConcern = writeConcern;
	}

	/**
	 * Configures the {@link WriteConcernResolver} to be used with the template.
	 * 
	 * @param writeConcernResolver
	 */
	public void setWriteConcernResolver(WriteConcernResolver writeConcernResolver) {
		this.writeConcernResolver = writeConcernResolver;
	}

	/**
	 * Used by @{link {@link #prepareCollection(DBCollection)} to set the {@link ReadPreference} before any operations are
	 * performed.
	 * 
	 * @param readPreference
	 */
	public void setReadPreference(ReadPreference readPreference) {
		this.readPreference = readPreference;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.context.ApplicationContextAware#setApplicationContext(org.springframework.context.ApplicationContext)
	 */
	public void setApplicationContext(ApplicationContext applicationContext) throws BeansException {

		prepareIndexCreator(applicationContext);

		eventPublisher = applicationContext;
		if (mappingContext instanceof ApplicationEventPublisherAware) {
			((ApplicationEventPublisherAware) mappingContext).setApplicationEventPublisher(eventPublisher);
		}
		resourceLoader = applicationContext;
	}

	/**
	 * Inspects the given {@link ApplicationContext} for {@link SequoiadbPersistentEntityIndexCreator} and those in turn if
	 * they were registered for the current {@link MappingContext}. If no creator for the current {@link MappingContext}
	 * can be found we manually add the internally created one as {@link ApplicationListener} to make sure indexes get
	 * created appropriately for entity types persisted through this {@link SequoiadbTemplate} instance.
	 * 
	 * @param context must not be {@literal null}.
	 */
	private void prepareIndexCreator(ApplicationContext context) {

		String[] indexCreators = context.getBeanNamesForType(SequoiadbPersistentEntityIndexCreator.class);

		for (String creator : indexCreators) {
			SequoiadbPersistentEntityIndexCreator creatorBean = context.getBean(creator, SequoiadbPersistentEntityIndexCreator.class);
			if (creatorBean.isIndexCreatorFor(mappingContext)) {
				return;
			}
		}

		if (context instanceof ConfigurableApplicationContext) {
			((ConfigurableApplicationContext) context).addApplicationListener(indexCreator);
		}
	}

	/**
	 * Returns the default {@link org.springframework.data.sequoiadb.core.core.convert.SequoiadbConverter}.
	 * 
	 * @return
	 */
	public SequoiadbConverter getConverter() {
		return this.sequoiadbConverter;
	}

	public String getCollectionName(Class<?> entityClass) {
		return this.determineCollectionName(entityClass);
	}

	public CommandResult executeCommand(String jsonCommand) {
		throw new UnsupportedOperationException("not supported!");
	}

	public CommandResult executeCommand(final BSONObject command) {
		throw new UnsupportedOperationException("not supported!");
	}

	public CommandResult executeCommand(final BSONObject command, final int options) {
		throw new UnsupportedOperationException("not supported!");
	}

	protected void logCommandExecutionError(final BSONObject command, CommandResult result) {
		String error = result.getErrorMessage();
		if (error != null) {
			LOGGER.warn("Command execution of " + command.toString() + " failed: " + error);
		}
	}

	public void executeQuery(Query query, String collectionName, DocumentCallbackHandler dch) {
		executeQuery(query, collectionName, dch, new QueryCursorPreparer(query, null));
	}

	/**
	 * Execute a SequoiaDB query and iterate over the query results on a per-document basis with a
	 * {@link DocumentCallbackHandler} using the provided CursorPreparer.
	 * 
	 * @param query the query class that specifies the criteria used to find a record and also an optional fields
	 *          specification, must not be {@literal null}.
	 * @param collectionName name of the collection to retrieve the objects from
	 * @param dch the handler that will extract results, one document at a time
	 * @param preparer allows for customization of the {@link DBCursor} used when iterating over the result set, (apply
	 *          limits, skips and so on).
	 */
	protected void executeQuery(Query query, String collectionName, DocumentCallbackHandler dch, CursorPreparer preparer) {

		Assert.notNull(query);

		BSONObject queryObject = queryMapper.getMappedObject(query.getQueryObject(), null);
		BSONObject fieldsObject = query.getFieldsObject();
		BSONObject sortObject = query.getSortObject();
		BSONObject hintObject = query.getHintObject();
		int skip = query.getSkip();
		int limit = query.getLimit();
		int flags = query.getQueryFlags();

		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug(String.format("Executing query: %s sort: %s fields: %s in collection: %s",
					serializeToJsonSafely(queryObject), sortObject, fieldsObject, collectionName));
		}

		this.executeQueryInternal(
				new FindCallback(queryObject, fieldsObject, sortObject, hintObject, skip, limit, flags),
				null, dch, collectionName);
	}

	public <T> T execute(DbCallback<T> action) {

		Assert.notNull(action);

		try {
			DB db = this.getDb();
			return action.doInDB(db);
		} catch (RuntimeException e) {
			throw potentiallyConvertRuntimeException(e);
		}
	}

	public <T> T execute(Class<?> entityClass, CollectionCallback<T> callback) {
		return execute(determineCollectionName(entityClass), callback);
	}

	public <T> T execute(String collectionName, CollectionCallback<T> callback) {

		Assert.notNull(callback);

		try {
			DBCollection collection = getAndPrepareCollection(getDb(), collectionName);
			return callback.doInCollection(collection);
		} catch (RuntimeException e) {
			throw potentiallyConvertRuntimeException(e);
		}
	}

	public <T> T executeInSession(final DbCallback<T> action) {
		return execute(new DbCallback<T>() {
			public T doInDB(DB db) throws BaseException, DataAccessException {
				try {
					db.requestStart();
					return action.doInDB(db);
				} finally {
					db.requestDone();
				}
			}
		});
	}

	public <T> DBCollection createCollection(Class<T> entityClass) {
		return createCollection(determineCollectionName(entityClass));
	}

	public <T> DBCollection createCollection(Class<T> entityClass, CollectionOptions collectionOptions) {
		return createCollection(determineCollectionName(entityClass), collectionOptions);
	}

	public DBCollection createCollection(final String collectionName) {
		return doCreateCollection(collectionName, new BasicBSONObject());
	}

	public DBCollection createCollection(final String collectionName, final CollectionOptions collectionOptions) {
		return doCreateCollection(collectionName, null);
	}

	public <T> DBCollection getCollection(Class<T> entityClass) {
		return getCollection(determineCollectionName(entityClass));
	}

	public DBCollection getCollection(final String collectionName) {
		return execute(new DbCallback<DBCollection>() {
			public DBCollection doInDB(DB db) throws BaseException, DataAccessException {
				return db.getCollection(collectionName);
			}
		});
	}

	public <T> boolean collectionExists(Class<T> entityClass) {
		return collectionExists(determineCollectionName(entityClass));
	}

	public boolean collectionExists(final String collectionName) {
		return execute(new DbCallback<Boolean>() {
			public Boolean doInDB(DB db) throws BaseException, DataAccessException {
				return db.collectionExists(collectionName);
			}
		});
	}

	public <T> void dropCollection(Class<T> entityClass) {
		dropCollection(determineCollectionName(entityClass));
	}

	public void dropCollection(String collectionName) {
		execute(collectionName, new CollectionCallback<Void>() {
			public Void doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				collection.drop();
				if (LOGGER.isDebugEnabled()) {
					LOGGER.debug("Dropped collection [" + collection.getFullName() + "]");
				}
				return null;
			}
		});
	}

	public IndexOperations indexOps(String collectionName) {
		return new DefaultIndexOperations(this, collectionName);
	}

	public IndexOperations indexOps(Class<?> entityClass) {
		return new DefaultIndexOperations(this, determineCollectionName(entityClass));
	}


	public <T> T findOne(Query query, Class<T> entityClass) {
		return findOne(query, entityClass, determineCollectionName(entityClass));
	}

	public <T> T findOne(Query query, Class<T> entityClass, String collectionName) {
		if (query.getSortObject() == null) {
			return doFindOne(collectionName, query.getQueryObject(), query.getFieldsObject(), entityClass);
		} else {
			query.limit(1);
			List<T> results = find(query, entityClass, collectionName);
			return results.isEmpty() ? null : results.get(0);
		}
	}

	public boolean exists(Query query, Class<?> entityClass) {
		return exists(query, entityClass, determineCollectionName(entityClass));
	}

	public boolean exists(Query query, String collectionName) {
		return exists(query, null, collectionName);
	}

	public boolean exists(Query query, Class<?> entityClass, String collectionName) {

		if (query == null) {
			throw new InvalidDataAccessApiUsageException("Query passed in to exist can't be null");
		}

		BSONObject mappedQuery = queryMapper.getMappedObject(query.getQueryObject(), getPersistentEntity(entityClass));
		return execute(collectionName, new FindCallback(mappedQuery)).hasNext();
	}


	public <T> List<T> find(Query query, Class<T> entityClass) {
		return find(query, entityClass, determineCollectionName(entityClass));
	}

	public <T> List<T> find(final Query query, Class<T> entityClass, String collectionName) {

		if (query == null) {
			return findAll(entityClass, collectionName);
		}

		return doFind(collectionName, query.getQueryObject(), query.getFieldsObject(), entityClass,
				new QueryCursorPreparer(query, entityClass));
	}

	public <T> T findById(Object id, Class<T> entityClass) {
		return findById(id, entityClass, determineCollectionName(entityClass));
	}

	public <T> T findById(Object id, Class<T> entityClass, String collectionName) {
		SequoiadbPersistentEntity<?> persistentEntity = mappingContext.getPersistentEntity(entityClass);
		SequoiadbPersistentProperty idProperty = persistentEntity == null ? null : persistentEntity.getIdProperty();
		String idKey = idProperty == null ? ID_FIELD : idProperty.getName();
		return doFindOne(collectionName, new BasicBSONObject(idKey, id), null, entityClass);
	}

	public <T> GeoResults<T> geoNear(NearQuery near, Class<T> entityClass) {
		return geoNear(near, entityClass, determineCollectionName(entityClass));
	}

	@SuppressWarnings("unchecked")
	public <T> GeoResults<T> geoNear(NearQuery near, Class<T> entityClass, String collectionName) {
		throw new UnsupportedOperationException("not supported!");
	}

	public <T> T findAndModify(Query query, Update update, Class<T> entityClass) {
		return findAndModify(query, update, new FindAndModifyOptions(), entityClass, determineCollectionName(entityClass));
	}

	public <T> T findAndModify(Query query, Update update, Class<T> entityClass, String collectionName) {
		return findAndModify(query, update, new FindAndModifyOptions(), entityClass, collectionName);
	}

	public <T> T findAndModify(Query query, Update update, FindAndModifyOptions options, Class<T> entityClass) {
		return findAndModify(query, update, options, entityClass, determineCollectionName(entityClass));
	}

	public <T> T findAndModify(Query query, Update update, FindAndModifyOptions options, Class<T> entityClass,
			String collectionName) {
		return doFindAndModify(collectionName, query.getQueryObject(), query.getFieldsObject(),
				getMappedSortObject(query, entityClass), entityClass, update, options);
	}


	public <T> T findAndRemove(Query query, Class<T> entityClass) {
		return findAndRemove(query, entityClass, determineCollectionName(entityClass));
	}

	public <T> T findAndRemove(Query query, Class<T> entityClass, String collectionName) {

		return doFindAndRemove(collectionName, query.getQueryObject(), query.getFieldsObject(),
				getMappedSortObject(query, entityClass), entityClass);
	}

	public long count(Query query, Class<?> entityClass) {
		Assert.notNull(entityClass);
		return count(query, entityClass, determineCollectionName(entityClass));
	}

	public long count(final Query query, String collectionName) {
		return count(query, null, collectionName);
	}

	private long count(Query query, Class<?> entityClass, String collectionName) {

		Assert.hasText(collectionName);
		final BSONObject dbObject = query == null ? null : queryMapper.getMappedObject(query.getQueryObject(),
				entityClass == null ? null : mappingContext.getPersistentEntity(entityClass));

		return execute(collectionName, new CollectionCallback<Long>() {
			public Long doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				return collection.count(dbObject);
			}
		});
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.SequoiadbOperations#insert(java.lang.Object)
	 */
	public void insert(Object objectToSave) {
		ensureNotIterable(objectToSave);
		insert(objectToSave, determineEntityCollectionName(objectToSave));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.SequoiadbOperations#insert(java.lang.Object, java.lang.String)
	 */
	public void insert(Object objectToSave, String collectionName) {
		ensureNotIterable(objectToSave);
		doInsert(collectionName, objectToSave, this.sequoiadbConverter);
	}

	protected void ensureNotIterable(Object o) {
		if (null != o) {
			if (o.getClass().isArray() || ITERABLE_CLASSES.contains(o.getClass().getName())) {
				throw new IllegalArgumentException("Cannot use a collection here.");
			}
		}
	}

	/**
	 * Prepare the collection before any processing is done using it. This allows a convenient way to apply settings like
	 * slaveOk() etc. Can be overridden in sub-classes.
	 * 
	 * @param collection
	 */
	protected void prepareCollection(DBCollection collection) {
		if (this.readPreference != null) {
			collection.setReadPreference(readPreference);
		}
	}

	/**
	 * Prepare the WriteConcern before any processing is done using it. This allows a convenient way to apply custom
	 * settings in sub-classes.
	 * 
	 * @param writeConcern any WriteConcern already configured or null
	 * @return The prepared WriteConcern or null
	 */
	protected WriteConcern prepareWriteConcern(SequoiadbAction sequoiadbAction) {
		return writeConcernResolver.resolve(sequoiadbAction);
	}

	protected <T> void doInsert(String collectionName, T objectToSave, SequoiadbWriter<T> writer) {

		assertUpdateableIdIfNotSet(objectToSave);

		initializeVersionProperty(objectToSave);

		maybeEmitEvent(new BeforeConvertEvent<T>(objectToSave));

		BSONObject dbDoc = toDbObject(objectToSave, writer);

		maybeEmitEvent(new BeforeSaveEvent<T>(objectToSave, dbDoc));
		Object id = insertDBObject(collectionName, dbDoc, objectToSave.getClass());

		populateIdIfNecessary(objectToSave, id);
		maybeEmitEvent(new AfterSaveEvent<T>(objectToSave, dbDoc));
	}

	/**
	 * @param objectToSave
	 * @param writer
	 * @return
	 */
	private <T> BSONObject toDbObject(T objectToSave, SequoiadbWriter<T> writer) {

		if (!(objectToSave instanceof String)) {
			BSONObject dbDoc = new BasicBSONObject();
			writer.write(objectToSave, dbDoc);
			return dbDoc;
		} else {
			try {
				return (BasicBSONObject) JSON.parse((String) objectToSave);
			} catch (JSONParseException e) {
				throw new MappingException("Could not parse given String to save into a JSON document!", e);
			}
		}
	}

	private void initializeVersionProperty(Object entity) {

		SequoiadbPersistentEntity<?> sequoiadbPersistentEntity = getPersistentEntity(entity.getClass());

		if (sequoiadbPersistentEntity != null && sequoiadbPersistentEntity.hasVersionProperty()) {
			BeanWrapper<Object> wrapper = BeanWrapper.create(entity, this.sequoiadbConverter.getConversionService());
			wrapper.setProperty(sequoiadbPersistentEntity.getVersionProperty(), 0);
		}
	}

	public void insert(Collection<? extends Object> batchToSave, Class<?> entityClass) {
		doInsertBatch(determineCollectionName(entityClass), batchToSave, this.sequoiadbConverter);
	}

	public void insert(Collection<? extends Object> batchToSave, String collectionName) {
		doInsertBatch(collectionName, batchToSave, this.sequoiadbConverter);
	}

	public void insertAll(Collection<? extends Object> objectsToSave) {
		doInsertAll(objectsToSave, this.sequoiadbConverter);
	}

	protected <T> void doInsertAll(Collection<? extends T> listToSave, SequoiadbWriter<T> writer) {
		Map<String, List<T>> objs = new HashMap<String, List<T>>();

		for (T o : listToSave) {

			SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(o.getClass());
			if (entity == null) {
				throw new InvalidDataAccessApiUsageException("No Persitent Entity information found for the class "
						+ o.getClass().getName());
			}
			String collection = entity.getCollection();

			List<T> objList = objs.get(collection);
			if (null == objList) {
				objList = new ArrayList<T>();
				objs.put(collection, objList);
			}
			objList.add(o);

		}

		for (Map.Entry<String, List<T>> entry : objs.entrySet()) {
			doInsertBatch(entry.getKey(), entry.getValue(), this.sequoiadbConverter);
		}
	}

	protected <T> void doInsertBatch(String collectionName, Collection<? extends T> batchToSave, SequoiadbWriter<T> writer) {

		Assert.notNull(writer);

		List<BSONObject> dbObjectList = new ArrayList<BSONObject>();
		for (T o : batchToSave) {

			initializeVersionProperty(o);
			BasicBSONObject dbDoc = new BasicBSONObject();

			maybeEmitEvent(new BeforeConvertEvent<T>(o));
			writer.write(o, dbDoc);

			maybeEmitEvent(new BeforeSaveEvent<T>(o, dbDoc));
			dbObjectList.add(dbDoc);
		}
		List<ObjectId> ids = insertDBObjectList(collectionName, dbObjectList);
		int i = 0;
		for (T obj : batchToSave) {
			if (i < ids.size()) {
				populateIdIfNecessary(obj, ids.get(i));
				maybeEmitEvent(new AfterSaveEvent<T>(obj, dbObjectList.get(i)));
			}
			i++;
		}
	}

	public void save(Object objectToSave) {

		Assert.notNull(objectToSave);
		save(objectToSave, determineEntityCollectionName(objectToSave));
	}

	public void save(Object objectToSave, String collectionName) {

		Assert.notNull(objectToSave);
		Assert.hasText(collectionName);

		SequoiadbPersistentEntity<?> sequoiadbPersistentEntity = getPersistentEntity(objectToSave.getClass());

		if (sequoiadbPersistentEntity == null || !sequoiadbPersistentEntity.hasVersionProperty()) {
			doSave(collectionName, objectToSave, this.sequoiadbConverter);
			return;
		}

		doSaveVersioned(objectToSave, sequoiadbPersistentEntity, collectionName);
	}

	private <T> void doSaveVersioned(T objectToSave, SequoiadbPersistentEntity<?> entity, String collectionName) {

		BeanWrapper<T> beanWrapper = BeanWrapper.create(objectToSave, this.sequoiadbConverter.getConversionService());
		SequoiadbPersistentProperty idProperty = entity.getIdProperty();
		SequoiadbPersistentProperty versionProperty = entity.getVersionProperty();

		Number version = beanWrapper.getProperty(versionProperty, Number.class);

		if (version == null) {
			doInsert(collectionName, objectToSave, this.sequoiadbConverter);
		} else {

			assertUpdateableIdIfNotSet(objectToSave);

			Object id = beanWrapper.getProperty(idProperty);
			Query query = new Query(Criteria.where(idProperty.getName()).is(id).and(versionProperty.getName()).is(version));

			Number number = beanWrapper.getProperty(versionProperty, Number.class);
			beanWrapper.setProperty(versionProperty, number.longValue() + 1);

			BasicBSONObject dbObject = new BasicBSONObject();

			maybeEmitEvent(new BeforeConvertEvent<T>(objectToSave));
			this.sequoiadbConverter.write(objectToSave, dbObject);

			maybeEmitEvent(new BeforeSaveEvent<T>(objectToSave, dbObject));
			Update update = new Update();
			BSONObject updateElements = Update.fromDBObject(dbObject, ID_FIELD).getUpdateObject();
			for(String k : updateElements.keySet()) {
				update.replace(k,updateElements.get(k));
			}
			doUpdate(collectionName, query, update, objectToSave.getClass(), false, false);
			maybeEmitEvent(new AfterSaveEvent<T>(objectToSave, dbObject));
		}
	}

	protected <T> void doSave(String collectionName, T objectToSave, SequoiadbWriter<T> writer) {

		assertUpdateableIdIfNotSet(objectToSave);

		maybeEmitEvent(new BeforeConvertEvent<T>(objectToSave));

		BSONObject dbDoc = toDbObject(objectToSave, writer);

		maybeEmitEvent(new BeforeSaveEvent<T>(objectToSave, dbDoc));
		Object id = saveDBObject(collectionName, dbDoc, objectToSave.getClass());

		populateIdIfNecessary(objectToSave, id);
		maybeEmitEvent(new AfterSaveEvent<T>(objectToSave, dbDoc));
	}

	protected Object insertDBObject(final String collectionName, final BSONObject dbDoc, final Class<?> entityClass) {
		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug("Inserting BSONObject containing fields: " + dbDoc.keySet() + " in collection: " + collectionName);
		}
		return execute(collectionName, new CollectionCallback<Object>() {
			public Object doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				SequoiadbAction sequoiadbAction = new SequoiadbAction(writeConcern, SequoiadbActionOperation.INSERT, collectionName,
						entityClass, dbDoc, null);
				WriteConcern writeConcernToUse = prepareWriteConcern(sequoiadbAction);
				WriteResult writeResult = writeConcernToUse == null ? collection.insert(dbDoc) : collection.insert(dbDoc,
						writeConcernToUse);
				handleAnyWriteResultErrors(writeResult, dbDoc, SequoiadbActionOperation.INSERT);
				Object retObj = dbDoc.get(ID_FIELD);
				if (retObj instanceof ObjectId) {
					((ObjectId) retObj).notNew();
				}
				return retObj;
			}
		});
	}

	protected List<ObjectId> insertDBObjectList(final String collectionName, final List<BSONObject> dbDocList) {
		if (dbDocList.isEmpty()) {
			return Collections.emptyList();
		}

		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug("Inserting list of DBObjects containing " + dbDocList.size() + " items");
		}
		execute(collectionName, new CollectionCallback<Void>() {
			public Void doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				SequoiadbAction sequoiadbAction = new SequoiadbAction(writeConcern, SequoiadbActionOperation.INSERT_LIST, collectionName, null,
						null, null);
				WriteConcern writeConcernToUse = prepareWriteConcern(sequoiadbAction);
				WriteResult writeResult = writeConcernToUse == null ? collection.insert(dbDocList) : collection.insert(
						dbDocList.toArray((BSONObject[]) new BasicBSONObject[dbDocList.size()]), writeConcernToUse);
				handleAnyWriteResultErrors(writeResult, null, SequoiadbActionOperation.INSERT_LIST);
				return null;
			}
		});

		List<ObjectId> ids = new ArrayList<ObjectId>();
		for (BSONObject dbo : dbDocList) {
			Object id = dbo.get(ID_FIELD);
			if (id instanceof ObjectId) {
				ids.add((ObjectId) id);
			} else {
				ids.add(null);
			}
		}
		return ids;
	}

	protected Object saveDBObject(final String collectionName, final BSONObject dbDoc, final Class<?> entityClass) {
		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug("Saving BSONObject containing fields: " + dbDoc.keySet());
		}
		return execute(collectionName, new CollectionCallback<Object>() {
			public Object doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				SequoiadbAction sequoiadbAction = new SequoiadbAction(writeConcern, SequoiadbActionOperation.SAVE, collectionName, entityClass,
						dbDoc, null);
				WriteConcern writeConcernToUse = prepareWriteConcern(sequoiadbAction);
				WriteResult writeResult = writeConcernToUse == null ? collection.save(dbDoc) : collection.save(dbDoc,
						writeConcernToUse);
				handleAnyWriteResultErrors(writeResult, dbDoc, SequoiadbActionOperation.SAVE);
				return dbDoc.get(ID_FIELD);
			}
		});
	}

	public WriteResult upsert(Query query, Update update, Class<?> entityClass) {
		return doUpdate(determineCollectionName(entityClass), query, update, entityClass, true, false);
	}

	public WriteResult upsert(Query query, Update update, String collectionName) {
		return doUpdate(collectionName, query, update, null, true, false);
	}

	public WriteResult upsert(Query query, Update update, Class<?> entityClass, String collectionName) {
		return doUpdate(collectionName, query, update, entityClass, true, false);
	}

	public WriteResult updateFirst(Query query, Update update, Class<?> entityClass) {
		return doUpdate(determineCollectionName(entityClass), query, update, entityClass, false, false);
	}

	public WriteResult updateFirst(final Query query, final Update update, final String collectionName) {
		return doUpdate(collectionName, query, update, null, false, false);
	}

	public WriteResult updateFirst(Query query, Update update, Class<?> entityClass, String collectionName) {
		return doUpdate(collectionName, query, update, entityClass, false, false);
	}

	public WriteResult updateMulti(Query query, Update update, Class<?> entityClass) {
		return doUpdate(determineCollectionName(entityClass), query, update, entityClass, false, true);
	}

	public WriteResult updateMulti(final Query query, final Update update, String collectionName) {
		return doUpdate(collectionName, query, update, null, false, true);
	}

	public WriteResult updateMulti(final Query query, final Update update, Class<?> entityClass, String collectionName) {
		return doUpdate(collectionName, query, update, entityClass, false, true);
	}

	protected WriteResult doUpdate(final String collectionName, final Query query, final Update update,
			final Class<?> entityClass, final boolean upsert, final boolean multi) {

		return execute(collectionName, new CollectionCallback<WriteResult>() {
			public WriteResult doInCollection(DBCollection collection) throws BaseException, DataAccessException {

				SequoiadbPersistentEntity<?> entity = entityClass == null ? null : getPersistentEntity(entityClass);

				increaseVersionForUpdateIfNecessary(entity, update);

				BSONObject queryObj = query == null ? new BasicBSONObject() : queryMapper.getMappedObject(query.getQueryObject(),
						entity);
				BSONObject updateObj = update == null ? new BasicBSONObject() : updateMapper.getMappedObject(
						update.getUpdateObject(), entity);

				if (LOGGER.isDebugEnabled()) {
					LOGGER.debug("Calling update using query: " + queryObj + " and update: " + updateObj + " in collection: "
							+ collectionName);
				}

				SequoiadbAction sequoiadbAction = new SequoiadbAction(writeConcern, SequoiadbActionOperation.UPDATE, collectionName,
						entityClass, updateObj, queryObj);
				WriteConcern writeConcernToUse = prepareWriteConcern(sequoiadbAction);
				WriteResult writeResult = writeConcernToUse == null ? collection.update(queryObj, updateObj, upsert, multi)
						: collection.update(queryObj, updateObj, upsert, multi, writeConcernToUse);

				if (entity != null && entity.hasVersionProperty() && !multi) {
					if (writeResult.getN() < 0 && dbObjectContainsVersionProperty(queryObj, entity)) {
						throw new OptimisticLockingFailureException("Optimistic lock exception on saving entity: "
								+ updateObj.toMap().toString() + " to collection " + collectionName);
					}
				}

				handleAnyWriteResultErrors(writeResult, queryObj, SequoiadbActionOperation.UPDATE);
				return writeResult;
			}
		});
	}

	private void increaseVersionForUpdateIfNecessary(SequoiadbPersistentEntity<?> persistentEntity, Update update) {

		if (persistentEntity != null && persistentEntity.hasVersionProperty()) {
			String versionFieldName = persistentEntity.getVersionProperty().getFieldName();
			if (!update.modifies(versionFieldName)) {
				update.inc(versionFieldName, 1L);
			}
		}
	}

	private boolean dbObjectContainsVersionProperty(BSONObject dbObject, SequoiadbPersistentEntity<?> persistentEntity) {

		if (persistentEntity == null || !persistentEntity.hasVersionProperty()) {
			return false;
		}

		return dbObject.containsField(persistentEntity.getVersionProperty().getFieldName());
	}

	public WriteResult remove(Object object) {

		if (object == null) {
			return null;
		}

		return remove(getIdQueryFor(object), object.getClass());
	}

	public WriteResult remove(Object object, String collection) {

		Assert.hasText(collection);

		if (object == null) {
			return null;
		}

		return doRemove(collection, getIdQueryFor(object), object.getClass());
	}

	/**
	 * Returns {@link Entry} containing the field name of the id property as {@link Entry#getKey()} and the {@link Id}s
	 * property value as its {@link Entry#getValue()}.
	 * 
	 * @param object
	 * @return
	 */
	private Entry<String, Object> extractIdPropertyAndValue(Object object) {

		Assert.notNull(object, "Id cannot be extracted from 'null'.");

		Class<?> objectType = object.getClass();

		if (object instanceof BSONObject) {
			return Collections.singletonMap(ID_FIELD, ((BSONObject) object).get(ID_FIELD)).entrySet().iterator().next();
		}

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(objectType);
		SequoiadbPersistentProperty idProp = entity == null ? null : entity.getIdProperty();

		if (idProp == null) {
			throw new MappingException("No id property found for object of type " + objectType);
		}

		Object idValue = BeanWrapper.create(object, sequoiadbConverter.getConversionService())
				.getProperty(idProp, Object.class);
		return Collections.singletonMap(idProp.getFieldName(), idValue).entrySet().iterator().next();
	}

	/**
	 * Returns a {@link Query} for the given entity by its id.
	 * 
	 * @param object must not be {@literal null}.
	 * @return
	 */
	private Query getIdQueryFor(Object object) {

		Entry<String, Object> id = extractIdPropertyAndValue(object);
		return new Query(where(id.getKey()).is(id.getValue()));
	}

	/**
	 * Returns a {@link Query} for the given entities by their ids.
	 * 
	 * @param objects must not be {@literal null} or {@literal empty}.
	 * @return
	 */
	private Query getIdInQueryFor(Collection<?> objects) {

		Assert.notEmpty(objects, "Cannot create Query for empty collection.");

		Iterator<?> it = objects.iterator();
		Entry<String, Object> firstEntry = extractIdPropertyAndValue(it.next());

		ArrayList<Object> ids = new ArrayList<Object>(objects.size());
		ids.add(firstEntry.getValue());

		while (it.hasNext()) {
			ids.add(extractIdPropertyAndValue(it.next()).getValue());
		}

		return new Query(where(firstEntry.getKey()).in(ids));
	}

	private void assertUpdateableIdIfNotSet(Object entity) {

		SequoiadbPersistentEntity<?> persistentEntity = mappingContext.getPersistentEntity(entity.getClass());
		SequoiadbPersistentProperty idProperty = persistentEntity == null ? null : persistentEntity.getIdProperty();

		if (idProperty == null) {
			return;
		}

		ConversionService service = sequoiadbConverter.getConversionService();
		Object idValue = BeanWrapper.create(entity, service).getProperty(idProperty, Object.class);

		if (idValue == null && !SequoiadbSimpleTypes.AUTOGENERATED_ID_TYPES.contains(idProperty.getType())) {
			throw new InvalidDataAccessApiUsageException(String.format(
					"Cannot autogenerate id of type %s for entity of type %s!", idProperty.getType().getName(), entity.getClass()
							.getName()));
		}
	}

	public WriteResult remove(Query query, String collectionName) {
		return remove(query, null, collectionName);
	}

	public WriteResult remove(Query query, Class<?> entityClass) {
		return remove(query, entityClass, determineCollectionName(entityClass));
	}

	public WriteResult remove(Query query, Class<?> entityClass, String collectionName) {
		return doRemove(collectionName, query, entityClass);
	}

	protected <T> WriteResult doRemove(final String collectionName, final Query query, final Class<T> entityClass) {

		if (query == null) {
			throw new InvalidDataAccessApiUsageException("Query passed in to remove can't be null!");
		}

		Assert.hasText(collectionName, "Collection name must not be null or empty!");

		final BSONObject queryObject = query.getQueryObject();
		final SequoiadbPersistentEntity<?> entity = getPersistentEntity(entityClass);

		return execute(collectionName, new CollectionCallback<WriteResult>() {
			public WriteResult doInCollection(DBCollection collection) throws BaseException, DataAccessException {

				maybeEmitEvent(new BeforeDeleteEvent<T>(queryObject, entityClass));

				BSONObject dboq = queryMapper.getMappedObject(queryObject, entity);

				SequoiadbAction sequoiadbAction = new SequoiadbAction(writeConcern, SequoiadbActionOperation.REMOVE, collectionName,
						entityClass, null, queryObject);
				WriteConcern writeConcernToUse = prepareWriteConcern(sequoiadbAction);

				if (LOGGER.isDebugEnabled()) {
					LOGGER.debug("Remove using query: {} in collection: {}.", new Object[] { dboq, collection.getName() });
				}

				WriteResult wr = writeConcernToUse == null ? collection.remove(dboq) : collection.remove(dboq,
						writeConcernToUse);

				handleAnyWriteResultErrors(wr, dboq, SequoiadbActionOperation.REMOVE);

				maybeEmitEvent(new AfterDeleteEvent<T>(queryObject, entityClass));

				return wr;
			}
		});
	}

	public <T> List<T> findAll(Class<T> entityClass) {
		return executeFindMultiInternal(new FindCallback(null), null, new ReadDbObjectCallback<T>(sequoiadbConverter,
				entityClass), determineCollectionName(entityClass));
	}

	public <T> List<T> findAll(Class<T> entityClass, String collectionName) {
		return executeFindMultiInternal(new FindCallback(null), null, new ReadDbObjectCallback<T>(sequoiadbConverter,
				entityClass), collectionName);
	}

	public <T> MapReduceResults<T> mapReduce(String inputCollectionName, String mapFunction, String reduceFunction,
			Class<T> entityClass) {
		return mapReduce(null, inputCollectionName, mapFunction, reduceFunction, new MapReduceOptions().outputTypeInline(),
				entityClass);
	}

	public <T> MapReduceResults<T> mapReduce(String inputCollectionName, String mapFunction, String reduceFunction,
			MapReduceOptions mapReduceOptions, Class<T> entityClass) {
		return mapReduce(null, inputCollectionName, mapFunction, reduceFunction, mapReduceOptions, entityClass);
	}

	public <T> MapReduceResults<T> mapReduce(Query query, String inputCollectionName, String mapFunction,
			String reduceFunction, Class<T> entityClass) {
		return mapReduce(query, inputCollectionName, mapFunction, reduceFunction,
				new MapReduceOptions().outputTypeInline(), entityClass);
	}

	public <T> MapReduceResults<T> mapReduce(Query query, String inputCollectionName, String mapFunction,
			String reduceFunction, MapReduceOptions mapReduceOptions, Class<T> entityClass) {
		throw new UnsupportedOperationException("not supported!");
	}

	public <T> GroupByResults<T> group(String inputCollectionName, GroupBy groupBy, Class<T> entityClass) {
		return group(null, inputCollectionName, groupBy, entityClass);
	}

	public <T> GroupByResults<T> group(Criteria criteria, String inputCollectionName, GroupBy groupBy,
			Class<T> entityClass) {
		throw new UnsupportedOperationException("not supported!");
	}

	@Override
	public <O> AggregationResults<O> aggregate(TypedAggregation<?> aggregation, Class<O> outputType) {
		return aggregate(aggregation, determineCollectionName(aggregation.getInputType()), outputType);
	}

	@Override
	public <O> AggregationResults<O> aggregate(TypedAggregation<?> aggregation, String inputCollectionName,
			Class<O> outputType) {

		Assert.notNull(aggregation, "Aggregation pipeline must not be null!");

		AggregationOperationContext context = new TypeBasedAggregationOperationContext(aggregation.getInputType(),
				mappingContext, queryMapper);
		return aggregate(aggregation, inputCollectionName, outputType, context);
	}

	@Override
	public <O> AggregationResults<O> aggregate(Aggregation aggregation, Class<?> inputType, Class<O> outputType) {

		return aggregate(aggregation, determineCollectionName(inputType), outputType,
				new TypeBasedAggregationOperationContext(inputType, mappingContext, queryMapper));
	}

	@Override
	public <O> AggregationResults<O> aggregate(Aggregation aggregation, String collectionName, Class<O> outputType) {
		return aggregate(aggregation, collectionName, outputType, null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.SequoiadbOperations#findAllAndRemove(org.springframework.data.sequoiadb.core.query.Query, java.lang.String)
	 */
	@Override
	public <T> List<T> findAllAndRemove(Query query, String collectionName) {
		return findAndRemove(query, null, collectionName);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.SequoiadbOperations#findAllAndRemove(org.springframework.data.sequoiadb.core.query.Query, java.lang.Class)
	 */
	@Override
	public <T> List<T> findAllAndRemove(Query query, Class<T> entityClass) {
		return findAllAndRemove(query, entityClass, determineCollectionName(entityClass));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.SequoiadbOperations#findAllAndRemove(org.springframework.data.sequoiadb.core.query.Query, java.lang.Class, java.lang.String)
	 */
	@Override
	public <T> List<T> findAllAndRemove(Query query, Class<T> entityClass, String collectionName) {
		return doFindAndDelete(collectionName, query, entityClass);
	}

	/**
	 * Retrieve and remove all documents matching the given {@code query} by calling {@link #find(Query, Class, String)}
	 * and {@link #remove(Query, Class, String)}, whereas the {@link Query} for {@link #remove(Query, Class, String)} is
	 * constructed out of the find result.
	 * 
	 * @param collectionName
	 * @param query
	 * @param entityClass
	 * @return
	 */
	protected <T> List<T> doFindAndDelete(String collectionName, Query query, Class<T> entityClass) {

		List<T> result = find(query, entityClass, collectionName);

		if (!CollectionUtils.isEmpty(result)) {
			remove(getIdInQueryFor(result), entityClass, collectionName);
		}

		return result;
	}

	protected <O> AggregationResults<O> aggregate(Aggregation aggregation, String collectionName, Class<O> outputType,
			AggregationOperationContext context) {
		throw new UnsupportedOperationException("not supported!");
	}

	/**
	 * Returns the potentially mapped results of the given {@commandResult} contained some.
	 * 
	 * @param outputType
	 * @param commandResult
	 * @return
	 */
	private <O> List<O> returnPotentiallyMappedResults(Class<O> outputType, CommandResult commandResult) {

		@SuppressWarnings("unchecked")
		Iterable<BSONObject> resultSet = (Iterable<BSONObject>) commandResult.get("result");
		if (resultSet == null) {
			return Collections.emptyList();
		}

		DbObjectCallback<O> callback = new UnwrapAndReadDbObjectCallback<O>(sequoiadbConverter, outputType);

		List<O> mappedResults = new ArrayList<O>();
		for (BSONObject dbObject : resultSet) {
			mappedResults.add(callback.doWith(dbObject));
		}

		return mappedResults;
	}

	protected String replaceWithResourceIfNecessary(String function) {

		String func = function;

		if (this.resourceLoader != null && ResourceUtils.isUrl(function)) {

			Resource functionResource = resourceLoader.getResource(func);

			if (!functionResource.exists()) {
				throw new InvalidDataAccessApiUsageException(String.format("Resource %s not found!", function));
			}

			try {
				return new Scanner(functionResource.getInputStream()).useDelimiter("\\A").next();
			} catch (IOException e) {
				throw new InvalidDataAccessApiUsageException(String.format("Cannot read map-reduce file %s!", function), e);
			}
		}

		return func;
	}

	private BSONObject copyQuery(Query query, BSONObject copyMapReduceOptions) {
		if (query != null) {
			if (query.getSkip() != 0 || query.getFieldsObject() != null) {
				throw new InvalidDataAccessApiUsageException(
						"Can not use skip or field specification with map reduce operations");
			}
			if (query.getQueryObject() != null) {
				copyMapReduceOptions.put("query", queryMapper.getMappedObject(query.getQueryObject(), null));
			}
			if (query.getLimit() > 0) {
				copyMapReduceOptions.put("limit", query.getLimit());
			}
			if (query.getSortObject() != null) {
				copyMapReduceOptions.put("sort", queryMapper.getMappedObject(query.getSortObject(), null));
			}
		}
		return copyMapReduceOptions;
	}

	private BSONObject copyMapReduceOptions(MapReduceOptions mapReduceOptions, MapReduceCommand command) {
		if (mapReduceOptions.getJavaScriptMode() != null) {
			command.addExtraOption("jsMode", true);
		}
		if (!mapReduceOptions.getExtraOptions().isEmpty()) {
			for (Map.Entry<String, Object> entry : mapReduceOptions.getExtraOptions().entrySet()) {
				command.addExtraOption(entry.getKey(), entry.getValue());
			}
		}
		if (mapReduceOptions.getFinalizeFunction() != null) {
			command.setFinalize(this.replaceWithResourceIfNecessary(mapReduceOptions.getFinalizeFunction()));
		}
		if (mapReduceOptions.getOutputDatabase() != null) {
			command.setOutputDB(mapReduceOptions.getOutputDatabase());
		}
		if (!mapReduceOptions.getScopeVariables().isEmpty()) {
			command.setScope(mapReduceOptions.getScopeVariables());
		}

		BSONObject commandObject = command.toDBObject();
		BSONObject outObject = (BSONObject) commandObject.get("out");

		if (mapReduceOptions.getOutputSharded() != null) {
			outObject.put("sharded", mapReduceOptions.getOutputSharded());
		}
		return commandObject;
	}

	public Set<String> getCollectionNames() {
		return execute(new DbCallback<Set<String>>() {
			public Set<String> doInDB(DB db) throws BaseException, DataAccessException {
				return db.getCollectionNames();
			}
		});
	}

	public DB getDb() {
		return sequoiadbFactory.getDb();
	}

	protected <T> void maybeEmitEvent(SequoiadbMappingEvent<T> event) {
		if (null != eventPublisher) {
			eventPublisher.publishEvent(event);
		}
	}

	/**
	 * Create the specified collection using the provided options
	 * 
	 * @param collectionName
	 * @param collectionOptions
	 * @return the collection that was created
	 */
	protected DBCollection doCreateCollection(final String collectionName, final BSONObject collectionOptions) {
		return execute(new DbCallback<DBCollection>() {
			public DBCollection doInDB(DB db) throws BaseException, DataAccessException {
				DBCollection coll = db.createCollection(collectionName, collectionOptions);
				if (LOGGER.isDebugEnabled()) {
					LOGGER.debug("Created collection [{}]", coll.getFullName());
				}
				return coll;
			}
		});
	}

	/**
	 * Map the results of an ad-hoc query on the default SequoiaDB collection to an object using the template's converter.
	 * The query document is specified as a standard {@link BSONObject} and so is the fields specification.
	 * 
	 * @param collectionName name of the collection to retrieve the objects from.
	 * @param query the query document that specifies the criteria used to find a record.
	 * @param fields the document that specifies the fields to be returned.
	 * @param entityClass the parameterized type of the returned list.
	 * @return the {@link List} of converted objects.
	 */
	protected <T> T doFindOne(String collectionName, BSONObject query, BSONObject fields, Class<T> entityClass) {

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(entityClass);
		BSONObject mappedQuery = queryMapper.getMappedObject(query, entity);
		BSONObject mappedFields = fields == null ? null : queryMapper.getMappedObject(fields, entity);

		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug(String.format("findOne using query: %s fields: %s for class: %s in collection: %s",
					serializeToJsonSafely(query), mappedFields, entityClass, collectionName));
		}

		return executeFindOneInternal(new FindOneCallback(mappedQuery, mappedFields), new ReadDbObjectCallback<T>(
				this.sequoiadbConverter, entityClass), collectionName);
	}

	/**
	 * Map the results of an ad-hoc query on the default SequoiaDB collection to a List using the template's converter. The
	 * query document is specified as a standard BSONObject and so is the fields specification.
	 * 
	 * @param collectionName name of the collection to retrieve the objects from
	 * @param query the query document that specifies the criteria used to find a record
	 * @param fields the document that specifies the fields to be returned
	 * @param entityClass the parameterized type of the returned list.
	 * @return the List of converted objects.
	 */
	protected <T> List<T> doFind(String collectionName, BSONObject query, BSONObject fields, Class<T> entityClass) {
		return doFind(collectionName, query, fields, entityClass, null, new ReadDbObjectCallback<T>(this.sequoiadbConverter,
				entityClass));
	}

	/**
	 * Map the results of an ad-hoc query on the default SequoiaDB collection to a List of the specified type. The object is
	 * converted from the SequoiaDB native representation using an instance of {@see SequoiadbConverter}. The query document is
	 * specified as a standard BSONObject and so is the fields specification.
	 * 
	 * @param collectionName name of the collection to retrieve the objects from.
	 * @param query the query document that specifies the criteria used to find a record.
	 * @param fields the document that specifies the fields to be returned.
	 * @param entityClass the parameterized type of the returned list.
	 * @param preparer allows for customization of the {@link DBCursor} used when iterating over the result set, (apply
	 *          limits, skips and so on).
	 * @return the {@link List} of converted objects.
	 */
	protected <T> List<T> doFind(String collectionName, BSONObject query, BSONObject fields, Class<T> entityClass,
			CursorPreparer preparer) {
		return doFind(collectionName, query, fields, entityClass, preparer, new ReadDbObjectCallback<T>(sequoiadbConverter,
				entityClass));
	}

	protected <S, T> List<T> doFind(String collectionName, BSONObject query, BSONObject fields, Class<S> entityClass,
			CursorPreparer preparer, DbObjectCallback<T> objectCallback) {

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(entityClass);

		BSONObject mappedFields = queryMapper.getMappedFields(fields, entity);
		BSONObject mappedQuery = queryMapper.getMappedObject(query, entity);
		BSONObject sortObj = preparer.getType() != null ? getMappedSortObject(preparer.getQuery(), preparer.getType()) : preparer.getQuery().getSortObject();
		BSONObject hintObj = preparer.getQuery().getHintObject();
		int skipRows = preparer.getQuery().getSkip();
		int returnRows = preparer.getQuery().getLimit();
		int flags = preparer.getQuery().getQueryFlags();

		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug(String.format(
					"find using query: %s, fields: %s, sort: %s, hint: %s, skip: %d, limit: %d, flags: %d, for class: %s in collection: %s",
					mappedFields, mappedFields, sortObj, hintObj, skipRows, returnRows, flags, entityClass, collectionName));
		}

		return executeFindMultiInternal(
				new FindCallback(mappedQuery, mappedFields, sortObj, hintObj, skipRows, returnRows, flags),
				null, objectCallback, collectionName);
	}

	protected BSONObject convertToDbObject(CollectionOptions collectionOptions) {
		BSONObject dbo = new BasicBSONObject();
		if (collectionOptions != null) {
			if (collectionOptions.getCapped() != null) {
				dbo.put("capped", collectionOptions.getCapped().booleanValue());
			}
			if (collectionOptions.getSize() != null) {
				dbo.put("size", collectionOptions.getSize().intValue());
			}
			if (collectionOptions.getMaxDocuments() != null) {
				dbo.put("max", collectionOptions.getMaxDocuments().intValue());
			}
		}
		return dbo;
	}

	/**
	 * Map the results of an ad-hoc query on the default SequoiaDB collection to an object using the template's converter.
	 * The first document that matches the query is returned and also removed from the collection in the database.
	 * <p/>
	 * The query document is specified as a standard BSONObject and so is the fields specification.
	 * 
	 * @param collectionName name of the collection to retrieve the objects from
	 * @param query the query document that specifies the criteria used to find a record
	 * @param entityClass the parameterized type of the returned list.
	 * @return the List of converted objects.
	 */
	protected <T> T doFindAndRemove(String collectionName, BSONObject query, BSONObject fields, BSONObject sort,
			Class<T> entityClass) {
		EntityReader<? super T, BSONObject> readerToUse = this.sequoiadbConverter;
		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug("findAndRemove using query: " + query + " fields: " + fields + " sort: " + sort + " for class: "
					+ entityClass + " in collection: " + collectionName);
		}
		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(entityClass);
		return executeFindOneInternal(new FindAndRemoveCallback(queryMapper.getMappedObject(query, entity), fields, sort),
				new ReadDbObjectCallback<T>(readerToUse, entityClass), collectionName);
	}

	protected <T> T doFindAndModify(String collectionName, BSONObject query, BSONObject fields, BSONObject sort,
			Class<T> entityClass, Update update, FindAndModifyOptions options) {

		EntityReader<? super T, BSONObject> readerToUse = this.sequoiadbConverter;

		if (options == null) {
			options = new FindAndModifyOptions();
		}

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(entityClass);

		increaseVersionForUpdateIfNecessary(entity, update);

		BSONObject mappedQuery = queryMapper.getMappedObject(query, entity);
		BSONObject mappedUpdate = updateMapper.getMappedObject(update.getUpdateObject(), entity);

		if (LOGGER.isDebugEnabled()) {
			LOGGER.debug("findAndModify using query: " + mappedQuery + " fields: " + fields + " sort: " + sort
					+ " for class: " + entityClass + " and update: " + mappedUpdate + " in collection: " + collectionName);
		}

		return executeFindOneInternal(new FindAndModifyCallback(mappedQuery, fields, sort, mappedUpdate, options),
				new ReadDbObjectCallback<T>(readerToUse, entityClass), collectionName);
	}

	/**
	 * Populates the id property of the saved object, if it's not set already.
	 * 
	 * @param savedObject
	 * @param id
	 */
	protected void populateIdIfNecessary(Object savedObject, Object id) {

		if (id == null) {
			return;
		}

		if (savedObject instanceof BasicBSONObject) {
			BSONObject dbObject = (BSONObject) savedObject;
			dbObject.put(ID_FIELD, id);
			return;
		}

		SequoiadbPersistentProperty idProp = getIdPropertyFor(savedObject.getClass());

		if (idProp == null) {
			return;
		}

		ConversionService conversionService = sequoiadbConverter.getConversionService();
		BeanWrapper<Object> wrapper = BeanWrapper.create(savedObject, conversionService);

		Object idValue = wrapper.getProperty(idProp, idProp.getType());

		if (idValue != null) {
			return;
		}

		wrapper.setProperty(idProp, id);
	}

	private DBCollection getAndPrepareCollection(DB db, String collectionName) {
		try {
			DBCollection collection = db.getCollection(collectionName);
			prepareCollection(collection);
			return collection;
		} catch (RuntimeException e) {
			throw potentiallyConvertRuntimeException(e);
		}
	}

	/**
	 * Internal method using callbacks to do queries against the datastore that requires reading a single object from a
	 * collection of objects. It will take the following steps
	 * <ol>
	 * <li>Execute the given {@link ConnectionCallback} for a {@link BSONObject}.</li>
	 * <li>Apply the given {@link DbObjectCallback} to each of the {@link BSONObject}s to obtain the result.</li>
	 * <ol>
	 * 
	 * @param <T>
	 * @param collectionCallback the callback to retrieve the {@link BSONObject} with
	 * @param objectCallback the {@link DbObjectCallback} to transform {@link BSONObject}s into the actual domain type
	 * @param collectionName the collection to be queried
	 * @return
	 */
	private <T> T executeFindOneInternal(CollectionCallback<BSONObject> collectionCallback,
			DbObjectCallback<T> objectCallback, String collectionName) {

		try {
			T result = objectCallback.doWith(collectionCallback.doInCollection(getAndPrepareCollection(getDb(),
					collectionName)));
			return result;
		} catch (RuntimeException e) {
			throw potentiallyConvertRuntimeException(e);
		}
	}

	/**
	 * Internal method using callback to do queries against the datastore that requires reading a collection of objects.
	 * It will take the following steps
	 * <ol>
	 * <li>Execute the given {@link ConnectionCallback} for a {@link DBCursor}.</li>
	 * <li>Prepare that {@link DBCursor} with the given {@link CursorPreparer} (will be skipped if {@link CursorPreparer}
	 * is {@literal null}</li>
	 * <li>Iterate over the {@link DBCursor} and applies the given {@link DbObjectCallback} to each of the
	 * {@link BSONObject}s collecting the actual result {@link List}.</li>
	 * <ol>
	 * 
	 * @param <T>
	 * @param collectionCallback the callback to retrieve the {@link DBCursor} with
	 * @param preparer the {@link CursorPreparer} to potentially modify the {@link DBCursor} before ireating over it
	 * @param objectCallback the {@link DbObjectCallback} to transform {@link BSONObject}s into the actual domain type
	 * @param collectionName the collection to be queried
	 * @return
	 */
	private <T> List<T> executeFindMultiInternal(CollectionCallback<DBCursor> collectionCallback, CursorPreparer preparer,
			DbObjectCallback<T> objectCallback, String collectionName) {

		try {

			DBCursor cursor = null;

			try {
				cursor = collectionCallback.doInCollection(getAndPrepareCollection(getDb(), collectionName));
				List<T> result = new ArrayList<T>();
				while (cursor.hasNext()) {
					BSONObject object = cursor.next();
					result.add(objectCallback.doWith(object));
				}
				return result;

			} finally {

				if (cursor != null) {
					cursor.close();
				}
			}
		} catch (RuntimeException e) {
			throw potentiallyConvertRuntimeException(e);
		}
	}

	private void executeQueryInternal(CollectionCallback<DBCursor> collectionCallback, CursorPreparer preparer,
			DocumentCallbackHandler callbackHandler, String collectionName) {

		try {
			DBCursor cursor = null;

			try {
				cursor = collectionCallback.doInCollection(getAndPrepareCollection(getDb(), collectionName));
				while (cursor.hasNext()) {
					BSONObject dbobject = cursor.next();
					callbackHandler.processDocument(dbobject);
				}
			} finally {
				if (cursor != null) {
					cursor.close();
				}
			}
		} catch (RuntimeException e) {
			throw potentiallyConvertRuntimeException(e);
		}
	}

	private SequoiadbPersistentEntity<?> getPersistentEntity(Class<?> type) {
		return type == null ? null : mappingContext.getPersistentEntity(type);
	}

	private SequoiadbPersistentProperty getIdPropertyFor(Class<?> type) {
		SequoiadbPersistentEntity<?> persistentEntity = mappingContext.getPersistentEntity(type);
		return persistentEntity == null ? null : persistentEntity.getIdProperty();
	}

	private <T> String determineEntityCollectionName(T obj) {
		if (null != obj) {
			return determineCollectionName(obj.getClass());
		}

		return null;
	}

	String determineCollectionName(Class<?> entityClass) {

		if (entityClass == null) {
			throw new InvalidDataAccessApiUsageException(
					"No class parameter provided, entity collection can't be determined!");
		}

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(entityClass);
		if (entity == null) {
			throw new InvalidDataAccessApiUsageException("No Persitent Entity information found for the class "
					+ entityClass.getName());
		}
		return entity.getCollection();
	}

	/**
	 * Handles {@link WriteResult} errors based on the configured {@link WriteResultChecking}.
	 * 
	 * @param writeResult
	 * @param query
	 * @param operation
	 */
	protected void handleAnyWriteResultErrors(WriteResult writeResult, BSONObject query, SequoiadbActionOperation operation) {

		if (writeResultChecking == WriteResultChecking.NONE) {
			return;
		}

		String error = writeResult.getError();

		if (error == null) {
			return;
		}

		String message;

		switch (operation) {

			case INSERT:
			case SAVE:
				message = String.format("Insert/Save for %s failed: %s", query, error);
				break;
			case INSERT_LIST:
				message = String.format("Insert list failed: %s", error);
				break;
			default:
				message = String.format("Execution of %s%s failed: %s", operation,
						query == null ? "" : " using query " + query.toString(), error);
		}

		if (writeResultChecking == WriteResultChecking.EXCEPTION) {
			throw new SequoiadbDataIntegrityViolationException(message, writeResult, operation);
		} else {
			LOGGER.error(message);
			return;
		}
	}

	/**
	 * Tries to convert the given {@link RuntimeException} into a {@link DataAccessException} but returns the original
	 * exception if the conversation failed. Thus allows safe rethrowing of the return value.
	 * 
	 * @param ex
	 * @return
	 */
	private RuntimeException potentiallyConvertRuntimeException(RuntimeException ex) {
		RuntimeException resolved = this.exceptionTranslator.translateExceptionIfPossible(ex);
		return resolved == null ? ex : resolved;
	}

	/**
	 * Inspects the given {@link CommandResult} for erros and potentially throws an
	 * {@link InvalidDataAccessApiUsageException} for that error.
	 * 
	 * @param result must not be {@literal null}.
	 * @param source must not be {@literal null}.
	 */
	private void handleCommandError(CommandResult result, BSONObject source) {

		try {
			result.throwOnError();
		} catch (BaseException ex) {

			String error = result.getErrorMessage();
			error = error == null ? "NO MESSAGE" : error;

			throw new InvalidDataAccessApiUsageException("Command execution failed:  Error [" + error + "], Command = "
					+ source, ex);
		}
	}

	private static final SequoiadbConverter getDefaultSequoiadbConverter(SequoiadbFactory factory) {

		DbRefResolver dbRefResolver = new DefaultDbRefResolver(factory);
		MappingSequoiadbConverter converter = new MappingSequoiadbConverter(dbRefResolver, new SequoiadbMappingContext());
		converter.afterPropertiesSet();
		return converter;
	}

	private BSONObject getMappedSortObject(Query query, Class<?> type) {

		if (query == null || query.getSortObject() == null) {
			return null;
		}

		return queryMapper.getMappedSort(query.getSortObject(), mappingContext.getPersistentEntity(type));
	}


	/**
	 * Simple {@link CollectionCallback} that takes a query {@link BSONObject} plus an optional fields specification
	 * {@link BSONObject} and executes that against the {@link DBCollection}.
	 * 


	 */
	private static class FindOneCallback implements CollectionCallback<BSONObject> {

		private final BSONObject query;
		private final BSONObject fields;

		public FindOneCallback(BSONObject query, BSONObject fields) {
			this.query = query;
			this.fields = fields;
		}

		public BSONObject doInCollection(DBCollection collection) throws BaseException, DataAccessException {
			if (fields == null) {
				if (LOGGER.isDebugEnabled()) {
					LOGGER.debug("findOne using query: " + query + " in db.collection: " + collection.getFullName());
				}
				return collection.findOne(query);
			} else {
				if (LOGGER.isDebugEnabled()) {
					LOGGER.debug("findOne using query: " + query + " fields: " + fields + " in db.collection: "
							+ collection.getFullName());
				}
				return collection.findOne(query, fields);
			}
		}
	}

	/**
	 * Simple {@link CollectionCallback} that takes a query {@link BSONObject} plus an optional fields specification
	 * {@link BSONObject} and executes that against the {@link DBCollection}.
	 *
	 */
	private static class FindCallback implements CollectionCallback<DBCursor> {

		private final BSONObject _matcher;
		private final BSONObject _selector;
		private final BSONObject _order;
		private final BSONObject _hint;
		private final int _skipRows;
		private final int _returnRows;
		private final int _flags;

		public FindCallback(BSONObject query) {
			this(query, null, null, null, 0, -1, 0);
		}

		public FindCallback(BSONObject query, BSONObject fields) {
			this(query, fields, null, null, 0, -1, 0);
		}

		public FindCallback(BSONObject query, BSONObject fields, BSONObject order, BSONObject hint,
							int skipRows, int returnRows, int flags) {
			this._matcher = query;
			this._selector = fields;
			this._order = order;
			this._hint = hint;
			this._skipRows = skipRows;
			this._returnRows = returnRows < 0 ? -1 : returnRows;
			this._flags = flags;
		}

		public DBCursor doInCollection(DBCollection collection) throws BaseException, DataAccessException {
			return collection.find(this._matcher, this._selector, this._order, this._hint,
					this._skipRows, this._returnRows, this._flags);
		}
	}

	/**
	 * Simple {@link CollectionCallback} that takes a query {@link BSONObject} plus an optional fields specification
	 * {@link BSONObject} and executes that against the {@link DBCollection}.
	 * 

	 */
	private static class FindAndRemoveCallback implements CollectionCallback<BSONObject> {

		private final BSONObject query;
		private final BSONObject fields;
		private final BSONObject sort;

		public FindAndRemoveCallback(BSONObject query, BSONObject fields, BSONObject sort) {
			this.query = query;
			this.fields = fields;
			this.sort = sort;
		}

		public BSONObject doInCollection(DBCollection collection) throws BaseException, DataAccessException {
			return collection.findAndModify(query, fields, sort, true, null, false, false);
		}
	}

	private static class FindAndModifyCallback implements CollectionCallback<BSONObject> {

		private final BSONObject query;
		private final BSONObject fields;
		private final BSONObject sort;
		private final BSONObject update;
		private final FindAndModifyOptions options;

		public FindAndModifyCallback(BSONObject query, BSONObject fields, BSONObject sort, BSONObject update,
				FindAndModifyOptions options) {
			this.query = query;
			this.fields = fields;
			this.sort = sort;
			this.update = update;
			this.options = options;
		}

		public BSONObject doInCollection(DBCollection collection) throws BaseException, DataAccessException {
			return collection.findAndModify(query, fields, sort, options.isRemove(), update, options.isReturnNew(),
					options.isUpsert());
		}
	}

	/**
	 * Simple internal callback to allow operations on a {@link BSONObject}.
	 * 

	 */

	private interface DbObjectCallback<T> {

		T doWith(BSONObject object);
	}

	/**
	 * Simple {@link DbObjectCallback} that will transform {@link BSONObject} into the given target type using the given
	 * {@link SequoiadbReader}.
	 * 

	 */
	private class ReadDbObjectCallback<T> implements DbObjectCallback<T> {

		private final EntityReader<? super T, BSONObject> reader;
		private final Class<T> type;

		public ReadDbObjectCallback(EntityReader<? super T, BSONObject> reader, Class<T> type) {
			Assert.notNull(reader);
			Assert.notNull(type);
			this.reader = reader;
			this.type = type;
		}

		public T doWith(BSONObject object) {
			if (null != object) {
				maybeEmitEvent(new AfterLoadEvent<T>(object, type));
			}
			T source = reader.read(type, object);
			if (null != source) {
				maybeEmitEvent(new AfterConvertEvent<T>(object, source));
			}
			return source;
		}
	}

	class UnwrapAndReadDbObjectCallback<T> extends ReadDbObjectCallback<T> {

		public UnwrapAndReadDbObjectCallback(EntityReader<? super T, BSONObject> reader, Class<T> type) {
			super(reader, type);
		}

		@Override
		public T doWith(BSONObject object) {

			Object idField = object.get(Fields.UNDERSCORE_ID);

			if (!(idField instanceof BSONObject)) {
				return super.doWith(object);
			}

			BSONObject toMap = new BasicBSONObject();
			BSONObject nested = (BSONObject) idField;
			toMap.putAll(nested);

			for (String key : object.keySet()) {
				if (!Fields.UNDERSCORE_ID.equals(key)) {
					toMap.put(key, object.get(key));
				}
			}

			return super.doWith(toMap);
		}
	}

	private enum DefaultWriteConcernResolver implements WriteConcernResolver {

		INSTANCE;

		public WriteConcern resolve(SequoiadbAction action) {
			return action.getDefaultWriteConcern();
		}
	}

	class QueryCursorPreparer implements CursorPreparer {

		private final Query query;
		private final Class<?> type;

		public QueryCursorPreparer(Query query, Class<?> type) {

			this.query = query;
			this.type = type;
		}

		public Query getQuery() {
			return this.query;
		}

		public Class<?> getType() { return this.type; }

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.CursorPreparer#prepare(com.sequoiadb.DBCursor)
		 */
		public DBCursor prepare(DBCursor cursor) {

			return cursor;
		}
	}

	/**
	 * {@link DbObjectCallback} that assumes a {@link GeoResult} to be created, delegates actual content unmarshalling to
	 * a delegate and creates a {@link GeoResult} from the result.
	 * 

	 */
	static class GeoNearResultDbObjectCallback<T> implements DbObjectCallback<GeoResult<T>> {

		private final DbObjectCallback<T> delegate;
		private final Metric metric;

		/**
		 * Creates a new {@link GeoNearResultDbObjectCallback} using the given {@link DbObjectCallback} delegate for
		 * {@link GeoResult} content unmarshalling.
		 * 
		 * @param delegate must not be {@literal null}.
		 */
		public GeoNearResultDbObjectCallback(DbObjectCallback<T> delegate, Metric metric) {
			Assert.notNull(delegate);
			this.delegate = delegate;
			this.metric = metric;
		}

		public GeoResult<T> doWith(BSONObject object) {

			double distance = ((Double) object.get("dis")).doubleValue();
			BSONObject content = (BSONObject) object.get("obj");

			T doWith = delegate.doWith(content);

			return new GeoResult<T>(doWith, new Distance(distance, metric));
		}
	}

}
