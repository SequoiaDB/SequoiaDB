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
package org.springframework.data.sequoiadb.core;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;
import static org.springframework.data.sequoiadb.core.query.Criteria.*;
import static org.springframework.data.sequoiadb.core.query.Query.*;
import static org.springframework.data.sequoiadb.core.query.Update.*;

import java.math.BigInteger;
import java.util.*;

import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.joda.time.DateTime;
import org.junit.*;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.convert.converter.Converter;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.dao.InvalidDataAccessApiUsageException;
import org.springframework.dao.OptimisticLockingFailureException;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.PersistenceConstructor;
import org.springframework.data.annotation.Version;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.data.domain.Sort.Direction;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.sequoiadb.InvalidSequoiadbApiUsageException;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.convert.*;
import org.springframework.data.sequoiadb.core.convert.MappingSequoiadbConverter;
import org.springframework.data.sequoiadb.core.index.Index;
import org.springframework.data.sequoiadb.core.index.Index.Duplicates;
import org.springframework.data.sequoiadb.core.index.IndexField;
import org.springframework.data.sequoiadb.core.index.IndexInfo;
import org.springframework.data.sequoiadb.core.mapping.Field;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbMappingContext;
import org.springframework.data.sequoiadb.core.query.BasicQuery;
import org.springframework.data.sequoiadb.core.query.Criteria;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.core.query.Update;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;
import org.springframework.util.StringUtils;

/**
 * Integration test for {@link SequoiadbTemplate}.
 *
 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration("classpath:infrastructure.xml")
public class SequoiadbTemplateTests {

	private static final org.springframework.data.util.Version TWO_DOT_FOUR = org.springframework.data.util.Version
			.parse("2.4");

	@Autowired
	SequoiadbTemplate template;
	@Autowired
	SequoiadbFactory factory;

	SequoiadbTemplate mappingTemplate;
	org.springframework.data.util.Version sequoiadbVersion;

	@Rule public ExpectedException thrown = ExpectedException.none();

	@Autowired
	@SuppressWarnings("unchecked")
	public void setSequoiadb(Sdb sdb) throws Exception {

		CustomConversions conversions = new CustomConversions(Arrays.asList(DateToDateTimeConverter.INSTANCE,
				DateTimeToDateConverter.INSTANCE));

		SequoiadbMappingContext mappingContext = new SequoiadbMappingContext();
		mappingContext.setInitialEntitySet(new HashSet<Class<?>>(Arrays.asList(PersonWith_idPropertyOfTypeObjectId.class,
				PersonWith_idPropertyOfTypeString.class, PersonWithIdPropertyOfTypeObjectId.class,
				PersonWithIdPropertyOfTypeString.class, PersonWithIdPropertyOfTypeInteger.class,
				PersonWithIdPropertyOfTypeBigInteger.class, PersonWithIdPropertyOfPrimitiveInt.class,
				PersonWithIdPropertyOfTypeLong.class, PersonWithIdPropertyOfPrimitiveLong.class)));
		mappingContext.setSimpleTypeHolder(conversions.getSimpleTypeHolder());
		mappingContext.initialize();

		DbRefResolver dbRefResolver = new DefaultDbRefResolver(factory);
		MappingSequoiadbConverter mappingConverter = new MappingSequoiadbConverter(dbRefResolver, mappingContext);
		mappingConverter.setCustomConversions(conversions);
		mappingConverter.afterPropertiesSet();

		this.mappingTemplate = new SequoiadbTemplate(factory, mappingConverter);
	}

	@Before
	public void setUp() {
		cleanDb();
		querySequoiadbVersionIfNecessary();
	}

	@After
	public void cleanUp() {
		cleanDb();
	}

	private void prepareCollection(String clName) {
		if (template.collectionExists(clName)) {
			template.dropCollection(clName);
		}
		template.createCollection(clName);
	}

	private <T> void prepareCollection(Class<T> entityClass) {
		if (template.collectionExists(entityClass)) {
			template.dropCollection(entityClass);
		}
		template.createCollection(entityClass);
	}

	private void querySequoiadbVersionIfNecessary() {

		if (sequoiadbVersion == null) {
			sequoiadbVersion = org.springframework.data.util.Version.parse("2.10");
		}
	}

	protected void cleanDb() {
		List<Class<?>> classList = new ArrayList<Class<?>>();
		List<String> clNameList = new ArrayList<String>();

		classList.add(Person.class);
		classList.add(PersonWithAList.class);
		classList.add(PersonWith_idPropertyOfTypeObjectId.class);
		classList.add(PersonWith_idPropertyOfTypeString.class);
		classList.add(PersonWithIdPropertyOfTypeObjectId.class);
		classList.add(PersonWithIdPropertyOfTypeString.class);
		classList.add(PersonWithIdPropertyOfTypeInteger.class);
		classList.add(PersonWithIdPropertyOfTypeBigInteger.class);
		classList.add(PersonWithIdPropertyOfPrimitiveInt.class);
		classList.add(PersonWithIdPropertyOfTypeLong.class);
		classList.add(PersonWithIdPropertyOfPrimitiveLong.class);
		classList.add(PersonWithVersionPropertyOfTypeInteger.class);
		classList.add(TestClass.class);
		classList.add(Sample.class);
		classList.add(MyPerson.class);
		classList.add(TypeWithFieldAnnotation.class);
		classList.add(TypeWithDate.class);
		classList.add(Document.class);
		classList.add(ObjectWith3AliasedFields.class);
		classList.add(ObjectWith3AliasedFieldsAndNestedAddress.class);
		classList.add(BaseDoc.class);
		classList.add(ObjectWithEnumValue.class);
		classList.add(DocumentWithCollection.class);
		classList.add(DocumentWithCollectionOfSimpleType.class);
		classList.add(DocumentWithMultipleCollections.class);
		classList.add(DocumentWithDBRefCollection.class);
		classList.add(SomeContent.class);
		classList.add(SomeTemplate.class);
		classList.add(VersionedPerson.class);
		clNameList.add("collection");
		clNameList.add("personX");
		clNameList.add("test");

		for(int i = 0; i < classList.size(); i++) {
			try {
				template.dropCollection(classList.get(i));
			} catch(Exception e) {
			}
		}
		for(int i = 0; i < clNameList.size(); i++) {
			try {
				template.dropCollection(clNameList.get(i));
			} catch(Exception e) {
			}
		}
	}

    @Test
    public void CRUDTest() {
        prepareCollection("test");
        DBCollection cl = template.getCollection("test");
        cl.insert(new BasicBSONObject().append("a", 1));
        DBCursor cursor = cl.find(new BasicBSONObject().append("a", new BasicBSONObject("$gt", 0)),
                            new BasicBSONObject().append("_id", new BasicBSONObject("$include", 0)),
                null, null,
                    0, -1, 0);
        System.out.println(String.format("idle: %d, used: %d", template.getDb().getSdb().getIdleConnCount(),
                template.getDb().getSdb().getUsedConnCount()));
        while(cursor.hasNext()) {
            System.out.println("after insert, record is: " + cursor.next().toString());
        }
        System.out.println(String.format("idle: %d, used: %d", template.getDb().getSdb().getIdleConnCount(),
                template.getDb().getSdb().getUsedConnCount()));
        cl.update(null, new BasicBSONObject().append("$set", new BasicBSONObject("a", 2)), null, false );
        cursor = cl.find();
        System.out.println(String.format("idle: %d, used: %d", template.getDb().getSdb().getIdleConnCount(),
                template.getDb().getSdb().getUsedConnCount()));
        try {
            while (cursor.hasNext()) {
                System.out.println("after update, record is: " + cursor.next().toString());
            }
            System.out.println(String.format("idle: %d, used: %d", template.getDb().getSdb().getIdleConnCount(),
                    template.getDb().getSdb().getUsedConnCount()));
        } finally {
            cursor.close();
        }
        System.out.println(String.format("idle: %d, used: %d", template.getDb().getSdb().getIdleConnCount(),
                template.getDb().getSdb().getUsedConnCount()));
        cl.remove(new BasicBSONObject().append("a", new BasicBSONObject("$gt", 1)));
        cursor = cl.find();
        Assert.assertFalse(cursor.hasNext());
    }

	@Test
    public void cursorCloseTest() {
        prepareCollection(Person.class);
        Sdb sdb = template.getDb().getSdb();
        Person person = new Person("Sam");
        person.setAge(25);
        Assert.assertEquals(0, sdb.getUsedConnCount());
        template.insert(person);
        Assert.assertEquals(0, sdb.getUsedConnCount());

        DBCollection cl = template.getCollection(Person.class);
        DBCursor cursor = cl.find();
        Assert.assertEquals(1, sdb.getUsedConnCount());
        while(cursor.hasNext()) {
            System.out.println("after insert, record is: " + cursor.next().toString());
        }
        Assert.assertEquals(0, sdb.getUsedConnCount());

        cursor = cl.find();
        Assert.assertEquals(1, sdb.getUsedConnCount());
        try {
            while (cursor.hasNext()) {
                System.out.println("after update, record is: " + cursor.next().toString());
            }
            Assert.assertEquals(0, sdb.getUsedConnCount());
        } finally {
            cursor.close();
        }
        Assert.assertEquals(0, sdb.getUsedConnCount());
    }

	@Test
    public void queryWithHintTest() {
        prepareCollection(Person.class);
        String indexName = "age_1_firstname_-1";
        Person person = new Person("Sam");
        person.setAge(25);
        Person person2 = new Person("Tom");
        person2.setAge(26);
        template.insert(person);
        template.insert(person2);
        DBCollection cl = template.getCollection(Person.class);
        cl.createIndex(new BasicBSONObject("age", 1).append("firstname", -1));
        List<BSONObject> indexes = cl.getIndexInfo();
        for(BSONObject idx : indexes) {
            System.out.println(String.format("Idx is: %s", idx.toString()));
        }
        Query query = new Query();
        query.addCriteria(Criteria.where("age").gt(0));
        query.withHint("$id", indexName);
        List<Person> results = template.find(query, Person.class);
        for(Person p : results) {
            System.out.println("person is: " + p.toString());
        }
        query = new Query();
        query.addCriteria(Criteria.where("age").gt(0));
        query.withHint(null);
        results = template.find(query, Person.class);
        for(Person p : results) {
            System.out.println("person is: " + p.toString());
        }
    }

	@Test
	public void executeSqlCommand() {
		prepareCollection("test");
		DB db = template.getDb();
		String selectSqlString = "select join_set2.teleActivityId, join_set2.totalAllot, join_set2.totalCallNum, join_set2.totalExecute, join_set2.totalConnect, t4.notDistributeNameCount from ( select join_set1.teleActivityId, join_set1.totalAllot, join_set1.totalCallNum, join_set1.totalExecute, t3.totalConnect from ( select t1.teleActivityId, t1.totalAllot, t1.totalCallNum, t2.totalExecute from ( select teleActivityId, count(callNum) as totalAllot, sum(callNum) as totalCallNum from database.test group by teleActivityId ) as t1 left outer join ( select teleActivityId, count(status) as totalExecute from database.test where status = '6'  group by teleActivityId ) as t2 on t1.teleActivityId = t2.teleActivityId ) as join_set1 left outer join ( select teleActivityId, count(isConnect) as totalConnect from database.test where isConnect = '1'  group by teleActivityId ) as t3 on join_set1.teleActivityId = t3.teleActivityId ) as join_set2 left outer join ( select teleActivityId, count(status) as notDistributeNameCount from database.test where status = '1'  group by teleActivityId ) as t4 on join_set2.teleActivityId = t4.teleActivityId";
		DBCursor cursor = db.executeSelectSql(selectSqlString);
		try {
			while (cursor.hasNext()) {
				System.out.println(cursor.next());
			}
		} finally {
			cursor.close();
		}
		long time = new Date().getTime();
		String createCSString = "create collectionspace foo" + time;
		String dropCSString = "drop collectionspace foo" + time;
		db.executeOtherSql(createCSString);
		db.executeOtherSql(dropCSString);
	}

	@Test
	public void cursorTest() {
		template.createCollection("test");
		DB db = template.getDb();
		DBCollection cl = template.getCollection("test");
		DBCursor cursor = cl.find(null, null, null, null,0, 10, 0);
		try {
			while (cursor.hasNext()) {
				System.out.println(cursor.next());
			}
		} finally {
			cursor.close();
		}
	}

	@Test
	public void aggregate() {
		prepareCollection(Person.class);
		Person person = new Person("Oliver");
		person.setAge(25);
		Person person2 = new Person("Oliver");
		person.setAge(25);
		template.insert(person);
		template.insert(person2);
		List<BSONObject> list = new ArrayList<BSONObject>();
		list.add(new BasicBSONObject("$project", new BasicBSONObject("age", 1).append("a", 1)));
		AggregationOutput output = template.getCollection("person").aggregate(list);
		Iterator<BSONObject> itr = output.results().iterator();
		while(itr.hasNext()) {
			System.out.println(itr.next());
		}
	}

	@Test
	public void insertsSimpleEntityCorrectly() throws Exception {
		prepareCollection(Person.class);
		Person person = new Person("Oliver");
		person.setAge(25);
		template.insert(person);

		List<Person> result = template.find(new Query(Criteria.where("_id").is(person.getId())), Person.class);
		assertThat(result.size(), is(1));
		assertThat(result, hasItem(person));
	}

	@Test
	public void bogusUpdateDoesNotTriggerException() throws Exception {
		prepareCollection(Person.class);
		SequoiadbTemplate sequoiadbTemplate = new SequoiadbTemplate(factory);
		sequoiadbTemplate.setWriteResultChecking(WriteResultChecking.EXCEPTION);

		Person person = new Person("Oliver2");
		person.setAge(25);
		sequoiadbTemplate.insert(person);

		Query q = new Query(Criteria.where("BOGUS").gt(22));
		Update u = new Update().set("firstName", "Sven");
		sequoiadbTemplate.updateFirst(q, u, Person.class);
	}

	/**
	 * @see DATA_JIRA-480
	 */
	@Test
	public void throwsExceptionForDuplicateIds() {
		thrown.expect(DuplicateKeyException.class);
		prepareCollection(Person.class);
		SequoiadbTemplate template = new SequoiadbTemplate(factory);
		template.setWriteResultChecking(WriteResultChecking.EXCEPTION);

		Person person = new Person(new ObjectId(), "Amol");
		person.setAge(28);

		template.insert(person);
		template.insert(person);

	}

	/**
	 * @see DATA_JIRA-480
	 * @see DATA_JIRA-799
	 */
	@Test
	@Ignore // no exception will be throw in sdb
	public void throwsExceptionForUpdateWithInvalidPushOperator() {
		prepareCollection(Person.class);
		SequoiadbTemplate template = new SequoiadbTemplate(factory);
		template.setWriteResultChecking(WriteResultChecking.EXCEPTION);

		ObjectId id = new ObjectId();
		Person person = new Person(id, "Amol");
		person.setAge(28);

		template.insert(person);

		thrown.expect(DataIntegrityViolationException.class);
		thrown.expectMessage("Execution");
		thrown.expectMessage("UPDATE");
		thrown.expectMessage("array");
		thrown.expectMessage("firstName");
		thrown.expectMessage("failed");

		Query query = new Query(Criteria.where("firstName").is("Amol"));
		Update upd = new Update().push("age", 29);
		template.updateFirst(query, upd, Person.class);
	}

	/**
	 * @see DATA_JIRA-480
	 */
	@Test
	public void throwsExceptionForIndexViolationIfConfigured() {
		thrown.expect(DuplicateKeyException.class);
		prepareCollection(Person.class);
		SequoiadbTemplate template = new SequoiadbTemplate(factory);
		template.setWriteResultChecking(WriteResultChecking.EXCEPTION);
		template.indexOps(Person.class).ensureIndex(new Index().on("firstName", Direction.DESC).unique());

		Person person = new Person(new ObjectId(), "Amol");
		person.setAge(28);

		template.save(person);

		person = new Person(new ObjectId(), "Amol");
		person.setAge(28);

		template.save(person);
	}

	/**
	 * @see DATA_JIRA-480
	 */
	@Test
	public void rejectsDuplicateIdInInsertAll() {


		thrown.expect(DuplicateKeyException.class);
		prepareCollection(Person.class);
		SequoiadbTemplate template = new SequoiadbTemplate(factory);
		template.setWriteResultChecking(WriteResultChecking.EXCEPTION);

		ObjectId id = new ObjectId();
		Person person = new Person(id, "Amol");
		person.setAge(28);

		List<Person> records = new ArrayList<Person>();
		records.add(person);
		records.add(person);

		template.insertAll(records);
	}

	@Test
	public void testEnsureIndex() throws Exception {
		prepareCollection(Person.class);
		Person p1 = new Person("Oliver");
		p1.setAge(25);
		template.insert(p1);
		Person p2 = new Person("Sven");
		p2.setAge(40);
		template.insert(p2);

		template.indexOps(Person.class).ensureIndex(new Index().on("age", Direction.DESC).unique(Duplicates.DROP));

		DBCollection coll = template.getCollection(template.getCollectionName(Person.class));
		List<BSONObject> indexInfo = coll.getIndexInfo();
		assertThat(indexInfo.size(), is(2));
		String indexKey = null;
		boolean unique = false;
		boolean dropDupes = false;
		for (BSONObject ix : indexInfo) {
			org.bson.BSONObject idxDef = (org.bson.BasicBSONObject)ix.get("IndexDef");
			if ("age_-1".equals(idxDef.get("name"))) {
				indexKey = idxDef.get("key").toString();
				unique = (Boolean) idxDef.get("unique");
				dropDupes = (Boolean) idxDef.get("dropDups");
			}
		}
		assertThat(indexKey, is("{ \"age\" : -1 }"));
		assertThat(unique, is(true));
		assertThat(dropDupes, is(false));

		List<IndexInfo> indexInfoList = template.indexOps(Person.class).getIndexInfo();

		assertThat(indexInfoList.size(), is(2));
		IndexInfo ii = indexInfoList.get(1);
		assertThat(ii.isUnique(), is(true));
		assertThat(ii.isDropDuplicates(), is(false));
		assertThat(ii.isSparse(), is(false));

		List<IndexField> indexFields = ii.getIndexFields();
		IndexField field = indexFields.get(0);

		assertThat(field, is(IndexField.create("age", Direction.DESC)));
	}

	/**
	 * @see DATA_JIRA-746
	 */
	@Test
	public void testReadIndexInfoForIndicesCreatedViaSequoiadbShellCommands() throws Exception {
		prepareCollection(Person.class);
		template.indexOps(Person.class).dropAllIndexes();
		assertThat(template.indexOps(Person.class).getIndexInfo().size(), is(1));

	}

	@Test
	public void testProperHandlingOfDifferentIdTypesWithMappingSequoiadbConverter() throws Exception {
		testProperHandlingOfDifferentIdTypes(this.mappingTemplate);
	}

	private void testProperHandlingOfDifferentIdTypes(SequoiadbTemplate sequoiadbTemplate) throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeString.class);
		prepareCollection(PersonWith_idPropertyOfTypeString.class);
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		prepareCollection(PersonWith_idPropertyOfTypeObjectId.class);
		prepareCollection(PersonWithIdPropertyOfTypeInteger.class);
		prepareCollection(PersonWithIdPropertyOfTypeBigInteger.class);
		prepareCollection(PersonWithIdPropertyOfPrimitiveInt.class);
		prepareCollection(PersonWithIdPropertyOfTypeLong.class);
		prepareCollection(PersonWithIdPropertyOfPrimitiveLong.class);
		PersonWithIdPropertyOfTypeString p1 = new PersonWithIdPropertyOfTypeString();
		p1.setFirstName("Sven_1");
		p1.setAge(22);
		sequoiadbTemplate.insert(p1);
		sequoiadbTemplate.save(p1);
		assertThat(p1.getId(), notNullValue());
		PersonWithIdPropertyOfTypeString p1q = sequoiadbTemplate.findOne(new Query(where("id").is(p1.getId())),
				PersonWithIdPropertyOfTypeString.class);
		assertThat(p1q, notNullValue());
		assertThat(p1q.getId(), is(p1.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeString.class, 1);

		PersonWithIdPropertyOfTypeString p2 = new PersonWithIdPropertyOfTypeString();
		p2.setFirstName("Sven_2");
		p2.setAge(22);
		p2.setId("TWO");
		sequoiadbTemplate.insert(p2);
		sequoiadbTemplate.save(p2);
		assertThat(p2.getId(), notNullValue());
		PersonWithIdPropertyOfTypeString p2q = sequoiadbTemplate.findOne(new Query(where("id").is(p2.getId())),
				PersonWithIdPropertyOfTypeString.class);
		assertThat(p2q, notNullValue());
		assertThat(p2q.getId(), is(p2.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeString.class, 2);

		PersonWith_idPropertyOfTypeString p3 = new PersonWith_idPropertyOfTypeString();
		p3.setFirstName("Sven_3");
		p3.setAge(22);
		sequoiadbTemplate.insert(p3);
		sequoiadbTemplate.save(p3);
		assertThat(p3.get_id(), notNullValue());
		PersonWith_idPropertyOfTypeString p3q = sequoiadbTemplate.findOne(new Query(where("_id").is(p3.get_id())),
				PersonWith_idPropertyOfTypeString.class);
		assertThat(p3q, notNullValue());
		assertThat(p3q.get_id(), is(p3.get_id()));
		checkCollectionContents(PersonWith_idPropertyOfTypeString.class, 1);

		PersonWith_idPropertyOfTypeString p4 = new PersonWith_idPropertyOfTypeString();
		p4.setFirstName("Sven_4");
		p4.setAge(22);
		p4.set_id("FOUR");
		sequoiadbTemplate.insert(p4);
		sequoiadbTemplate.save(p4);
		assertThat(p4.get_id(), notNullValue());
		PersonWith_idPropertyOfTypeString p4q = sequoiadbTemplate.findOne(new Query(where("_id").is(p4.get_id())),
				PersonWith_idPropertyOfTypeString.class);
		assertThat(p4q, notNullValue());
		assertThat(p4q.get_id(), is(p4.get_id()));
		checkCollectionContents(PersonWith_idPropertyOfTypeString.class, 2);

		PersonWithIdPropertyOfTypeObjectId p5 = new PersonWithIdPropertyOfTypeObjectId();
		p5.setFirstName("Sven_5");
		p5.setAge(22);
		sequoiadbTemplate.insert(p5);
		sequoiadbTemplate.save(p5);
		assertThat(p5.getId(), notNullValue());
		PersonWithIdPropertyOfTypeObjectId p5q = sequoiadbTemplate.findOne(new Query(where("id").is(p5.getId())),
				PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(p5q, notNullValue());
		assertThat(p5q.getId(), is(p5.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeObjectId.class, 1);

		PersonWithIdPropertyOfTypeObjectId p6 = new PersonWithIdPropertyOfTypeObjectId();
		p6.setFirstName("Sven_6");
		p6.setAge(22);
		p6.setId(new ObjectId());
		sequoiadbTemplate.insert(p6);
		sequoiadbTemplate.save(p6);
		assertThat(p6.getId(), notNullValue());
		PersonWithIdPropertyOfTypeObjectId p6q = sequoiadbTemplate.findOne(new Query(where("id").is(p6.getId())),
				PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(p6q, notNullValue());
		assertThat(p6q.getId(), is(p6.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeObjectId.class, 2);

		PersonWith_idPropertyOfTypeObjectId p7 = new PersonWith_idPropertyOfTypeObjectId();
		p7.setFirstName("Sven_7");
		p7.setAge(22);
		sequoiadbTemplate.insert(p7);
		sequoiadbTemplate.save(p7);
		assertThat(p7.get_id(), notNullValue());
		PersonWith_idPropertyOfTypeObjectId p7q = sequoiadbTemplate.findOne(new Query(where("_id").is(p7.get_id())),
				PersonWith_idPropertyOfTypeObjectId.class);
		assertThat(p7q, notNullValue());
		assertThat(p7q.get_id(), is(p7.get_id()));
		checkCollectionContents(PersonWith_idPropertyOfTypeObjectId.class, 1);

		PersonWith_idPropertyOfTypeObjectId p8 = new PersonWith_idPropertyOfTypeObjectId();
		p8.setFirstName("Sven_8");
		p8.setAge(22);
		p8.set_id(new ObjectId());
		sequoiadbTemplate.insert(p8);
		sequoiadbTemplate.save(p8);
		assertThat(p8.get_id(), notNullValue());
		PersonWith_idPropertyOfTypeObjectId p8q = sequoiadbTemplate.findOne(new Query(where("_id").is(p8.get_id())),
				PersonWith_idPropertyOfTypeObjectId.class);
		assertThat(p8q, notNullValue());
		assertThat(p8q.get_id(), is(p8.get_id()));
		checkCollectionContents(PersonWith_idPropertyOfTypeObjectId.class, 2);

		PersonWithIdPropertyOfTypeInteger p9 = new PersonWithIdPropertyOfTypeInteger();
		p9.setFirstName("Sven_9");
		p9.setAge(22);
		p9.setId(Integer.valueOf(12345));
		sequoiadbTemplate.insert(p9);
		sequoiadbTemplate.save(p9);
		assertThat(p9.getId(), notNullValue());
		PersonWithIdPropertyOfTypeInteger p9q = sequoiadbTemplate.findOne(new Query(where("id").in(p9.getId())),
				PersonWithIdPropertyOfTypeInteger.class);
		assertThat(p9q, notNullValue());
		assertThat(p9q.getId(), is(p9.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeInteger.class, 1);

		/*
		 * @see DATA_JIRA-602
		 */
		PersonWithIdPropertyOfTypeBigInteger p9bi = new PersonWithIdPropertyOfTypeBigInteger();
		p9bi.setFirstName("Sven_9bi");
		p9bi.setAge(22);
		p9bi.setId(BigInteger.valueOf(12345));
		sequoiadbTemplate.insert(p9bi);
		sequoiadbTemplate.save(p9bi);
		assertThat(p9bi.getId(), notNullValue());
		PersonWithIdPropertyOfTypeBigInteger p9qbi = sequoiadbTemplate.findOne(new Query(where("id").in(p9bi.getId())),
				PersonWithIdPropertyOfTypeBigInteger.class);
		assertThat(p9qbi, notNullValue());
		assertThat(p9qbi.getId(), is(p9bi.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeBigInteger.class, 1);

		PersonWithIdPropertyOfPrimitiveInt p10 = new PersonWithIdPropertyOfPrimitiveInt();
		p10.setFirstName("Sven_10");
		p10.setAge(22);
		p10.setId(12345);
		sequoiadbTemplate.insert(p10);
		sequoiadbTemplate.save(p10);
		assertThat(p10.getId(), notNullValue());
		PersonWithIdPropertyOfPrimitiveInt p10q = sequoiadbTemplate.findOne(new Query(where("id").in(p10.getId())),
				PersonWithIdPropertyOfPrimitiveInt.class);
		assertThat(p10q, notNullValue());
		assertThat(p10q.getId(), is(p10.getId()));
		checkCollectionContents(PersonWithIdPropertyOfPrimitiveInt.class, 1);

		PersonWithIdPropertyOfTypeLong p11 = new PersonWithIdPropertyOfTypeLong();
		p11.setFirstName("Sven_9");
		p11.setAge(22);
		p11.setId(Long.valueOf(12345L));
		sequoiadbTemplate.insert(p11);
		sequoiadbTemplate.save(p11);
		assertThat(p11.getId(), notNullValue());
		PersonWithIdPropertyOfTypeLong p11q = sequoiadbTemplate.findOne(new Query(where("id").in(p11.getId())),
				PersonWithIdPropertyOfTypeLong.class);
		assertThat(p11q, notNullValue());
		assertThat(p11q.getId(), is(p11.getId()));
		checkCollectionContents(PersonWithIdPropertyOfTypeLong.class, 1);

		PersonWithIdPropertyOfPrimitiveLong p12 = new PersonWithIdPropertyOfPrimitiveLong();
		p12.setFirstName("Sven_10");
		p12.setAge(22);
		p12.setId(12345L);
		sequoiadbTemplate.insert(p12);
		sequoiadbTemplate.save(p12);
		assertThat(p12.getId(), notNullValue());
		PersonWithIdPropertyOfPrimitiveLong p12q = sequoiadbTemplate.findOne(new Query(where("id").in(p12.getId())),
				PersonWithIdPropertyOfPrimitiveLong.class);
		assertThat(p12q, notNullValue());
		assertThat(p12q.getId(), is(p12.getId()));
		checkCollectionContents(PersonWithIdPropertyOfPrimitiveLong.class, 1);
	}

	private void checkCollectionContents(Class<?> entityClass, int count) {
		assertThat(template.findAll(entityClass).size(), is(count));
	}

	/**
	 * @see DATA_JIRA-234
	 */
	@Test
	public void testFindAndUpdate() {
		prepareCollection(Person.class);
		template.insert(new Person("Tom", 21));
		template.insert(new Person("Dick", 22));
		template.insert(new Person("Harry", 23));

		Query query = new Query(Criteria.where("firstName").is("Harry"));
		Update update = new Update().inc("age", 1);
		Person p = template.findAndModify(query, update, Person.class); // return old
		assertThat(p.getFirstName(), is("Harry"));
		assertThat(p.getAge(), is(23));
		p = template.findOne(query, Person.class);
		assertThat(p.getAge(), is(24));

		p = template.findAndModify(query, update, Person.class, "person");
		assertThat(p.getAge(), is(24));
		p = template.findOne(query, Person.class);
		assertThat(p.getAge(), is(25));

		p = template.findAndModify(query, update, new FindAndModifyOptions().returnNew(true), Person.class);
		assertThat(p.getAge(), is(26));

		p = template.findAndModify(query, update, null, Person.class, "person");
		assertThat(p.getAge(), is(26));
		p = template.findOne(query, Person.class);
		assertThat(p.getAge(), is(27));

		try {
			Query query2 = new Query(Criteria.where("firstName").is("Mary"));
			p = template.findAndModify(query2, update, new FindAndModifyOptions().returnNew(true).upsert(true), Person.class);
			assertThat(p.getFirstName(), is("Mary"));
			assertThat(p.getAge(), is(1));
		} catch (UnsupportedOperationException unsupportException) {

		} catch(Exception e) {
			assertFalse("should be unsupport exception",true);
		}

	}

	@Test
	public void testFindAndUpdateUpsert() {
		prepareCollection(Person.class);
		template.insert(new Person("Tom", 21));
		template.insert(new Person("Dick", 22));
		Query query = new Query(Criteria.where("firstName").is("Tom"));
		Update update = new Update().set("age", 23);
		Person p = template.findAndModify(query, update, new FindAndModifyOptions().upsert(false).returnNew(true),
				Person.class);
		assertThat(p.getFirstName(), is("Tom"));
		assertThat(p.getAge(), is(23));
	}

	@Test
	public void testFindAndRemove() throws Exception {
		prepareCollection(Message.class);
		Message m1 = new Message("Hello Spring");
		template.insert(m1);
		Message m2 = new Message("Hello SequoiaDB");
		template.insert(m2);

		Query q = new Query(Criteria.where("text").regex("^Hello.*"));
		Message found1 = template.findAndRemove(q, Message.class);
		Message found2 = template.findAndRemove(q, Message.class);
		BSONObject notFound = template.getCollection(Message.class).findAndRemove(q.getQueryObject());
		assertThat(found1, notNullValue());
		assertThat(found2, notNullValue());
		assertThat(notFound, nullValue());
	}

	@Test
	public void testUsingAnInQueryWithObjectId() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeObjectId.class);

		PersonWithIdPropertyOfTypeObjectId p1 = new PersonWithIdPropertyOfTypeObjectId();
		p1.setFirstName("Sven");
		p1.setAge(11);
		template.insert(p1);
		PersonWithIdPropertyOfTypeObjectId p2 = new PersonWithIdPropertyOfTypeObjectId();
		p2.setFirstName("Mary");
		p2.setAge(21);
		template.insert(p2);
		PersonWithIdPropertyOfTypeObjectId p3 = new PersonWithIdPropertyOfTypeObjectId();
		p3.setFirstName("Ann");
		p3.setAge(31);
		template.insert(p3);
		PersonWithIdPropertyOfTypeObjectId p4 = new PersonWithIdPropertyOfTypeObjectId();
		p4.setFirstName("John");
		p4.setAge(41);
		template.insert(p4);

		Query q1 = new Query(Criteria.where("age").in(11, 21, 41));
		List<PersonWithIdPropertyOfTypeObjectId> results1 = template.find(q1, PersonWithIdPropertyOfTypeObjectId.class);
		Query q2 = new Query(Criteria.where("firstName").in("Ann", "Mary"));
		List<PersonWithIdPropertyOfTypeObjectId> results2 = template.find(q2, PersonWithIdPropertyOfTypeObjectId.class);
		Query q3 = new Query(Criteria.where("id").in(p3.getId()));
		List<PersonWithIdPropertyOfTypeObjectId> results3 = template.find(q3, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(results1.size(), is(3));
		assertThat(results2.size(), is(2));
		assertThat(results3.size(), is(1));
	}

	@Test
	public void testUsingAnInQueryWithStringId() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeString.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeString.class);

		PersonWithIdPropertyOfTypeString p1 = new PersonWithIdPropertyOfTypeString();
		p1.setFirstName("Sven");
		p1.setAge(11);
		template.insert(p1);
		PersonWithIdPropertyOfTypeString p2 = new PersonWithIdPropertyOfTypeString();
		p2.setFirstName("Mary");
		p2.setAge(21);
		template.insert(p2);
		PersonWithIdPropertyOfTypeString p3 = new PersonWithIdPropertyOfTypeString();
		p3.setFirstName("Ann");
		p3.setAge(31);
		template.insert(p3);
		PersonWithIdPropertyOfTypeString p4 = new PersonWithIdPropertyOfTypeString();
		p4.setFirstName("John");
		p4.setAge(41);
		template.insert(p4);

		Query q1 = new Query(Criteria.where("age").in(11, 21, 41));
		List<PersonWithIdPropertyOfTypeString> results1 = template.find(q1, PersonWithIdPropertyOfTypeString.class);
		Query q2 = new Query(Criteria.where("firstName").in("Ann", "Mary"));
		List<PersonWithIdPropertyOfTypeString> results2 = template.find(q2, PersonWithIdPropertyOfTypeString.class);
		Query q3 = new Query(Criteria.where("id").in(p3.getId(), p4.getId()));
		List<PersonWithIdPropertyOfTypeString> results3 = template.find(q3, PersonWithIdPropertyOfTypeString.class);
		assertThat(results1.size(), is(3));
		assertThat(results2.size(), is(2));
		assertThat(results3.size(), is(2));
	}

	@Test
	public void testUsingAnInQueryWithLongId() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeLong.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeLong.class);

		PersonWithIdPropertyOfTypeLong p1 = new PersonWithIdPropertyOfTypeLong();
		p1.setFirstName("Sven");
		p1.setAge(11);
		p1.setId(1001L);
		template.insert(p1);
		PersonWithIdPropertyOfTypeLong p2 = new PersonWithIdPropertyOfTypeLong();
		p2.setFirstName("Mary");
		p2.setAge(21);
		p2.setId(1002L);
		template.insert(p2);
		PersonWithIdPropertyOfTypeLong p3 = new PersonWithIdPropertyOfTypeLong();
		p3.setFirstName("Ann");
		p3.setAge(31);
		p3.setId(1003L);
		template.insert(p3);
		PersonWithIdPropertyOfTypeLong p4 = new PersonWithIdPropertyOfTypeLong();
		p4.setFirstName("John");
		p4.setAge(41);
		p4.setId(1004L);
		template.insert(p4);

		Query q1 = new Query(Criteria.where("age").in(11, 21, 41));
		List<PersonWithIdPropertyOfTypeLong> results1 = template.find(q1, PersonWithIdPropertyOfTypeLong.class);
		Query q2 = new Query(Criteria.where("firstName").in("Ann", "Mary"));
		List<PersonWithIdPropertyOfTypeLong> results2 = template.find(q2, PersonWithIdPropertyOfTypeLong.class);
		Query q3 = new Query(Criteria.where("id").in(1001L, 1004L));
		List<PersonWithIdPropertyOfTypeLong> results3 = template.find(q3, PersonWithIdPropertyOfTypeLong.class);
		assertThat(results1.size(), is(3));
		assertThat(results2.size(), is(2));
		assertThat(results3.size(), is(2));
	}

	/**
	 * @see DATA_JIRA-602
	 */
	@Test
	public void testUsingAnInQueryWithBigIntegerId() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeBigInteger.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeBigInteger.class);

		PersonWithIdPropertyOfTypeBigInteger p1 = new PersonWithIdPropertyOfTypeBigInteger();
		p1.setFirstName("Sven");
		p1.setAge(11);
		p1.setId(new BigInteger("2666666666666666665069473312490162649510603601"));
		template.insert(p1);
		PersonWithIdPropertyOfTypeBigInteger p2 = new PersonWithIdPropertyOfTypeBigInteger();
		p2.setFirstName("Mary");
		p2.setAge(21);
		p2.setId(new BigInteger("2666666666666666665069473312490162649510603602"));
		template.insert(p2);
		PersonWithIdPropertyOfTypeBigInteger p3 = new PersonWithIdPropertyOfTypeBigInteger();
		p3.setFirstName("Ann");
		p3.setAge(31);
		p3.setId(new BigInteger("2666666666666666665069473312490162649510603603"));
		template.insert(p3);
		PersonWithIdPropertyOfTypeBigInteger p4 = new PersonWithIdPropertyOfTypeBigInteger();
		p4.setFirstName("John");
		p4.setAge(41);
		p4.setId(new BigInteger("2666666666666666665069473312490162649510603604"));
		template.insert(p4);

		Query q1 = new Query(Criteria.where("age").in(11, 21, 41));
		List<PersonWithIdPropertyOfTypeBigInteger> results1 = template.find(q1, PersonWithIdPropertyOfTypeBigInteger.class);
		Query q2 = new Query(Criteria.where("firstName").in("Ann", "Mary"));
		List<PersonWithIdPropertyOfTypeBigInteger> results2 = template.find(q2, PersonWithIdPropertyOfTypeBigInteger.class);
		Query q3 = new Query(Criteria.where("id").in(new BigInteger("2666666666666666665069473312490162649510603601"),
				new BigInteger("2666666666666666665069473312490162649510603604")));
		List<PersonWithIdPropertyOfTypeBigInteger> results3 = template.find(q3, PersonWithIdPropertyOfTypeBigInteger.class);
		assertThat(results1.size(), is(3));
		assertThat(results2.size(), is(2));
		assertThat(results3.size(), is(2));
	}

	@Test
	public void testUsingAnInQueryWithPrimitiveIntId() throws Exception {
		prepareCollection(PersonWithIdPropertyOfPrimitiveInt.class);
		template.remove(new Query(), PersonWithIdPropertyOfPrimitiveInt.class);

		PersonWithIdPropertyOfPrimitiveInt p1 = new PersonWithIdPropertyOfPrimitiveInt();
		p1.setFirstName("Sven");
		p1.setAge(11);
		p1.setId(1001);
		template.insert(p1);
		PersonWithIdPropertyOfPrimitiveInt p2 = new PersonWithIdPropertyOfPrimitiveInt();
		p2.setFirstName("Mary");
		p2.setAge(21);
		p2.setId(1002);
		template.insert(p2);
		PersonWithIdPropertyOfPrimitiveInt p3 = new PersonWithIdPropertyOfPrimitiveInt();
		p3.setFirstName("Ann");
		p3.setAge(31);
		p3.setId(1003);
		template.insert(p3);
		PersonWithIdPropertyOfPrimitiveInt p4 = new PersonWithIdPropertyOfPrimitiveInt();
		p4.setFirstName("John");
		p4.setAge(41);
		p4.setId(1004);
		template.insert(p4);

		Query q1 = new Query(Criteria.where("age").in(11, 21, 41));
		List<PersonWithIdPropertyOfPrimitiveInt> results1 = template.find(q1, PersonWithIdPropertyOfPrimitiveInt.class);
		Query q2 = new Query(Criteria.where("firstName").in("Ann", "Mary"));
		List<PersonWithIdPropertyOfPrimitiveInt> results2 = template.find(q2, PersonWithIdPropertyOfPrimitiveInt.class);
		Query q3 = new Query(Criteria.where("id").in(1001, 1003));
		List<PersonWithIdPropertyOfPrimitiveInt> results3 = template.find(q3, PersonWithIdPropertyOfPrimitiveInt.class);
		assertThat(results1.size(), is(3));
		assertThat(results2.size(), is(2));
		assertThat(results3.size(), is(2));
	}

	@Test
	public void testUsingInQueryWithList() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeObjectId.class);

		PersonWithIdPropertyOfTypeObjectId p1 = new PersonWithIdPropertyOfTypeObjectId();
		p1.setFirstName("Sven");
		p1.setAge(11);
		template.insert(p1);
		PersonWithIdPropertyOfTypeObjectId p2 = new PersonWithIdPropertyOfTypeObjectId();
		p2.setFirstName("Mary");
		p2.setAge(21);
		template.insert(p2);
		PersonWithIdPropertyOfTypeObjectId p3 = new PersonWithIdPropertyOfTypeObjectId();
		p3.setFirstName("Ann");
		p3.setAge(31);
		template.insert(p3);
		PersonWithIdPropertyOfTypeObjectId p4 = new PersonWithIdPropertyOfTypeObjectId();
		p4.setFirstName("John");
		p4.setAge(41);
		template.insert(p4);

		List<Integer> l1 = new ArrayList<Integer>();
		l1.add(11);
		l1.add(21);
		l1.add(41);
		Query q1 = new Query(Criteria.where("age").in(l1));
		List<PersonWithIdPropertyOfTypeObjectId> results1 = template.find(q1, PersonWithIdPropertyOfTypeObjectId.class);
		Query q2 = new Query(Criteria.where("age").in(l1.toArray()));
		List<PersonWithIdPropertyOfTypeObjectId> results2 = template.find(q2, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(results1.size(), is(3));
		assertThat(results2.size(), is(3));
		try {
			List<Integer> l2 = new ArrayList<Integer>();
			l2.add(31);
			Query q3 = new Query(Criteria.where("age").in(l1, l2));
			template.find(q3, PersonWithIdPropertyOfTypeObjectId.class);
			fail("Should have trown an InvalidDocumentStoreApiUsageException");
		} catch (InvalidSequoiadbApiUsageException e) {}
	}

	@Test
	public void testUsingRegexQueryWithOptions() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeObjectId.class);

		PersonWithIdPropertyOfTypeObjectId p1 = new PersonWithIdPropertyOfTypeObjectId();
		p1.setFirstName("Sven");
		p1.setAge(11);
		template.insert(p1);
		PersonWithIdPropertyOfTypeObjectId p2 = new PersonWithIdPropertyOfTypeObjectId();
		p2.setFirstName("Mary");
		p2.setAge(21);
		template.insert(p2);
		PersonWithIdPropertyOfTypeObjectId p3 = new PersonWithIdPropertyOfTypeObjectId();
		p3.setFirstName("Ann");
		p3.setAge(31);
		template.insert(p3);
		PersonWithIdPropertyOfTypeObjectId p4 = new PersonWithIdPropertyOfTypeObjectId();
		p4.setFirstName("samantha");
		p4.setAge(41);
		template.insert(p4);

		Query q1 = new Query(Criteria.where("firstName").regex("S.*"));
		List<PersonWithIdPropertyOfTypeObjectId> results1 = template.find(q1, PersonWithIdPropertyOfTypeObjectId.class);
		Query q2 = new Query(Criteria.where("firstName").regex("S.*", "i"));
		List<PersonWithIdPropertyOfTypeObjectId> results2 = template.find(q2, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(results1.size(), is(1));
		assertThat(results2.size(), is(2));
	}

	@Test
	public void testUsingAnOrQuery() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeObjectId.class);

		PersonWithIdPropertyOfTypeObjectId p1 = new PersonWithIdPropertyOfTypeObjectId();
		p1.setFirstName("Sven");
		p1.setAge(11);
		template.insert(p1);
		PersonWithIdPropertyOfTypeObjectId p2 = new PersonWithIdPropertyOfTypeObjectId();
		p2.setFirstName("Mary");
		p2.setAge(21);
		template.insert(p2);
		PersonWithIdPropertyOfTypeObjectId p3 = new PersonWithIdPropertyOfTypeObjectId();
		p3.setFirstName("Ann");
		p3.setAge(31);
		template.insert(p3);
		PersonWithIdPropertyOfTypeObjectId p4 = new PersonWithIdPropertyOfTypeObjectId();
		p4.setFirstName("John");
		p4.setAge(41);
		template.insert(p4);

		Query orQuery = new Query(new Criteria().orOperator(where("age").in(11, 21), where("age").is(31)));
		List<PersonWithIdPropertyOfTypeObjectId> results = template.find(orQuery, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(results.size(), is(3));
		for (PersonWithIdPropertyOfTypeObjectId p : results) {
			assertThat(p.getAge(), isOneOf(11, 21, 31));
		}
	}

	@Test
	public void testUsingUpdateWithMultipleSet() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		template.remove(new Query(), PersonWithIdPropertyOfTypeObjectId.class);

		PersonWithIdPropertyOfTypeObjectId p1 = new PersonWithIdPropertyOfTypeObjectId();
		p1.setFirstName("Sven");
		p1.setAge(11);
		template.insert(p1);
		PersonWithIdPropertyOfTypeObjectId p2 = new PersonWithIdPropertyOfTypeObjectId();
		p2.setFirstName("Mary");
		p2.setAge(21);
		template.insert(p2);

		Update u = new Update().set("firstName", "Bob").set("age", 10);

		WriteResult wr = template.updateMulti(new Query(), u, PersonWithIdPropertyOfTypeObjectId.class);


		Query q1 = new Query(Criteria.where("age").in(11, 21));
		List<PersonWithIdPropertyOfTypeObjectId> r1 = template.find(q1, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(r1.size(), is(0));
		Query q2 = new Query(Criteria.where("age").is(10));
		List<PersonWithIdPropertyOfTypeObjectId> r2 = template.find(q2, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(r2.size(), is(2));
		for (PersonWithIdPropertyOfTypeObjectId p : r2) {
			assertThat(p.getAge(), is(10));
			assertThat(p.getFirstName(), is("Bob"));
		}
	}

	@Test
	public void testRemovingDocument() throws Exception {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		PersonWithIdPropertyOfTypeObjectId p1 = new PersonWithIdPropertyOfTypeObjectId();
		p1.setFirstName("Sven_to_be_removed");
		p1.setAge(51);
		template.insert(p1);

		Query q1 = new Query(Criteria.where("id").is(p1.getId()));
		PersonWithIdPropertyOfTypeObjectId found1 = template.findOne(q1, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(found1, notNullValue());
		Query _q = new Query(Criteria.where("_id").is(p1.getId()));
		template.remove(_q, PersonWithIdPropertyOfTypeObjectId.class);
		PersonWithIdPropertyOfTypeObjectId notFound1 = template.findOne(q1, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(notFound1, nullValue());

		PersonWithIdPropertyOfTypeObjectId p2 = new PersonWithIdPropertyOfTypeObjectId();
		p2.setFirstName("Bubba_to_be_removed");
		p2.setAge(51);
		template.insert(p2);

		Query q2 = new Query(Criteria.where("id").is(p2.getId()));
		PersonWithIdPropertyOfTypeObjectId found2 = template.findOne(q2, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(found2, notNullValue());
		template.remove(q2, PersonWithIdPropertyOfTypeObjectId.class);
		PersonWithIdPropertyOfTypeObjectId notFound2 = template.findOne(q2, PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(notFound2, nullValue());
	}

	@Test
	public void testAddingToList() {
		prepareCollection(PersonWithAList.class);
		PersonWithAList p = new PersonWithAList();
		p.setFirstName("Sven");
		p.setAge(22);
		template.insert(p);

		Query q1 = new Query(Criteria.where("id").is(p.getId()));
		PersonWithAList p2 = template.findOne(q1, PersonWithAList.class);
		assertThat(p2, notNullValue());
		assertThat(p2.getWishList().size(), is(0));

		p2.addToWishList("please work!");

		template.save(p2);

		PersonWithAList p3 = template.findOne(q1, PersonWithAList.class);
		assertThat(p3, notNullValue());
		assertThat(p3.getWishList().size(), is(1));

		Friend f = new Friend();
		p.setFirstName("Erik");
		p.setAge(21);

		p3.addFriend(f);
		template.save(p3);

		PersonWithAList p4 = template.findOne(q1, PersonWithAList.class);
		assertThat(p4, notNullValue());
		assertThat(p4.getWishList().size(), is(1));
		assertThat(p4.getFriends().size(), is(1));

	}

	@Test
	public void testFindOneWithSort() {
		prepareCollection(PersonWithAList.class);
		PersonWithAList p = new PersonWithAList();
		p.setFirstName("Sven");
		p.setAge(22);
		template.insert(p);

		PersonWithAList p2 = new PersonWithAList();
		p2.setFirstName("Erik");
		p2.setAge(21);
		template.insert(p2);

		PersonWithAList p3 = new PersonWithAList();
		p3.setFirstName("Mark");
		p3.setAge(40);
		template.insert(p3);

		Query q2 = new Query(Criteria.where("age").gt(10));
		q2.with(new Sort(Direction.DESC, "age"));
		PersonWithAList p5 = template.findOne(q2, PersonWithAList.class);
		assertThat(p5.getFirstName(), is("Mark"));
	}

	@Test
	@Ignore /// we are not support getOptions
	@SuppressWarnings("deprecation")
	public void testUsingReadPreference() throws Exception {
		this.template.execute("readPref", new CollectionCallback<Object>() {
			public Object doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				assertThat(collection.getOptions(), is(0));
				assertThat(collection.getDB().getOptions(), is(0));
				return null;
			}
		});
		SequoiadbTemplate slaveTemplate = new SequoiadbTemplate(factory);
		slaveTemplate.execute("readPref", new CollectionCallback<Object>() {
			public Object doInCollection(DBCollection collection) throws BaseException, DataAccessException {
				assertThat(collection.getDB().getOptions(), is(0));
				return null;
			}
		});
	}

	/**
	 * @see DATADOC-166
	 */
	@Test
	public void removingNullIsANoOp() {
		template.remove(null);
	}

	/**
	 * @see DATADOC-240, DATADOC-212
	 */
	@Test
	public void updatesObjectIdsCorrectly() {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		PersonWithIdPropertyOfTypeObjectId person = new PersonWithIdPropertyOfTypeObjectId();
		person.setId(new ObjectId());
		person.setFirstName("Dave");

		template.save(person);
		template.updateFirst(query(where("id").is(person.getId())), update("firstName", "Carter"),
				PersonWithIdPropertyOfTypeObjectId.class);

		PersonWithIdPropertyOfTypeObjectId result = template.findById(person.getId(),
				PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(result, is(notNullValue()));
		assertThat(result.getId(), is(person.getId()));
		assertThat(result.getFirstName(), is("Carter"));
	}

	@Test
	@Ignore // sdb not support write concern
	public void testWriteConcernResolver() {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		PersonWithIdPropertyOfTypeObjectId person = new PersonWithIdPropertyOfTypeObjectId();
		person.setId(new ObjectId());
		person.setFirstName("Dave");

		template.setWriteConcern(WriteConcern.NONE);
		template.save(person);
		WriteResult result = template.updateFirst(query(where("id").is(person.getId())), update("firstName", "Carter"),
				PersonWithIdPropertyOfTypeObjectId.class);
		WriteConcern lastWriteConcern = result.getLastConcern();
		assertThat(lastWriteConcern, equalTo(WriteConcern.NONE));

		FsyncSafeWriteConcernResolver resolver = new FsyncSafeWriteConcernResolver();
		template.setWriteConcernResolver(resolver);
		Query q = query(where("_id").is(person.getId()));
		Update u = update("firstName", "Carter");
		result = template.updateFirst(q, u, PersonWithIdPropertyOfTypeObjectId.class);
		lastWriteConcern = result.getLastConcern();
		assertThat(lastWriteConcern, equalTo(WriteConcern.FSYNC_SAFE));

		SequoiadbAction lastSequoiadbAction = resolver.getSequoiadbAction();
		assertThat(lastSequoiadbAction.getCollectionName(), is("personWithIdPropertyOfTypeObjectId"));
		assertThat(lastSequoiadbAction.getDefaultWriteConcern(), equalTo(WriteConcern.NONE));
		assertThat(lastSequoiadbAction.getDocument(), notNullValue());
		assertThat(lastSequoiadbAction.getEntityType().toString(), is(PersonWithIdPropertyOfTypeObjectId.class.toString()));
		assertThat(lastSequoiadbAction.getSequoiadbActionOperation(), is(SequoiadbActionOperation.UPDATE));
		assertThat(lastSequoiadbAction.getQuery(), equalTo(q.getQueryObject()));

	}

	private class FsyncSafeWriteConcernResolver implements WriteConcernResolver {

		private SequoiadbAction sequoiadbAction;

		public WriteConcern resolve(SequoiadbAction action) {
			this.sequoiadbAction = action;
			return WriteConcern.FSYNC_SAFE;
		}

		public SequoiadbAction getSequoiadbAction() {
			return sequoiadbAction;
		}
	}

	/**
	 * @see DATADOC-246
	 */
	@Test
	@Ignore // sdb not support DBRef
	public void updatesDBRefsCorrectly() {

		DBRef first = new DBRef(factory.getDb(), "foo", new ObjectId());
		DBRef second = new DBRef(factory.getDb(), "bar", new ObjectId());

		template.updateFirst(null, update("dbRefs", Arrays.asList(first, second)), ClassWithDBRefs.class);
	}

	class ClassWithDBRefs {
		List<DBRef> dbrefs;
	}

	/**
	 * @see DATADOC-202
	 */
	@Test
	public void executeDocument() {
		prepareCollection(Person.class);
		template.insert(new Person("Tom"));
		template.insert(new Person("Dick"));
		template.insert(new Person("Harry"));
		final List<String> names = new ArrayList<String>();
		template.executeQuery(new Query(), template.getCollectionName(Person.class), new DocumentCallbackHandler() {
			public void processDocument(BSONObject dbObject) {
				String name = (String) dbObject.get("firstName");
				if (name != null) {
					names.add(name);
				}
			}
		});
		assertEquals(3, names.size());
	}

	/**
	 * @see DATADOC-202
	 */
	@Test
	public void executeDocumentWithCursorPreparer() {
		prepareCollection(Person.class);
		template.insert(new Person("Tom"));
		template.insert(new Person("Dick"));
		template.insert(new Person("Harry"));
		final List<String> names = new ArrayList<String>();
		template.executeQuery(new Query().limit(1), template.getCollectionName(Person.class), new DocumentCallbackHandler() {
			public void processDocument(BSONObject dbObject) {
				String name = (String) dbObject.get("firstName");
				if (name != null) {
					names.add(name);
				}
			}
		}, null/*new CursorPreparer() {

			public DBCursor prepare(DBCursor cursor) {
				cursor.limit(1);
				return cursor;
			}

			@Override
			public Query getQuery() {
				return null;
			}

		}*/);
		assertEquals(1, names.size());
	}

	/**
	 * @see DATADOC-183
	 */
	@Test
	public void countsDocumentsCorrectly() {
		prepareCollection(Person.class);
		assertThat(template.count(new Query(), Person.class), is(0L));

		Person dave = new Person("Dave");
		Person carter = new Person("Carter");

		template.save(dave);
		template.save(carter);

		assertThat(template.count(null, Person.class), is(2L));
		assertThat(template.count(query(where("firstName").is("Carter")), Person.class), is(1L));
	}

	/**
	 * @see DATADOC-183
	 */
	@Test(expected = IllegalArgumentException.class)
	public void countRejectsNullEntityClass() {
		template.count(null, (Class<?>) null);
	}

	/**
	 * @see DATADOC-183
	 */
	@Test(expected = IllegalArgumentException.class)
	public void countRejectsEmptyCollectionName() {
		template.count(null, "");
	}

	/**
	 * @see DATADOC-183
	 */
	@Test(expected = IllegalArgumentException.class)
	public void countRejectsNullCollectionName() {
		template.count(null, (String) null);
	}

	@Test
	public void returnsEntityWhenQueryingForDateTime() {
		prepareCollection(TestClass.class);
		DateTime dateTime = new DateTime(2011, 3, 3, 12, 0, 0, 0);
		TestClass testClass = new TestClass(dateTime);
		mappingTemplate.save(testClass);

		List<TestClass> testClassList = mappingTemplate.find(new Query(Criteria.where("myDate").is(dateTime.toDate())),
				TestClass.class);
		assertThat(testClassList.size(), is(1));
		assertThat(testClassList.get(0).myDate, is(testClass.myDate));
	}

	/**
	 * @see DATADOC-230
	 */
	@Test
	public void removesEntityFromCollection() {
		prepareCollection("mycollection");
		template.remove(new Query(), "mycollection");

		Person person = new Person("Dave");

		template.save(person, "mycollection");
		assertThat(template.findAll(TestClass.class, "mycollection").size(), is(1));

		template.remove(person, "mycollection");
		assertThat(template.findAll(Person.class, "mycollection").isEmpty(), is(true));
	}

	/**
	 * @see DATADOC-349
	 */
	@Test
	public void removesEntityWithAnnotatedIdIfIdNeedsMassaging() {
		prepareCollection(Sample.class);
		String id = new ObjectId().toString();

		Sample sample = new Sample();
		sample.id = id;

		template.save(sample);

		assertThat(template.findOne(query(where("id").is(id)), Sample.class).id, is(id));

		template.remove(sample);
		assertThat(template.findOne(query(where("id").is(id)), Sample.class), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-423
	 */
	@Test
	public void executesQueryWithNegatedRegexCorrectly() {
		prepareCollection(Sample.class);
		Sample first = new Sample();
		first.field = "Matthews";

		Sample second = new Sample();
		second.field = "Beauford";

		template.save(first);
		template.save(second);


		Query query = query(where("field").regex("Matthews"));
		List<Sample> result = template.find(query, Sample.class);
		assertThat(result.size(), is(1));
		assertThat(result.get(0).field, is("Matthews"));
	}

	/**
	 * @see DATA_JIRA-447
	 */
	@Test
	public void storesAndRemovesTypeWithComplexId() {
		prepareCollection(TypeWithMyId.class);
		MyId id = new MyId();
		id.first = "foo";
		id.second = "bar";

		TypeWithMyId source = new TypeWithMyId();
		source.id = id;

		template.save(source);
		template.remove(query(where("id").is(id)), TypeWithMyId.class);
	}

	/**
	 * @see DATA_JIRA-506
	 */
	@Test
	public void exceutesBasicQueryCorrectly() {
		prepareCollection(MyPerson.class);
		Address address = new Address();
		address.state = "PA";
		address.city = "Philadelphia";

		MyPerson person = new MyPerson();
		person.name = "Oleg";
		person.address = address;

		template.save(person);

		Query query = new BasicQuery("{'name' : 'Oleg'}");
		List<MyPerson> result = template.find(query, MyPerson.class);

		assertThat(result, hasSize(1));
		assertThat(result.get(0), hasProperty("name", is("Oleg")));

		query = new BasicQuery("{'address.state' : 'PA' }");
		result = template.find(query, MyPerson.class);

		assertThat(result, hasSize(1));
		assertThat(result.get(0), hasProperty("name", is("Oleg")));
	}

	/**
	 * @see DATA_JIRA-279
	 */
	@Test(expected = OptimisticLockingFailureException.class)
	@Ignore // when update has no matched records, sdb whould not report the error, so we can't
	public void optimisticLockingHandling() {
		prepareCollection(PersonWithVersionPropertyOfTypeInteger.class);
		PersonWithVersionPropertyOfTypeInteger person = new PersonWithVersionPropertyOfTypeInteger();
		person.age = 29;
		person.firstName = "Patryk";
		template.save(person);

		List<PersonWithVersionPropertyOfTypeInteger> result = template
				.findAll(PersonWithVersionPropertyOfTypeInteger.class);

		assertThat(result, hasSize(1));
		assertThat(result.get(0).version, is(0));

		person = result.get(0);
		person.firstName = "Patryk2";

		template.save(person);

		assertThat(person.version, is(1));

		result = mappingTemplate.findAll(PersonWithVersionPropertyOfTypeInteger.class);

		assertThat(result, hasSize(1));
		assertThat(result.get(0).version, is(1));

		person.version = 0;
		person.firstName = "Patryk3";

		template.save(person);
	}

	/**
	 * @see DATA_JIRA-562
	 */
	@Test
	public void optimisticLockingHandlingWithExistingId() {
		prepareCollection(PersonWithVersionPropertyOfTypeInteger.class);
		PersonWithVersionPropertyOfTypeInteger person = new PersonWithVersionPropertyOfTypeInteger();
		person.id = new ObjectId().toString();
		person.age = 29;
		person.firstName = "Patryk";
		template.save(person);
	}

	/**
	 * @see DATA_JIRA-617
	 */
	@Test
	public void doesNotFailOnVersionInitForUnversionedEntity() {
		prepareCollection(PersonWithVersionPropertyOfTypeInteger.class);
		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("firstName", "Oliver");

		template.insert(dbObject, template.determineCollectionName(PersonWithVersionPropertyOfTypeInteger.class));
	}

	/**
	 * @see DATA_JIRA-539
	 */
	@Test
	public void removesObjectFromExplicitCollection() {

		String collectionName = "explicit";
		prepareCollection(collectionName);
		template.remove(new Query(), collectionName);

		PersonWithConvertedId person = new PersonWithConvertedId();
		person.name = "Dave";
		template.save(person, collectionName);
		assertThat(template.findAll(PersonWithConvertedId.class, collectionName).isEmpty(), is(false));

		template.remove(person, collectionName);
		assertThat(template.findAll(PersonWithConvertedId.class, collectionName).isEmpty(), is(true));
	}

	/**
	 * @see DATA_JIRA-549
	 */
	@Test
	public void savesMapCorrectly() {
		prepareCollection("maps");
		Map<String, String> map = new HashMap<String, String>();
		map.put("key", "value");

		template.save(map, "maps");
	}

	/**
	 * @see DATA_JIRA-549
	 */
	@Test(expected = MappingException.class)
	public void savesSequoiadbPrimitiveObjectCorrectly() {
		prepareCollection("collection");
		template.save(new Object(), "collection");
	}

	/**
	 * @see DATA_JIRA-549
	 */
	@Test(expected = IllegalArgumentException.class)
	public void rejectsNullObjectToBeSaved() {
		template.save(null);
	}

	/**
	 * @see DATA_JIRA-550
	 */
	@Test
	public void savesPlainDbObjectCorrectly() {
		prepareCollection("collection");
		BSONObject dbObject = new BasicBSONObject("foo", "bar");
		template.save(dbObject, "collection");

		assertThat(dbObject.containsField("_id"), is(true));
	}

	/**
	 * @see DATA_JIRA-550
	 */
	@Test(expected = InvalidDataAccessApiUsageException.class)
	public void rejectsPlainObjectWithOutExplicitCollection() {
		prepareCollection("collection");
		BSONObject dbObject = new BasicBSONObject("foo", "bar");
		template.save(dbObject, "collection");

		template.findById(dbObject.get("_id"), BSONObject.class);
	}

	/**
	 * @see DATA_JIRA-550
	 */
	@Test
	public void readsPlainDbObjectById() {
		prepareCollection("collection");
		BSONObject dbObject = new BasicBSONObject("foo", "bar");
		template.save(dbObject, "collection");

		BSONObject result = template.findById(dbObject.get("_id"), BSONObject.class, "collection");
		assertThat(result.get("foo"), is(dbObject.get("foo")));
		assertThat(result.get("_id"), is(dbObject.get("_id")));
	}

	/**
	 * @see DATA_JIRA-551
	 */
	@Test
	public void writesPlainString() {
		prepareCollection("collection");
		template.save("{ 'foo' : 'bar' }", "collection");
	}

	/**
	 * @see DATA_JIRA-551
	 */
	@Test(expected = MappingException.class)
	public void rejectsNonJsonStringForSave() {
		prepareCollection("collection");
		template.save("Foobar!", "collection");
	}

	/**
	 * @see DATA_JIRA-588
	 */
	@Test
	public void initializesVersionOnInsert() {
		prepareCollection(PersonWithVersionPropertyOfTypeInteger.class);
		PersonWithVersionPropertyOfTypeInteger person = new PersonWithVersionPropertyOfTypeInteger();
		person.firstName = "Dave";

		template.insert(person);

		assertThat(person.version, is(0));
	}

	/**
	 * @see DATA_JIRA-588
	 */
	@Test
	public void initializesVersionOnBatchInsert() {
		prepareCollection(PersonWithVersionPropertyOfTypeInteger.class);
		PersonWithVersionPropertyOfTypeInteger person = new PersonWithVersionPropertyOfTypeInteger();
		person.firstName = "Dave";

		template.insertAll(Arrays.asList(person));

		assertThat(person.version, is(0));
	}

	/**
	 * @see DATA_JIRA-568
	 */
	@Test
	public void queryCantBeNull() {
		prepareCollection(PersonWithIdPropertyOfTypeObjectId.class);
		List<PersonWithIdPropertyOfTypeObjectId> result = template.findAll(PersonWithIdPropertyOfTypeObjectId.class);
		assertThat(template.find(null, PersonWithIdPropertyOfTypeObjectId.class), is(result));
	}

	/**
	 * @see DATA_JIRA-620
	 */
	@Test
	public void versionsObjectIntoDedicatedCollection() {
		prepareCollection("personX");
		PersonWithVersionPropertyOfTypeInteger person = new PersonWithVersionPropertyOfTypeInteger();
		person.firstName = "Dave";

		template.save(person, "personX");
		assertThat(person.version, is(0));

		template.save(person, "personX");
		assertThat(person.version, is(1));
	}

	/**
	 * @see DATA_JIRA-621
	 */
	@Test
	public void correctlySetsLongVersionProperty() {
		prepareCollection(PersonWithVersionPropertyOfTypeLong.class);
		PersonWithVersionPropertyOfTypeLong person = new PersonWithVersionPropertyOfTypeLong();
		person.firstName = "Dave";

		template.save(person);
		assertThat(person.version, is(0L));
	}

	/**
	 * @see DATA_JIRA-622
	 */
	@Test(expected = DuplicateKeyException.class)
	public void preventsDuplicateInsert() {
		prepareCollection(PersonWithVersionPropertyOfTypeInteger.class);
		template.setWriteConcern(WriteConcern.SAFE);

		PersonWithVersionPropertyOfTypeInteger person = new PersonWithVersionPropertyOfTypeInteger();
		person.firstName = "Dave";

		template.save(person);
		assertThat(person.version, is(0));

		person.version = null;
		template.save(person);
	}

	/**
	 * @see DATA_JIRA-629
	 */
	@Test
	public void countAndFindWithoutTypeInformation() {
		prepareCollection(Person.class);
		Person person = new Person();
		template.save(person);

		Query query = query(where("_id").is(person.getId()));
		String collectionName = template.getCollectionName(Person.class);

		assertThat(template.find(query, HashMap.class, collectionName), hasSize(1));
		assertThat(template.count(query, collectionName), is(1L));
	}

	/**
	 * @see DATA_JIRA-571
	 */
	@Test
	public void nullsPropertiesForVersionObjectUpdates() {
		prepareCollection(VersionedPerson.class);
		VersionedPerson person = new VersionedPerson();
		person.firstname = "Dave";
		person.lastname = "Matthews";

		template.save(person);
		assertThat(person.id, is(notNullValue()));

		person.lastname = null;
		template.save(person);

		person = template.findOne(query(where("id").is(person.id)), VersionedPerson.class);
		assertThat(person.lastname, is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-571
	 */
	@Test
	public void nullsValuesForUpdatesOfUnversionedEntity() {
		prepareCollection(Person.class);
		Person person = new Person("Dave");
		template.save(person);

		person.setFirstName(null);
		template.save(person);

		person = template.findOne(query(where("id").is(person.getId())), Person.class);
		assertThat(person.getFirstName(), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-651
	 */
	@Test
	public void throwsSequoiadbSpecificExceptionForDataIntegrityViolations() {

		WriteResult result = mock(WriteResult.class);
		when(result.getError()).thenReturn("ERROR");

		SequoiadbActionOperation operation = SequoiadbActionOperation.INSERT;

		SequoiadbTemplate sequoiadbTemplate = new SequoiadbTemplate(factory);
		sequoiadbTemplate.setWriteResultChecking(WriteResultChecking.EXCEPTION);

		try {
			sequoiadbTemplate.handleAnyWriteResultErrors(result, null, operation);
			fail("Expected SequoiadbDataIntegrityViolationException!");
		} catch (SequoiadbDataIntegrityViolationException o_O) {
			assertThat(o_O.getActionOperation(), is(operation));
			assertThat(o_O.getWriteResult(), is(result));
		}
	}

	/**
	 * @see DATA_JIRA-679
	 */
	@Test
	public void savesJsonStringCorrectly() {
		prepareCollection("collection");
		BSONObject dbObject = new BasicBSONObject();
		dbObject.put("first", "first");
		dbObject.put("second", "second");

		template.save(dbObject.toString(), "collection");

		List<BSONObject> result = template.findAll(BSONObject.class, "collection");
		assertThat(result.size(), is(1));
		assertThat(result.get(0).containsField("first"), is(true));
	}

	@Test
	public void executesExistsCorrectly() {
		prepareCollection(Sample.class);
		Sample sample = new Sample();
		template.save(sample);

		Query query = query(where("id").is(sample.id));

		assertThat(template.exists(query, Sample.class), is(true));
		assertThat(template.exists(query(where("_id").is(sample.id)), template.getCollectionName(Sample.class)), is(true));
		assertThat(template.exists(query, Sample.class, template.getCollectionName(Sample.class)), is(true));
	}

	/**
	 * @see DATA_JIRA-675
	 */
	@Test
	public void updateConsidersMappingAnnotations() {
		prepareCollection(TypeWithFieldAnnotation.class);
		TypeWithFieldAnnotation entity = new TypeWithFieldAnnotation();
		entity.emailAddress = "old";

		template.save(entity);

		Query query = query(where("_id").is(entity.id));
		Update update = Update.update("emailAddress", "new");

		FindAndModifyOptions options = new FindAndModifyOptions().returnNew(true);
		TypeWithFieldAnnotation result = template.findAndModify(query, update, options, TypeWithFieldAnnotation.class);
		assertThat(result.emailAddress, is("new"));
	}

	/**
	 * @see DATA_JIRA-671
	 */
	@Test
	public void findsEntityByDateReference() {
		prepareCollection(TypeWithDate.class);
		TypeWithDate entity = new TypeWithDate();
		entity.date = new Date(System.currentTimeMillis() - 10);
		template.save(entity);

		Query query = query(where("date").lt(new Date()));
		List<TypeWithDate> result = template.find(query, TypeWithDate.class);

		assertThat(result, hasSize(1));
		assertThat(result.get(0).date, is(notNullValue()));
	}

	/**
	 * @see DATA_JIRA-540
	 */
	@Test
	public void findOneAfterUpsertForNonExistingObjectReturnsTheInsertedObject() {
		prepareCollection(Sample.class);
		String idValue = "4711";
		Query query = new Query(Criteria.where("id").is(idValue));

		String fieldValue = "bubu";
		Update update = Update.update("field", fieldValue);

		template.upsert(query, update, Sample.class);
		Sample result = template.findOne(query, Sample.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.field, is(fieldValue));
		assertThat(result.id, is(idValue));
	}

	/**
	 * @see DATA_JIRA-392
	 */
	@Test
	public void updatesShouldRetainTypeInformation() {
		prepareCollection(Document.class);
		Document doc = new Document();
		doc.id = "4711";
		doc.model = new ModelA("foo");
		template.insert(doc);

		Query query = new Query(Criteria.where("id").is(doc.id));
		String newModelValue = "bar";
		Update update = Update.update("model", new ModelA(newModelValue));
		template.updateFirst(query, update, Document.class);

		Document result = template.findOne(query, Document.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.id, is(doc.id));
		assertThat(result.model, is(notNullValue()));
		assertThat(result.model.value(), is(newModelValue));
	}

	/**
	 * @see DATA_JIRA-702
	 */
	@Test
	public void queryShouldSupportRealAndAliasedPropertyNamesForFieldInclusions() {
		prepareCollection(ObjectWith3AliasedFields.class);
		ObjectWith3AliasedFields obj = new ObjectWith3AliasedFields();
		obj.id = "4711";
		obj.property1 = "P1";
		obj.property2 = "P2";
		obj.property3 = "P3";

		template.insert(obj);

		Query query = new Query(Criteria.where("id").is(obj.id));
		query.fields() //
				.include("id", 1)
				.include("property2", 1) // real property name
				.include("prop3", 1); // aliased property name

		ObjectWith3AliasedFields result = template.findOne(query, ObjectWith3AliasedFields.class);

		assertThat(result.id, is(obj.id));
		assertThat(result.property1, is(nullValue()));
		assertThat(result.property2, is(obj.property2));
		assertThat(result.property3, is(obj.property3));
	}

	/**
	 * @see DATA_JIRA-702
	 */
	@Test
	public void queryShouldSupportRealAndAliasedPropertyNamesForFieldExclusions() {
		prepareCollection(ObjectWith3AliasedFields.class);
		ObjectWith3AliasedFields obj = new ObjectWith3AliasedFields();
		obj.id = "4711";
		obj.property1 = "P1";
		obj.property2 = "P2";
		obj.property3 = "P3";

		template.insert(obj);

		Query query = new Query(Criteria.where("id").is(obj.id));
		query.fields() //
				.include("property2", 0) // real property name
				.include("prop3", 0); // aliased property name

		ObjectWith3AliasedFields result = template.findOne(query, ObjectWith3AliasedFields.class);

		assertThat(result.id, is(obj.id));
		assertThat(result.property1, is(obj.property1));
		assertThat(result.property2, is(nullValue()));
		assertThat(result.property3, is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-702
	 */
	@Test
	public void findMultipleWithQueryShouldSupportRealAndAliasedPropertyNamesForFieldExclusions() {
		prepareCollection(ObjectWith3AliasedFields.class);
		ObjectWith3AliasedFields obj0 = new ObjectWith3AliasedFields();
		obj0.id = "4711";
		obj0.property1 = "P10";
		obj0.property2 = "P20";
		obj0.property3 = "P30";
		ObjectWith3AliasedFields obj1 = new ObjectWith3AliasedFields();
		obj1.id = "4712";
		obj1.property1 = "P11";
		obj1.property2 = "P21";
		obj1.property3 = "P31";

		template.insert(obj0);
		template.insert(obj1);

		Query query = new Query(Criteria.where("id").in(obj0.id, obj1.id));
		query.fields() //
				.include("property2", 0) // real property name
				.include("prop3", 0); // aliased property name

		List<ObjectWith3AliasedFields> results = template.find(query, ObjectWith3AliasedFields.class);

		assertThat(results, is(notNullValue()));
		assertThat(results.size(), is(2));

		ObjectWith3AliasedFields result0 = results.get(0);
		assertThat(result0, is(notNullValue()));
		assertThat(result0.id, is(obj0.id));
		assertThat(result0.property1, is(obj0.property1));
		assertThat(result0.property2, is(nullValue()));
		assertThat(result0.property3, is(nullValue()));

		ObjectWith3AliasedFields result1 = results.get(1);
		assertThat(result1, is(notNullValue()));
		assertThat(result1.id, is(obj1.id));
		assertThat(result1.property1, is(obj1.property1));
		assertThat(result1.property2, is(nullValue()));
		assertThat(result1.property3, is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-702
	 */
	@Test
	public void queryShouldSupportNestedPropertyNamesForFieldInclusions() {
		prepareCollection(ObjectWith3AliasedFieldsAndNestedAddress.class);
		ObjectWith3AliasedFieldsAndNestedAddress obj = new ObjectWith3AliasedFieldsAndNestedAddress();
		obj.id = "4711";
		obj.property1 = "P1";
		obj.property2 = "P2";
		obj.property3 = "P3";
		Address address = new Address();
		String stateValue = "WA";
		address.state = stateValue;
		address.city = "Washington";
		obj.address = address;

		template.insert(obj);

		Query query = new Query(Criteria.where("id").is(obj.id));
		query.fields() //
				.include("id", 1)
				.include("property2", 1) // real property name
				.include("address.state", 1); // aliased property name

		ObjectWith3AliasedFieldsAndNestedAddress result = template.findOne(query,
				ObjectWith3AliasedFieldsAndNestedAddress.class);

		assertThat(result.id, is(obj.id));
		assertThat(result.property1, is(nullValue()));
		assertThat(result.property2, is(obj.property2));
		assertThat(result.property3, is(nullValue()));
		assertThat(result.address, is(notNullValue()));
		assertThat(result.address.city, is(nullValue()));
		assertThat(result.address.state, is(stateValue));
	}

	/**
	 * @see DATA_JIRA-709
	 */
	@Test
	public void aQueryRestrictedWithOneRestrictedResultTypeShouldReturnOnlyInstancesOfTheRestrictedType() {
		prepareCollection(BaseDoc.class);
		BaseDoc doc0 = new BaseDoc();
		doc0.value = "foo";
		SpecialDoc doc1 = new SpecialDoc();
		doc1.value = "foo";
		doc1.specialValue = "specialfoo";
		VerySpecialDoc doc2 = new VerySpecialDoc();
		doc2.value = "foo";
		doc2.specialValue = "specialfoo";
		doc2.verySpecialValue = 4711;

		String collectionName = template.getCollectionName(BaseDoc.class);
		template.insert(doc0, collectionName);
		template.insert(doc1, collectionName);
		template.insert(doc2, collectionName);

		Query query = Query.query(where("value").is("foo")).restrict(SpecialDoc.class);
		List<BaseDoc> result = template.find(query, BaseDoc.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.size(), is(1));
		assertThat(result.get(0), is(instanceOf(SpecialDoc.class)));
	}

	/**
	 * @see DATA_JIRA-709
	 */
	@Test
	public void aQueryRestrictedWithMultipleRestrictedResultTypesShouldReturnOnlyInstancesOfTheRestrictedTypes() {
		prepareCollection(BaseDoc.class);
		BaseDoc doc0 = new BaseDoc();
		doc0.value = "foo";
		SpecialDoc doc1 = new SpecialDoc();
		doc1.value = "foo";
		doc1.specialValue = "specialfoo";
		VerySpecialDoc doc2 = new VerySpecialDoc();
		doc2.value = "foo";
		doc2.specialValue = "specialfoo";
		doc2.verySpecialValue = 4711;

		String collectionName = template.getCollectionName(BaseDoc.class);
		template.insert(doc0, collectionName);
		template.insert(doc1, collectionName);
		template.insert(doc2, collectionName);

		Query query = Query.query(where("value").is("foo")).restrict(BaseDoc.class, VerySpecialDoc.class);
		List<BaseDoc> result = template.find(query, BaseDoc.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.size(), is(2));
		assertThat(result.get(0).getClass(), is((Object) BaseDoc.class));
		assertThat(result.get(1).getClass(), is((Object) VerySpecialDoc.class));
	}

	/**
	 * @see DATA_JIRA-709
	 */
	@Test
	public void aQueryWithNoRestrictedResultTypesShouldReturnAllInstancesWithinTheGivenCollection() {
		prepareCollection(BaseDoc.class);
		BaseDoc doc0 = new BaseDoc();
		doc0.value = "foo";
		SpecialDoc doc1 = new SpecialDoc();
		doc1.value = "foo";
		doc1.specialValue = "specialfoo";
		VerySpecialDoc doc2 = new VerySpecialDoc();
		doc2.value = "foo";
		doc2.specialValue = "specialfoo";
		doc2.verySpecialValue = 4711;

		String collectionName = template.getCollectionName(BaseDoc.class);
		template.insert(doc0, collectionName);
		template.insert(doc1, collectionName);
		template.insert(doc2, collectionName);

		Query query = Query.query(where("value").is("foo"));
		List<BaseDoc> result = template.find(query, BaseDoc.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.size(), is(3));
		assertThat(result.get(0).getClass(), is((Object) BaseDoc.class));
		assertThat(result.get(1).getClass(), is((Object) SpecialDoc.class));
		assertThat(result.get(2).getClass(), is((Object) VerySpecialDoc.class));
	}

	/**
	 * @see DATA_JIRA-771
	 */
	@Test
	public void allowInsertWithPlainJsonString() {
		prepareCollection("sample");
		String id = "4711";
		String value = "bubu";
		String json = String.format("{_id:%s, field: '%s'}", id, value);

		template.insert(json, "sample");
		List<Sample> result = template.findAll(Sample.class);

		assertThat(result.size(), is(1));
		assertThat(result.get(0).id, is(id));
		assertThat(result.get(0).field, is(value));
	}

	/**
	 * @see DATA_JIRA-816
	 */
	@Test
	public void shouldExecuteQueryShouldMapQueryBeforeQueryExecution() {
		prepareCollection(ObjectWithEnumValue.class);
		ObjectWithEnumValue o = new ObjectWithEnumValue();
		o.value = EnumValue.VALUE2;
		template.save(o);

		Query q = Query.query(Criteria.where("value").in(EnumValue.VALUE2));

		template.executeQuery(q, StringUtils.uncapitalize(ObjectWithEnumValue.class.getSimpleName()),
				new DocumentCallbackHandler() {

					@Override
					public void processDocument(BSONObject dbObject) throws BaseException, DataAccessException {

						assertThat(dbObject, is(notNullValue()));

						ObjectWithEnumValue result = template.getConverter().read(ObjectWithEnumValue.class, dbObject);

						assertThat(result.value, is(EnumValue.VALUE2));
					}
				});
	}

	/**
	 * @see DATA_JIRA-811
	 */
	@Test
	public void updateFirstShouldIncreaseVersionForVersionedEntity() {
		prepareCollection(VersionedPerson.class);
		VersionedPerson person = new VersionedPerson();
		person.firstname = "Dave";
		person.lastname = "Matthews";
		template.save(person);
		assertThat(person.id, is(notNullValue()));

		Query qry = query(where("id").is(person.id));
		VersionedPerson personAfterFirstSave = template.findOne(qry, VersionedPerson.class);
		assertThat(personAfterFirstSave.version, is(0L));

		template.updateFirst(qry, Update.update("lastname", "Bubu"), VersionedPerson.class);

		VersionedPerson personAfterUpdateFirst = template.findOne(qry, VersionedPerson.class);
		assertThat(personAfterUpdateFirst.version, is(1L));
		assertThat(personAfterUpdateFirst.lastname, is("Bubu"));
	}

	/**
	 * @see DATA_JIRA-811
	 */
	@Test
	public void updateFirstShouldIncreaseVersionOnlyForFirstMatchingEntity() {
		prepareCollection(VersionedPerson.class);
		VersionedPerson person1 = new VersionedPerson();
		person1.firstname = "Dave";

		VersionedPerson person2 = new VersionedPerson();
		person2.firstname = "Dave";

		template.save(person1);
		template.save(person2);
		Query q = query(where("id").in(person1.id, person2.id));

		template.updateFirst(q, Update.update("lastname", "Metthews"), VersionedPerson.class);

		for (VersionedPerson p : template.find(q, VersionedPerson.class)) {
			if ("Metthews".equals(p.lastname)) {
				assertThat(p.version, equalTo(Long.valueOf(1)));
			} else {
				assertThat(p.version, equalTo(Long.valueOf(0)));
			}
		}
	}

	/**
	 * @see DATA_JIRA-811
	 */
	@Test
	public void updateMultiShouldIncreaseVersionOfAllUpdatedEntities() {
		prepareCollection(VersionedPerson.class);
		VersionedPerson person1 = new VersionedPerson();
		person1.firstname = "Dave";

		VersionedPerson person2 = new VersionedPerson();
		person2.firstname = "Dave";

		template.save(person1);
		template.save(person2);

		Query q = query(where("id").in(person1.id, person2.id));
		template.updateMulti(q, Update.update("lastname", "Metthews"), VersionedPerson.class);

		for (VersionedPerson p : template.find(q, VersionedPerson.class)) {
			assertThat(p.version, equalTo(Long.valueOf(1)));
		}
	}

	/**
	 * @see DATA_JIRA-686
	 */
	@Test
	public void itShouldBePossibleToReuseAnExistingQuery() {
		prepareCollection(Sample.class);
		Sample sample = new Sample();
		sample.id = "42";
		sample.field = "A";

		template.save(sample);

		Query query = new Query();
		query.addCriteria(where("_id").in("42", "43"));

		assertThat(template.count(query, Sample.class), is(1L));

		query.with(new PageRequest(0, 10));
		query.with(new Sort("field"));

		assertThat(template.find(query, Sample.class), is(not(empty())));
	}

	/**
	 * @see DATA_JIRA-807
	 */
	@Test
	public void findAndModifyShouldRetrainTypeInformationWithinUpdatedType() {
		prepareCollection(Document.class);
		Document document = new Document();
		document.model = new ModelA("value1");

		template.save(document);

		Query query = query(where("id").is(document.id));
		Update update = Update.update("model", new ModelA("value2"));
		template.findAndModify(query, update, Document.class);

		Document retrieved = template.findOne(query, Document.class);
		assertThat(retrieved.model, instanceOf(ModelA.class));
		assertThat(retrieved.model.value(), equalTo("value2"));
	}

	/**
	 * @see DATA_JIRA-407
	 */
	@Test
	public void updatesShouldRetainTypeInformationEvenForCollections() {
		prepareCollection(DocumentWithCollection.class);
		List<Model> models = Arrays.<Model> asList(new ModelA("foo"));

		DocumentWithCollection doc = new DocumentWithCollection(models);
		doc.id = "4711";
		template.insert(doc);

		Query query = new Query(Criteria.where("id").is(doc.id));
		query.addCriteria(where("models.value").is("foo"));
		String newModelValue = "bar";
		Update update = Update.update("models.$", new ModelA(newModelValue));
		template.updateFirst(query, update, DocumentWithCollection.class);

		Query findQuery = new Query(Criteria.where("id").is(doc.id));
		DocumentWithCollection result = template.findOne(findQuery, DocumentWithCollection.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.id, is(doc.id));
		assertThat(result.models, is(notNullValue()));
		assertThat(result.models, hasSize(1));
		assertThat(result.models.get(0).value(), is(newModelValue));
	}

	/**
	 * @see DATA_JIRA-812
	 */
	@Test
	@Ignore // not support "$each" in sdb
	public void updateMultiShouldAddValuesCorrectlyWhenUsingPushEachWithComplexTypes() {

	}

	/**
	 * @see DATA_JIRA-812
	 */
	@Test
	@Ignore // not support "$each" in sdb
	public void updateMultiShouldAddValuesCorrectlyWhenUsingPushEachWithSimpleTypes() {

	}

	/**
	 * @see DATAMONOGO-828
	 */
	@Test
	public void updateFirstShouldDoNothingWhenCalledForEntitiesThatDoNotExist() {
		prepareCollection(VersionedPerson.class);
		Query q = query(where("id").is(Long.MIN_VALUE));

		template.updateFirst(q, Update.update("lastname", "supercalifragilisticexpialidocious"), VersionedPerson.class);
		assertThat(template.findOne(q, VersionedPerson.class), nullValue());
	}

	/**
	 * @see DATA_JIRA-354
	 */
	@Test
	public void testUpdateShouldAllowMultiplePushAll() {
		prepareCollection(DocumentWithMultipleCollections.class);
		DocumentWithMultipleCollections doc = new DocumentWithMultipleCollections();
		doc.id = "1234";
		doc.string1 = Arrays.asList("spring");
		doc.string2 = Arrays.asList("one");

		template.save(doc);

		Update update = new Update().pushAll("string1", new Object[] { "data", "sequoiadb" });
		update.pushAll("string2", new String[] { "two", "three" });

		Query findQuery = new Query(Criteria.where("id").is(doc.id));
		template.updateFirst(findQuery, update, DocumentWithMultipleCollections.class);

		DocumentWithMultipleCollections result = template.findOne(findQuery, DocumentWithMultipleCollections.class);
		assertThat(result.string1, hasItems("spring", "data", "sequoiadb"));
		assertThat(result.string2, hasItems("one", "two", "three"));

	}

	/**
	 * @see DATA_JIRA-404
	 */
	@Test
	@Ignore // not support DBRef in sdb
	public void updateWithPullShouldRemoveNestedItemFromDbRefAnnotatedCollection() {
		prepareCollection(Sample.class);
		Sample sample1 = new Sample("1", "A");
		Sample sample2 = new Sample("2", "B");
		template.save(sample1);
		template.save(sample2);

		DocumentWithDBRefCollection doc = new DocumentWithDBRefCollection();
		doc.id = "1";
		doc.dbRefAnnotatedList = Arrays.asList( //
				sample1, //
				sample2 //
				);
		template.save(doc);

		Update update = new Update().pull("dbRefAnnotatedList", doc.dbRefAnnotatedList.get(1));

		Query qry = query(where("id").is("1"));
		template.updateFirst(qry, update, DocumentWithDBRefCollection.class);

		DocumentWithDBRefCollection result = template.findOne(qry, DocumentWithDBRefCollection.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.dbRefAnnotatedList, hasSize(1));
		assertThat(result.dbRefAnnotatedList.get(0), is(notNullValue()));
		assertThat(result.dbRefAnnotatedList.get(0).id, is((Object) "1"));
	}

	/**
	 * @see DATA_JIRA-404
	 */
	@Test
	@Ignore // not support DBRef in sdb
	public void updateWithPullShouldRemoveNestedItemFromDbRefAnnotatedCollectionWhenGivenAnIdValueOfComponentTypeEntity() {
		prepareCollection(Sample.class);
		Sample sample1 = new Sample("1", "A");
		Sample sample2 = new Sample("2", "B");
		template.save(sample1);
		template.save(sample2);

		DocumentWithDBRefCollection doc = new DocumentWithDBRefCollection();
		doc.id = "1";
		doc.dbRefAnnotatedList = Arrays.asList( //
				sample1, //
				sample2 //
				);
		template.save(doc);

		Update update = new Update().pull("dbRefAnnotatedList.id", "2");

		Query qry = query(where("id").is("1"));
		template.updateFirst(qry, update, DocumentWithDBRefCollection.class);

		DocumentWithDBRefCollection result = template.findOne(qry, DocumentWithDBRefCollection.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.dbRefAnnotatedList, hasSize(1));
		assertThat(result.dbRefAnnotatedList.get(0), is(notNullValue()));
		assertThat(result.dbRefAnnotatedList.get(0).id, is((Object) "1"));
	}

	/**
	 * @see DATA_JIRA-852
	 */
	@Test
	public void updateShouldNotBumpVersionNumberIfVersionPropertyIncludedInUpdate() {
		prepareCollection(VersionedPerson.class);
		VersionedPerson person = new VersionedPerson();
		person.firstname = "Dave";
		person.lastname = "Matthews";
		template.save(person);
		assertThat(person.id, is(notNullValue()));

		Query qry = query(where("id").is(person.id));
		VersionedPerson personAfterFirstSave = template.findOne(qry, VersionedPerson.class);
		assertThat(personAfterFirstSave.version, is(0L));

		template.updateFirst(qry, Update.update("lastname", "Bubu").set("version", 100L), VersionedPerson.class);

		VersionedPerson personAfterUpdateFirst = template.findOne(qry, VersionedPerson.class);
		assertThat(personAfterUpdateFirst.version, is(100L));
		assertThat(personAfterUpdateFirst.lastname, is("Bubu"));
	}

	/**
	 * @see DATA_JIRA-468
	 */
	@Test
	@Ignore // not support DBRef in sdb
	public void shouldBeAbleToUpdateDbRefPropertyWithDomainObject() {
		prepareCollection(Sample.class);
		Sample sample1 = new Sample("1", "A");
		Sample sample2 = new Sample("2", "B");
		template.save(sample1);
		template.save(sample2);

		DocumentWithDBRefCollection doc = new DocumentWithDBRefCollection();
		doc.id = "1";
		doc.dbRefProperty = sample1;
		template.save(doc);

		Update update = new Update().set("dbRefProperty", sample2);

		Query qry = query(where("id").is("1"));
		template.updateFirst(qry, update, DocumentWithDBRefCollection.class);

		DocumentWithDBRefCollection updatedDoc = template.findOne(qry, DocumentWithDBRefCollection.class);

		assertThat(updatedDoc, is(notNullValue()));
		assertThat(updatedDoc.dbRefProperty, is(notNullValue()));
		assertThat(updatedDoc.dbRefProperty.id, is(sample2.id));
		assertThat(updatedDoc.dbRefProperty.field, is(sample2.field));
	}

	/**
	 * @see DATA_JIRA-862
	 */
	@Test
	@Ignore // not support $ in sdb
	public void testUpdateShouldWorkForPathsOnInterfaceMethods() {
		prepareCollection(DocumentWithCollection.class);
		DocumentWithCollection document = new DocumentWithCollection(Arrays.<Model> asList(new ModelA("spring"),
				new ModelA("data")));

		template.save(document);

		Query query = query(where("id").is(document.id).and("models._id").exists(1));
		Update update = new Update().set("models.$.value", "sequoiadb");
		template.findAndModify(query, update, DocumentWithCollection.class);

		DocumentWithCollection result = template.findOne(query(where("id").is(document.id)), DocumentWithCollection.class);
		assertThat(result.models.get(0).value(), is("sequoiadb"));
	}

	/**
	 * @see DATA_JIRA-773
	 */
	@Test
	@Ignore // not support in sdb
	public void testShouldSupportQueryWithIncludedDbRefField() {
		prepareCollection(Sample.class);
		Sample sample = new Sample("47111", "foo");
		template.save(sample);

		DocumentWithDBRefCollection doc = new DocumentWithDBRefCollection();
		doc.id = "4711";
		doc.dbRefProperty = sample;

		template.save(doc);

		Query qry = query(where("id").is(doc.id));
		qry.fields().include("dbRefProperty", 1);

		List<DocumentWithDBRefCollection> result = template.find(qry, DocumentWithDBRefCollection.class);

		assertThat(result, is(notNullValue()));
		assertThat(result, hasSize(1));
		assertThat(result.get(0), is(notNullValue()));
		assertThat(result.get(0).dbRefProperty, is(notNullValue()));
		assertThat(result.get(0).dbRefProperty.field, is(sample.field));
	}

	/**
	 * @see DATA_JIRA-566
	 */
	@Test
	@Ignore // not support cursor.count() in sdb
	public void testFindAllAndRemoveFullyReturnsAndRemovesDocuments() {
		prepareCollection(Sample.class);
		Sample spring = new Sample("100", "spring");
		Sample data = new Sample("200", "data");
		Sample sequoiadb = new Sample("300", "sequoiadb");
		template.insert(Arrays.asList(spring, data, sequoiadb), Sample.class);

		Query qry = query(where("field").in("spring", "sequoiadb"));
		List<Sample> result = template.findAllAndRemove(qry, Sample.class);

		assertThat(result, hasSize(2));

		assertThat(
				template.getDb().getCollection("sample")
						.find(new BasicBSONObject("field", new BasicBSONObject("$in", Arrays.asList("spring", "sequoiadb")))).count(),
				is(0));
		assertThat(template.getDb().getCollection("sample").find(new BasicBSONObject("field", "data")).count(), is(1));
	}

	/**
	 * @see DATA_JIRA-1001
	 */
	@Test
	@Ignore // not support DBRef in sdb
	public void shouldAllowSavingOfLazyLoadedDbRefs() {

		template.dropCollection(SomeTemplate.class);
		template.dropCollection(SomeMessage.class);
		template.dropCollection(SomeContent.class);

		SomeContent content = new SomeContent();
		content.id = "content-1";
		content.text = "spring";
		template.save(content);

		SomeTemplate tmpl = new SomeTemplate();
		tmpl.id = "template-1";
		tmpl.content = content; // @DBRef(lazy=true) tmpl.content

		template.save(tmpl);

		SomeTemplate savedTmpl = template.findById(tmpl.id, SomeTemplate.class);

		SomeContent loadedContent = savedTmpl.getContent();
		loadedContent.setText("data");
		template.save(loadedContent);

		assertThat(template.findById(content.id, SomeContent.class).getText(), is("data"));

	}

	/**
	 * @see DATA_JIRA-880
	 */
	@Test
	@Ignore // not support DBRef in sdb
	public void savingAndReassigningLazyLoadingProxies() {

		template.dropCollection(SomeTemplate.class);
		template.dropCollection(SomeMessage.class);
		template.dropCollection(SomeContent.class);

		SomeContent content = new SomeContent();
		content.id = "C1";
		content.text = "BUBU";
		template.save(content);

		SomeTemplate tmpl = new SomeTemplate();
		tmpl.id = "T1";
		tmpl.content = content; // @DBRef(lazy=true) tmpl.content

		template.save(tmpl);

		SomeTemplate savedTmpl = template.findById(tmpl.id, SomeTemplate.class);

		SomeMessage message = new SomeMessage();
		message.id = "M1";
		message.dbrefContent = savedTmpl.content; // @DBRef message.dbrefContent
		message.normalContent = savedTmpl.content;

		template.save(message);

		SomeMessage savedMessage = template.findById(message.id, SomeMessage.class);

		assertThat(savedMessage.dbrefContent.text, is(content.text));
		assertThat(savedMessage.normalContent.text, is(content.text));
	}

	/**
	 * @see DATA_JIRA-884
	 */
	@Test
	@Ignore // not support DBRef in sdb
	public void callingNonObjectMethodsOnLazyLoadingProxyShouldReturnNullIfUnderlyingDbrefWasDeletedInbetween() {

		template.dropCollection(SomeTemplate.class);
		template.dropCollection(SomeContent.class);

		SomeContent content = new SomeContent();
		content.id = "C1";
		content.text = "BUBU";
		template.save(content);

		SomeTemplate tmpl = new SomeTemplate();
		tmpl.id = "T1";
		tmpl.content = content; // @DBRef(lazy=true) tmpl.content

		template.save(tmpl);

		SomeTemplate savedTmpl = template.findById(tmpl.id, SomeTemplate.class);

		template.remove(content);

		assertThat(savedTmpl.getContent().toString(), is("someContent:C1$LazyLoadingProxy"));
		assertThat(savedTmpl.getContent(), is(instanceOf(LazyLoadingProxy.class)));
		assertThat(savedTmpl.getContent().getText(), is(nullValue()));
	}

	/**
	 * @see DATA_JIRA-471
	 */
	@Test
	@Ignore // not support $each and $addToSet, but we try to let it done.
	public void updateMultiShouldAddValuesCorrectlyWhenUsingAddToSetWithEach() {
	}

	/**
	 * @see DATA_JIRA-888
	 */
	@Test
	public void sortOnIdFieldPropertyShouldBeMappedCorrectly() {
		prepareCollection(DoucmentWithNamedIdField.class);
		DoucmentWithNamedIdField one = new DoucmentWithNamedIdField();
		one.someIdKey = "1";
		one.value = "a";

		DoucmentWithNamedIdField two = new DoucmentWithNamedIdField();
		two.someIdKey = "2";
		two.value = "b";

		template.save(one);
		template.save(two);

		Query query = query(where("_id").in("1", "2")).with(new Sort(Direction.DESC, "someIdKey"));
		assertThat(template.find(query, DoucmentWithNamedIdField.class), contains(two, one));
	}

	/**
	 * @see DATA_JIRA-888
	 */
	@Test
	public void sortOnAnnotatedFieldPropertyShouldBeMappedCorrectly() {
		prepareCollection(DoucmentWithNamedIdField.class);
		DoucmentWithNamedIdField one = new DoucmentWithNamedIdField();
		one.someIdKey = "1";
		one.value = "a";

		DoucmentWithNamedIdField two = new DoucmentWithNamedIdField();
		two.someIdKey = "2";
		two.value = "b";

		template.save(one);
		template.save(two);

		Query query = query(where("_id").in("1", "2")).with(new Sort(Direction.DESC, "value"));
		assertThat(template.find(query, DoucmentWithNamedIdField.class), contains(two, one));
	}

	/**
	 * @see DATA_JIRA-913
	 */
	@Test
	@Ignore // notsupport DBRef in sdb
	public void shouldRetrieveInitializedValueFromDbRefAssociationAfterLoad() {
		prepareCollection(SomeContent.class);
		SomeContent content = new SomeContent();
		content.id = "content-1";
		content.name = "Content 1";
		content.text = "Some text";

		template.save(content);

		SomeTemplate tmpl = new SomeTemplate();
		tmpl.id = "template-1";
		tmpl.content = content;

		template.save(tmpl);

		SomeTemplate result = template.findOne(query(where("content").is(tmpl.getContent())), SomeTemplate.class);

		assertThat(result, is(notNullValue()));
		assertThat(result.getContent(), is(notNullValue()));
		assertThat(result.getContent().getId(), is(notNullValue()));
		assertThat(result.getContent().getName(), is(notNullValue()));
		assertThat(result.getContent().getText(), is(content.getText()));
	}

	/**
	 * @see DATA_JIRA-913
	 */
	@Test
	@Ignore // notsupport DBRef in sdb
	public void shouldReuseExistingDBRefInQueryFromDbRefAssociationAfterLoad() {
		prepareCollection(SomeContent.class);
		SomeContent content = new SomeContent();
		content.id = "content-1";
		content.name = "Content 1";
		content.text = "Some text";

		template.save(content);

		SomeTemplate tmpl = new SomeTemplate();
		tmpl.id = "template-1";
		tmpl.content = content;

		template.save(tmpl);

		SomeTemplate result = template.findOne(query(where("content").is(tmpl.getContent())), SomeTemplate.class);

		result = template.findOne(query(where("content").is(result.getContent())), SomeTemplate.class);

		assertNotNull(result.getContent().getName());
		assertThat(result.getContent().getName(), is(content.getName()));
	}

	/**
	 * @see DATA_JIRA-970
	 */
	@Test
	public void insertsAndRemovesBasicDbObjectCorrectly() {
		prepareCollection("collection");
		BasicBSONObject object = new BasicBSONObject("key", "value");
		template.insert(object, "collection");

		assertThat(object.get("_id"), is(notNullValue()));
		assertThat(template.findAll(BSONObject.class, "collection"), hasSize(1));

		template.remove(object, "collection");
		assertThat(template.findAll(BSONObject.class, "collection"), hasSize(0));
	}

	static class DoucmentWithNamedIdField {

		@Id String someIdKey;

		@Field(value = "val")//
		String value;

		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + (someIdKey == null ? 0 : someIdKey.hashCode());
			result = prime * result + (value == null ? 0 : value.hashCode());
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj) {
				return true;
			}
			if (obj == null) {
				return false;
			}
			if (!(obj instanceof DoucmentWithNamedIdField)) {
				return false;
			}
			DoucmentWithNamedIdField other = (DoucmentWithNamedIdField) obj;
			if (someIdKey == null) {
				if (other.someIdKey != null) {
					return false;
				}
			} else if (!someIdKey.equals(other.someIdKey)) {
				return false;
			}
			if (value == null) {
				if (other.value != null) {
					return false;
				}
			} else if (!value.equals(other.value)) {
				return false;
			}
			return true;
		}

		@Override
		public String toString() {
			return String.format("someIdKey: %s, value: %s", someIdKey, value);
		}
	}

	static class DocumentWithDBRefCollection {

		@Id public String id;

		@Field("db_ref_list")/** @see DATA_JIRA-1058 */
		@org.springframework.data.sequoiadb.core.mapping.DBRef//
		public List<Sample> dbRefAnnotatedList;

		@org.springframework.data.sequoiadb.core.mapping.DBRef//
		public Sample dbRefProperty;
	}

	static class DocumentWithCollection {

		@Id String id;
		List<Model> models;

		DocumentWithCollection(List<Model> models) {
			this.models = models;
		}
	}

	static class DocumentWithCollectionOfSimpleType {

		@Id String id;
		List<String> values;
	}

	static class DocumentWithMultipleCollections {
		@Id String id;
		List<String> string1;
		List<String> string2;
	}

	static interface Model {
		String value();

		String id();
	}

	static class ModelA implements Model {

		@Id String id;
		private String value;

		ModelA(String value) {
			this.value = value;
		}

		@Override
		public String value() {
			return this.value;
		}

		@Override
		public String id() {
			return id;
		}
	}

	static class Document {

		@Id public String id;
		public Model model;
	}

	static class MyId {

		String first;
		String second;
	}

	static class TypeWithMyId {

		@Id MyId id;
	}

	static class Sample {

		@Id String id;
		String field;

		public Sample() {}

		public Sample(String id, String field) {
			this.id = id;
			this.field = field;
		}
	}

	static class TestClass {

		DateTime myDate;

		@PersistenceConstructor
		TestClass(DateTime myDate) {
			this.myDate = myDate;
		}
	}

	static class PersonWithConvertedId {

		String id;
		String name;
	}

	static enum DateTimeToDateConverter implements Converter<DateTime, Date> {

		INSTANCE;

		public Date convert(DateTime source) {
			return source == null ? null : source.toDate();
		}
	}

	static enum DateToDateTimeConverter implements Converter<Date, DateTime> {

		INSTANCE;

		public DateTime convert(Date source) {
			return source == null ? null : new DateTime(source.getTime());
		}
	}

	public static class MyPerson {

		String id;
		String name;
		Address address;

		public String getName() {
			return name;
		}
	}

	static class Address {

		String state;
		String city;
	}

	static class VersionedPerson {

		@Version Long version;
		String id, firstname, lastname;
	}

	static class TypeWithFieldAnnotation {

		@Id ObjectId id;
		@Field("email") String emailAddress;
	}

	static class TypeWithDate {

		@Id String id;
		Date date;
	}

	static class ObjectWith3AliasedFields {

		@Id String id;
		@Field("prop1") String property1;
		@Field("prop2") String property2;
		@Field("prop3") String property3;
	}

	static class ObjectWith3AliasedFieldsAndNestedAddress extends ObjectWith3AliasedFields {
		@Field("adr") Address address;
	}

	static enum EnumValue {
		VALUE1, VALUE2, VALUE3
	}

	static class ObjectWithEnumValue {

		@Id String id;
		EnumValue value;
	}

	public static class SomeTemplate {

		String id;
		@org.springframework.data.sequoiadb.core.mapping.DBRef(lazy = true) SomeContent content;

		public SomeContent getContent() {
			return content;
		}
	}

	public static class SomeContent {

		String id;
		String text;
		String name;

		public String getName() {
			return name;
		}

		public void setText(String text) {
			this.text = text;

		}

		public String getId() {
			return id;
		}

		public String getText() {
			return text;
		}
	}

	static class SomeMessage {
		String id;
		@org.springframework.data.sequoiadb.core.mapping.DBRef SomeContent dbrefContent;
		SomeContent normalContent;
	}
}
