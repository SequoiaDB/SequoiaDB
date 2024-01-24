/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <bson.h>
#include <mongoc.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _MSC_VER
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <syscall.h>
#endif
#include "TestSuite.h"
#include "test-libmongoc.h"
#include "mongoc-tests.h"


extern void test_bulk_install             (TestSuite *suite);
extern void test_client_install           (TestSuite *suite);
extern void test_client_pool_install      (TestSuite *suite);
extern void test_collection_install       (TestSuite *suite);
extern void test_cursor_install           (TestSuite *suite);
extern void test_database_install         (TestSuite *suite);
extern void test_gridfs_install           (TestSuite *suite);
extern void test_load_install             (TestSuite *suite);


static int gSuppressCount;
TestSuite suite; 


void
suppress_one_message (void)
{
   gSuppressCount++;
}


static void
log_handler (mongoc_log_level_t  log_level,
             const char         *log_domain,
             const char         *message,
             void               *user_data)
{
   if (gSuppressCount) {
      gSuppressCount--;
      return;
   }
   if (log_level < MONGOC_LOG_LEVEL_INFO) {
      mongoc_log_default_handler (log_level, log_domain, message, NULL);
   }
}


char MONGOC_TEST_HOST [1024];
char MONGOC_TEST_UNIQUE [32];
#ifdef _MSC_VER
typedef DWORD OSSTID;
#else
typedef pthread_t OSSTID;
#endif

static OSSTID ossGetCurrentThreadID()
{
#ifdef _MSC_VER
   return GetCurrentThreadId();
#else
   return syscall(SYS_gettid);
#endif
}
char *
gen_database_name(const char *str)
{
   return bson_strdup_printf ("%s_%u",
                               str,
                               (unsigned)gettestpid());
}
char *
gen_collection_name (const char *str)
{
   return bson_strdup_printf ("%s_%u_%u_%u",
                              str,
                              (unsigned)time(NULL),
                              (unsigned)gettestpid(),
                              (unsigned)ossGetCurrentThreadID());

}

static void
set_mongoc_test_host(void)
{
#ifdef _MSC_VER
   size_t buflen;

   if (0 != getenv_s (&buflen, MONGOC_TEST_HOST, sizeof MONGOC_TEST_HOST, "MONGOC_TEST_HOST")) {
      bson_strncpy (MONGOC_TEST_HOST, "localhost", sizeof MONGOC_TEST_HOST);
   }
#else
   if (getenv("MONGOC_TEST_HOST")) {
      bson_strncpy (MONGOC_TEST_HOST, getenv("MONGOC_TEST_HOST"), sizeof MONGOC_TEST_HOST);
   } else {
      bson_strncpy (MONGOC_TEST_HOST, "localhost", sizeof MONGOC_TEST_HOST);
   }
#endif
}

mongoc_collection_t * create_collection(char* name, mongoc_database_t *database)
{
   mongoc_collection_t *collection = NULL;
   bson_error_t error = { 0 };
   bson_t options;
   bson_t wt_opts;
   bson_t storage_opts;
   bson_init (&options);
   BSON_APPEND_INT32 (&options, "size", 1234);
   BSON_APPEND_INT32 (&options, "max", 4567);
   BSON_APPEND_BOOL (&options, "capped", true);
   BSON_APPEND_BOOL (&options, "autoIndexId", true);

   BSON_APPEND_DOCUMENT_BEGIN(&options, "storage", &storage_opts);
   BSON_APPEND_DOCUMENT_BEGIN(&storage_opts, "wiredtiger", &wt_opts);
   BSON_APPEND_UTF8(&wt_opts, "configString", "block_compressor=zlib");
   bson_append_document_end(&storage_opts, &wt_opts);
   bson_append_document_end(&options, &storage_opts);


   collection = mongoc_database_create_collection (database, name, &options, &error);
   if (!collection)
   {
      MONGOC_ERROR("createCL:%s", error.message);
   }
   bson_destroy(&options);
   
   return collection;
}

static mongoc_database_t *
get_test_database (mongoc_client_t *client)
{
   static mongoc_database_t * database;
   char *databasename = "fapmongo_test";
  // databasename = gen_database_name("fapmongo_test");
   database =  mongoc_client_get_database (client, databasename);
  // free(databasename);
   return database;
}

mongoc_collection_t*  setUp(char* name, mongoc_client_t *client)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   char *fullname;

   fullname = gen_collection_name (name);
   database = get_test_database(client);
   ASSERT_CMPPTR(database, !=, NULL);
   collection = create_collection (fullname, database);
   ASSERT_CMPPTR (collection, !=, NULL);

   bson_free (fullname);
   mongoc_database_destroy(database);
   return collection;
}

void tearDown(mongoc_collection_t* col)
{
   bson_error_t error;
   bool r;
   r = mongoc_collection_drop (col, &error);
   if (!r)
   {
      MONGOC_ERROR("dropCL: %s", error.message);
   }
   ASSERT_CMPINT (r, ==, true);
   mongoc_collection_destroy (col);
   return;
}

int check_records(mongoc_collection_t *col, bson_t q, bool checkoid)
{
   bool r;
   const bson_t *doc;
   int recordnum = 0;
   mongoc_cursor_t *cursor;
   bson_error_t    error;
   bson_iter_t iter;

   cursor = mongoc_collection_find(col, MONGOC_QUERY_NONE, 0, 0, 0,
                                   &q, NULL, NULL);
   ASSERT_CMPPTR(cursor, !=, NULL);

   while(mongoc_cursor_next(cursor, &doc))
   {
      ASSERT_CMPPTR(doc, !=, NULL);
      if (( bson_iter_init(&iter, doc)  &&
            bson_iter_find(&iter, "_id") ))
      {
         if (checkoid && (BSON_TYPE_OID == bson_iter_type(&iter)))
         {
            ++recordnum;
            continue;
         }
         else if(BSON_TYPE_INT32 == bson_iter_type(&iter))
         {
            ++recordnum;
            continue;
         }
      }
      char *str = NULL;
      str = (char*)bson_as_json(doc,0);
      MONGOC_ERROR("%s", str);
      bson_free(str);
      ASSERT_CMPINT(false, !=, false);
   }
   
   if (mongoc_cursor_error(cursor, &error)) {
      MONGOC_ERROR("find:%s", error.message);
      ASSERT_CMPINT(false, !=, false);
   }
   
   mongoc_cursor_destroy(cursor); 
   return recordnum;
}

int
main (int   argc,
      char *argv[])
{
   //TestSuite suite;
   int ret;

   mongoc_init ();

   bson_snprintf (MONGOC_TEST_UNIQUE, sizeof MONGOC_TEST_UNIQUE,
                  "test_%u_%u", (unsigned)time (NULL),
                  (unsigned)gettestpid ());

   set_mongoc_test_host ();

   mongoc_log_set_handler (log_handler, NULL);

   TestSuite_Init (&suite, "", argc, argv);

   test_client_install (&suite);
   test_client_pool_install (&suite);
   test_bulk_install (&suite);
   test_collection_install (&suite);
   test_cursor_install (&suite);
   test_database_install (&suite);
   test_load_install(&suite);
   //test_gridfs_install (&suite);

   ret = TestSuite_Run (&suite);

   TestSuite_Destroy (&suite);

   mongoc_cleanup();

   return ret;
}
