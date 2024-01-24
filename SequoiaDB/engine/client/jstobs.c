/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/

#include "jstobs.h"
#include "cJSON_ext.h"
#include "base64c.h"
#include "timestamp.h"
#include "ossUtil.h"

#define INT_NUM_SIZE 32

#define INT64_FIRST_YEAR 0
#define INT64_LAST_YEAR 9999
#define INT32_LAST_YEAR 2038

#define RELATIVE_YEAR 1900
#define RELATIVE_MON 12
#define RELATIVE_DAY 31
#define RELATIVE_HOUR 24
#define RELATIVE_MIN_SEC 60

#define BSON_TEMP_SIZE_32 32
#define BSON_TEMP_SIZE_64 64
#define BSON_TEMP_SIZE_512 512

#define LONG_JS_MIN (-9007199254740991LL)
#define LONG_JS_MAX  (9007199254740991LL)

#define TIME_FORMAT  "%d-%d-%d-%d.%d.%d.%d"
#define TIME_FORMAT2 "%d-%d-%d-%d:%d:%d.%d"
#define DATE_FORMAT  "%d-%d-%d"

#define DATE_OUTPUT_CSV_FORMAT "%04d-%02d-%02d"
#define DATE_OUTPUT_FORMAT "{ \"$date\": \"" DATE_OUTPUT_CSV_FORMAT "\" }"

#define TIME_OUTPUT_CSV_FORMAT "%04d-%02d-%02d-%02d.%02d.%02d.%06d"
#define TIME_OUTPUT_FORMAT "{ \"$timestamp\": \"" TIME_OUTPUT_CSV_FORMAT "\" }"

#define RECORD_ID_NAME "_id"

static void get_char_num ( CHAR *str, INT32 i, INT32 str_size ) ;
static CHAR* intToString ( INT32 value, CHAR *string, INT32 radix ) ;
static BOOLEAN date2Time( const CHAR *pDate,
                          CJSON_VALUE_TYPE valType,
                          time_t *pTimestamp,
                          INT32 *pMicros ) ;
static void bsonConvertJsonRawConcat( CHAR **pbuf,
                                      INT32 *left,
                                      const CHAR *data,
                                      BOOLEAN isString ) ;
static BOOLEAN bsonConvertJson( CHAR **pbuf,
                                INT32 *left,
                                const CHAR *data ,
                                INT32 isobj,
                                BOOLEAN toCSV,
                                BOOLEAN skipUndefined,
                                BOOLEAN isStrict ) ;
static INT32 strlen_a( const CHAR *data ) ;
static void local_time( time_t *Time, struct tm *TM ) ;

typedef void (*JSON_PLOG_FUNC)( const CHAR *pFunc, \
                                const CHAR *pFile, \
                                UINT32 line, \
                                const CHAR *pFmt, \
                                ... ) ;

JSON_PLOG_FUNC _pJsonPrintfLogFun ;

#define JSON_PRINTF_LOG( fmt, ... )\
{\
   if( _pJsonPrintfLogFun != NULL )\
   {\
      _pJsonPrintfLogFun( __FUNC__, __FILE__, __LINE__, fmt, ##__VA_ARGS__ ) ;\
   }\
}

/*
 * check the remaining size
 * x : the remaining size
*/
#define CHECK_LEFT(x) \
{ \
   if ( (*x) <= 0 ) \
      return FALSE ; \
}

/* This function takes input from cJSON library, which parses a string to linked
 * list. In this function we iterate all elements in list and construct BSON
 * accordingly */
static BOOLEAN jsonConvertBson( const CJSON_MACHINE *pMachine,
                                const cJson_iterator *pIter,
                                bson *pBson,
                                BOOLEAN *hasId,
                                BOOLEAN isObj,
                                INT32 decimalto ) ;

/*
 * pFun print log function
*/
SDB_EXPORT  void JsonSetPrintfLog( void (*pFun)( const CHAR *pFunc,
                                                 const CHAR *pFile,
                                                 UINT32 line,
                                                 const CHAR *pFmt,
                                                 ... ) )
{
   _pJsonPrintfLogFun = pFun ;
   cJsonSetPrintfLog( pFun ) ;
}

//Compatible with the old version, the new code is not recommended use.
SDB_EXPORT BOOLEAN jsonToBson ( bson *bs, const CHAR *json_str )
{
   return json2bson2( json_str, bs ) ;
}

//Compatible with the old version, the new code is not recommended use.
SDB_EXPORT BOOLEAN jsonToBson2 ( bson *bs,
                                 const CHAR *json_str,
                                 BOOLEAN isMongo,
                                 BOOLEAN isBatch )
{
   return json2bson( json_str, NULL, CJSON_RIGOROUS_PARSE,
                     !isBatch, TRUE, JSON_DECIMAL_NO_CONVERT, bs ) ;
}

/*
 * json convert bson interface
 * pJson : json string
 * pBson : bson object
 * return : the conversion result
*/
SDB_EXPORT BOOLEAN json2bson2( const CHAR *pJson, bson *pBson )
{
   return json2bson( pJson, NULL, CJSON_RIGOROUS_PARSE, TRUE, TRUE,
                     JSON_DECIMAL_NO_CONVERT, pBson ) ;
}

/*
 * json convert bson interface
 * pJson : json string
 * pMachine : cJSON state machine
 * parseMode: 0 - loose mode
 *            1 - rigorous mode
 * isCheckEnd: whether to check the end of json
 * isUnicode: whether to escape Unicode encoding
 * pBson : bson object
 * return : the conversion result
*/
/* THIS IS EXTERNAL FUNCTION TO CONVERT FROM JSON STRING INTO BSON OBJECT */
SDB_EXPORT BOOLEAN json2bson( const CHAR *pJson,
                              CJSON_MACHINE *pMachine,
                              INT32 parseMode,
                              BOOLEAN isCheckEnd,
                              BOOLEAN isUnicode,
                              INT32 decimalto,
                              bson *pBson )
{
   INT32 flags = 0 ;

   if ( CJSON_RIGOROUS_PARSE == parseMode )
   {
      flags |= JSON_FLAG_RIGOROUS_MODE ;
   }

   if ( isCheckEnd )
   {
      flags |= JSON_FLAG_CHECK_END ;
   }

   if ( isUnicode )
   {
      flags |= JSON_FLAG_ESCAPE_UNICODE ;
   }

   if ( JSON_DECIMAL_TO_DOUBLE == decimalto )
   {
      flags |= JSON_FLAG_DECIMAL_TO_DOUBLE ;
   }
   else if ( JSON_DECIMAL_TO_STRING == decimalto )
   {
      flags |= JSON_FLAG_DECIMAL_TO_STRING ;
   }

   return json2bson3( pJson, pMachine, flags, pBson ) ;
}

/*
 * json convert bson interface
 * pJson : json string
 * pMachine : cJSON state machine
 * flags: converted parameters
 * pBson : bson object
 * return : the conversion result
*/
/* THIS IS EXTERNAL FUNCTION TO CONVERT FROM JSON STRING INTO BSON OBJECT */
SDB_EXPORT BOOLEAN json2bson3( const CHAR *pJson, CJSON_MACHINE *pMachine,
                               INT32 flags, bson *pBson )
{
   INT32 parseMode = 0 ;
   INT32 decimalto = JSON_DECIMAL_NO_CONVERT ;
   BOOLEAN result = TRUE ;
   BOOLEAN isOwn  = FALSE ;
   BOOLEAN hasId  = FALSE ;
   BOOLEAN isCheckEnd = JSON_FLAG_CHECK_END & flags ;
   BOOLEAN isUnicode  = JSON_FLAG_ESCAPE_UNICODE & flags ;
   const cJson_iterator *pIter = NULL ;

   if ( JSON_FLAG_RIGOROUS_MODE & flags )
   {
      parseMode = 1 ;
   }

   if ( JSON_FLAG_DECIMAL_TO_DOUBLE & flags )
   {
      decimalto = JSON_DECIMAL_TO_DOUBLE ;
   }
   else if ( JSON_FLAG_DECIMAL_TO_STRING & flags )
   {
      decimalto = JSON_DECIMAL_TO_STRING ;
   }

   cJsonExtAppendFunction() ;

   if( pMachine == NULL )
   {
      isOwn = TRUE ;
      pMachine = cJsonCreate() ;
      if( pMachine == NULL )
      {
         JSON_PRINTF_LOG( "Failed to call cJsonCreate" ) ;
         goto error ;
      }
   }

   cJsonInit( pMachine, parseMode, isCheckEnd, isUnicode ) ;

   if( cJsonParse( pJson, pMachine ) == FALSE )
   {
      JSON_PRINTF_LOG( "Failed to call cJsonParse" ) ;
      goto error ;
   }

   pIter = cJsonIteratorInit( pMachine ) ;
   if( pIter == NULL )
   {
      JSON_PRINTF_LOG( "Failed to init iterator" ) ;
      goto error ;
   }

   if ( !( JSON_FLAG_NOT_INIT_BSON & flags ) )
   {
      bson_init( pBson ) ;
   }

   if( jsonConvertBson( pMachine, pIter, pBson, &hasId,
                        TRUE, decimalto ) == FALSE )
   {
      JSON_PRINTF_LOG( "Failed to convert json to bson" ) ;
      goto error ;
   }

   if ( !hasId && ( JSON_FLAG_APPEND_OID & flags ) )
   {
      INT32 rc = SDB_OK ;
      bson_oid_t oid ;

      bson_oid_gen( &oid ) ;
      rc = bson_append_oid( pBson, RECORD_ID_NAME, &oid ) ;
      if ( rc )
      {
         JSON_PRINTF_LOG( "failed to append record id, rc=%d", rc ) ;
         goto error;
      }
   }

   if( bson_finish( pBson ) == BSON_ERROR )
   {
      JSON_PRINTF_LOG( "Failed to call bson_finish" ) ;
      goto error ;
   }

done:
   if( isOwn == TRUE )
   {
      cJsonRelease( pMachine ) ;
   }
   return result ;
error:
   result = FALSE ;
   goto done ;
}

static CHAR _precision[20] = "%.16g" ;

void setJsonPrecision( const CHAR *pFloatFmt )
{
   if( pFloatFmt != NULL )
   {
      INT32 length = strlen( pFloatFmt ) ;
      length = length > 16 ? 16 : length ;
      strncpy( _precision, pFloatFmt, length ) ;
      _precision[ length ] = 0 ;
   }
}

/*
 * bson convert json interface
 * buffer : output bson convert json string
 * bufsize : buffer's size
 * b : bson object
 * return : the conversion result
*/
/* THIS IS EXTERNAL FUNCTION TO CONVERT FROM BSON OBJECT INTO JSON STRING */
BOOLEAN bsonToJson ( CHAR *buffer, INT32 bufsize, const bson *b,
                     BOOLEAN toCSV, BOOLEAN skipUndefined )
{
    CHAR *pbuf = buffer ;
    BOOLEAN result = FALSE ;
    INT32 leftsize = bufsize ;
    if ( bufsize <= 0 || !buffer || !b )
       return FALSE ;
    //memset ( pbuf, 0, bufsize ) ;
    result = bsonConvertJson ( &pbuf, &leftsize, b->data, 1,
                               toCSV, skipUndefined, FALSE ) ;
    if ( !result || !leftsize )
       return FALSE ;
    *pbuf = '\0' ;
    return TRUE ;
}

/*
 * bson convert json interface
 * buffer : output bson convert json string
 * bufsize : buffer's size
 * b : bson object
 * isStrict: Strict export of data types
 * return : the conversion result
*/
/* THIS IS EXTERNAL FUNCTION TO CONVERT FROM BSON OBJECT INTO JSON STRING */
BOOLEAN bsonToJson2 ( CHAR *buffer, INT32 bufsize, const bson *b,
                      BOOLEAN isStrict )
{
    CHAR *pbuf = buffer ;
    BOOLEAN result = FALSE ;
    INT32 leftsize = bufsize ;
    if ( bufsize <= 0 || !buffer || !b )
       return FALSE ;
    //memset ( pbuf, 0, bufsize ) ;
    result = bsonConvertJson ( &pbuf, &leftsize, b->data, 1,
                               FALSE, TRUE, isStrict ) ;
    if ( !result || !leftsize )
       return FALSE ;
    *pbuf = '\0' ;
    return TRUE ;
}


static BOOLEAN date2Time( const CHAR *pDate,
                          CJSON_VALUE_TYPE valType,
                          time_t *pTimestamp,
                          INT32 *pMicros )
{
   /*
      eg. before 1927-12-31-23.54.07,
      will be more than 352 seconds
      UTC time
      date min 0000-01-01-00.00.00.000000
      date max 9999-12-31-23.59.59.999999
      timestamp min 1901-12-13-20.45.52.000000 +/- TZ
      timestamp max 2038-01-19-03.14.07.999999 +/- TZ
   */
   /* date and timestamp */
   BOOLEAN flag = TRUE ;
   INT32 year   = 0 ;
   INT32 month  = 0 ;
   INT32 day    = 0 ;
   INT32 hour   = 0 ;
   INT32 minute = 0 ;
   INT32 second = 0 ;
   INT32 micros = 0 ;
   time_t timep = 0 ;
   struct tm t ;

   ossMemset( &t, 0, sizeof( t ) ) ;
   if( ossStrchr( pDate, 'T' ) ||
       ossStrchr( pDate, 't' ) )
   {
      /* for mongo date type, iso8601 */
      sdbTimestamp sdbTime ;
      if( timestampParse( pDate,
                          ossStrlen( pDate ),
                          &sdbTime ) )
      {
         JSON_PRINTF_LOG( "Failed to parse timestamp" ) ;
         goto error ;
      }
      timep = (time_t)sdbTime.sec ;
      micros = sdbTime.nsec / 1000 ;
   }
   else
   {
      if( valType == CJSON_TIMESTAMP )
      {
         /* for timestamp type, we provide yyyy-mm-dd-hh.mm.ss.uuuuuu */
         BOOLEAN hasColon = FALSE ;
         if( ossStrchr( pDate, ':' ) )
         {
            hasColon = TRUE ;
         }

         if( !sscanf ( pDate,
                       hasColon ? TIME_FORMAT2 : TIME_FORMAT,
                       &year,
                       &month,
                       &day,
                       &hour,
                       &minute,
                       &second,
                       &micros ) )
         {
            JSON_PRINTF_LOG( "Failed to parse timestamp" ) ;
            goto error ;
         }
      }
      else
      {
         if( !sscanf ( pDate,
                       DATE_FORMAT,
                       &year,
                       &month,
                       &day ) )
         {
            JSON_PRINTF_LOG( "Failed to parse date" ) ;
            goto error ;
         }
      }
      /* sanity check for years */
      if( valType == CJSON_TIMESTAMP )
      {
         //[ 1901, 2038 ]
         if( year > INT32_LAST_YEAR )
         {
            JSON_PRINTF_LOG( "Timestamp year not greater than %d",
                             INT32_LAST_YEAR ) ;
            goto error ;
         }
         else if( year < RELATIVE_YEAR )
         {
            JSON_PRINTF_LOG( "Timestamp year not less than %d",
                             RELATIVE_YEAR + 1 ) ;
            goto error ;
         }

         //[1,12]
         if( month > RELATIVE_MON )
         {
            JSON_PRINTF_LOG( "Timestamp month not greater than %d",
                             RELATIVE_MON ) ;
            goto error ;
         }
         else if( month < 1 )
         {
            JSON_PRINTF_LOG( "Timestamp month not less than 1" ) ;
            goto error ;
         }

         //[1,31]
         if( day > RELATIVE_DAY )
         {
            JSON_PRINTF_LOG( "Timestamp day not greater than %d",
                             RELATIVE_DAY ) ;
            goto error ;
         }
         else if( day < 1 )
         {
            JSON_PRINTF_LOG( "Timestamp day not less than 1" ) ;
            goto error ;
         }

         //[0,23]
         if( hour >= RELATIVE_HOUR )
         {
            JSON_PRINTF_LOG( "Timestamp hours not greater than %d",
                             RELATIVE_HOUR ) ;
            goto error ;
         }
         else if( hour < 0 )
         {
            JSON_PRINTF_LOG( "Timestamp hours not less than 0" ) ;
            goto error ;
         }

         //[0,59]
         if( minute >= RELATIVE_MIN_SEC )
         {
            JSON_PRINTF_LOG( "Timestamp minutes not greater than %d",
                             RELATIVE_MIN_SEC ) ;
            goto error ;
         }
         else if( minute <  0 )
         {
            JSON_PRINTF_LOG( "Timestamp minutes not less than 0" ) ;
            goto error ;
         }

         //[0,59]
         if( second >= RELATIVE_MIN_SEC )
         {
            JSON_PRINTF_LOG( "Timestamp seconds not greater than %d",
                             RELATIVE_MIN_SEC ) ;
            goto error ;
         }
         else if( second < 0 )
         {
            JSON_PRINTF_LOG( "Timestamp seconds not less than 0" ) ;
            goto error ;
         }
      }
      else if( valType == CJSON_DATE )
      {
         //[0000,9999]
         if( year > INT64_LAST_YEAR )
         {
            JSON_PRINTF_LOG( "Date year not greater than %d",
                             INT64_LAST_YEAR ) ;
            goto error ;
         }
         else if( year < INT64_FIRST_YEAR )
         {
            JSON_PRINTF_LOG( "Date year not less than %d", INT64_FIRST_YEAR ) ;
            goto error ;
         }

         //[1,12]
         if( month > RELATIVE_MON )
         {
            JSON_PRINTF_LOG( "Date month not greater than %d",
                             RELATIVE_MON ) ;
            goto error ;
         }
         else if( month < 1 )
         {
            JSON_PRINTF_LOG( "Date month not less than 1" ) ;
            goto error ;
         }

         //[1,31]
         if( day > RELATIVE_DAY )
         {
            JSON_PRINTF_LOG( "Date day not greater than %d",
                             RELATIVE_DAY ) ;
            goto error ;
         }
         else if( day < 1 )
         {
            JSON_PRINTF_LOG( "Date day not less than 1" ) ;
            goto error ;
         }
      }
      --month ;
      year -= RELATIVE_YEAR ;
      /* construct tm */
      t.tm_year  = year   ;
      t.tm_mon   = month  ;
      t.tm_mday  = day    ;
      t.tm_hour  = hour   ;
      t.tm_min   = minute ;
      t.tm_sec   = second ;
      /* create integer time representation */
      timep = ossMkTime( &t ) ;
   }

   if( valType == CJSON_TIMESTAMP )
   {
      if ( !ossIsTimestampValid( timep ) )
      {
         JSON_PRINTF_LOG( "Timestamp must not greater than "
                          "2038-01-19-03.14.07.999999 +/- TZ; or less than"
                          "1901-12-13-20.45.52.000000 +/- TZ" ) ;
         goto error ;
      }
   }
   *pTimestamp = timep ;
   *pMicros = micros ;
done:
   return flag ;
error:
   flag = FALSE ;
   goto done ;
}

static const CHAR onethousand_num[1000][4] = {
    "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99",

    "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
    "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
    "120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
    "130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
    "140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
    "150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
    "160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
    "170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
    "180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
    "190", "191", "192", "193", "194", "195", "196", "197", "198", "199",

    "200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
    "210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
    "220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
    "230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
    "240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
    "250", "251", "252", "253", "254", "255", "256", "257", "258", "259",
    "260", "261", "262", "263", "264", "265", "266", "267", "268", "269",
    "270", "271", "272", "273", "274", "275", "276", "277", "278", "279",
    "280", "281", "282", "283", "284", "285", "286", "287", "288", "289",
    "290", "291", "292", "293", "294", "295", "296", "297", "298", "299",

    "300", "301", "302", "303", "304", "305", "306", "307", "308", "309",
    "310", "311", "312", "313", "314", "315", "316", "317", "318", "319",
    "320", "321", "322", "323", "324", "325", "326", "327", "328", "329",
    "330", "331", "332", "333", "334", "335", "336", "337", "338", "339",
    "340", "341", "342", "343", "344", "345", "346", "347", "348", "349",
    "350", "351", "352", "353", "354", "355", "356", "357", "358", "359",
    "360", "361", "362", "363", "364", "365", "366", "367", "368", "369",
    "370", "371", "372", "373", "374", "375", "376", "377", "378", "379",
    "380", "381", "382", "383", "384", "385", "386", "387", "388", "389",
    "390", "391", "392", "393", "394", "395", "396", "397", "398", "399",

    "400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
    "410", "411", "412", "413", "414", "415", "416", "417", "418", "419",
    "420", "421", "422", "423", "424", "425", "426", "427", "428", "429",
    "430", "431", "432", "433", "434", "435", "436", "437", "438", "439",
    "440", "441", "442", "443", "444", "445", "446", "447", "448", "449",
    "450", "451", "452", "453", "454", "455", "456", "457", "458", "459",
    "460", "461", "462", "463", "464", "465", "466", "467", "468", "469",
    "470", "471", "472", "473", "474", "475", "476", "477", "478", "479",
    "480", "481", "482", "483", "484", "485", "486", "487", "488", "489",
    "490", "491", "492", "493", "494", "495", "496", "497", "498", "499",

    "500", "501", "502", "503", "504", "505", "506", "507", "508", "509",
    "510", "511", "512", "513", "514", "515", "516", "517", "518", "519",
    "520", "521", "522", "523", "524", "525", "526", "527", "528", "529",
    "530", "531", "532", "533", "534", "535", "536", "537", "538", "539",
    "540", "541", "542", "543", "544", "545", "546", "547", "548", "549",
    "550", "551", "552", "553", "554", "555", "556", "557", "558", "559",
    "560", "561", "562", "563", "564", "565", "566", "567", "568", "569",
    "570", "571", "572", "573", "574", "575", "576", "577", "578", "579",
    "580", "581", "582", "583", "584", "585", "586", "587", "588", "589",
    "590", "591", "592", "593", "594", "595", "596", "597", "598", "599",

    "600", "601", "602", "603", "604", "605", "606", "607", "608", "609",
    "610", "611", "612", "613", "614", "615", "616", "617", "618", "619",
    "620", "621", "622", "623", "624", "625", "626", "627", "628", "629",
    "630", "631", "632", "633", "634", "635", "636", "637", "638", "639",
    "640", "641", "642", "643", "644", "645", "646", "647", "648", "649",
    "650", "651", "652", "653", "654", "655", "656", "657", "658", "659",
    "660", "661", "662", "663", "664", "665", "666", "667", "668", "669",
    "670", "671", "672", "673", "674", "675", "676", "677", "678", "679",
    "680", "681", "682", "683", "684", "685", "686", "687", "688", "689",
    "690", "691", "692", "693", "694", "695", "696", "697", "698", "699",

    "700", "701", "702", "703", "704", "705", "706", "707", "708", "709",
    "710", "711", "712", "713", "714", "715", "716", "717", "718", "719",
    "720", "721", "722", "723", "724", "725", "726", "727", "728", "729",
    "730", "731", "732", "733", "734", "735", "736", "737", "738", "739",
    "740", "741", "742", "743", "744", "745", "746", "747", "748", "749",
    "750", "751", "752", "753", "754", "755", "756", "757", "758", "759",
    "760", "761", "762", "763", "764", "765", "766", "767", "768", "769",
    "770", "771", "772", "773", "774", "775", "776", "777", "778", "779",
    "780", "781", "782", "783", "784", "785", "786", "787", "788", "789",
    "790", "791", "792", "793", "794", "795", "796", "797", "798", "799",

    "800", "801", "802", "803", "804", "805", "806", "807", "808", "809",
    "810", "811", "812", "813", "814", "815", "816", "817", "818", "819",
    "820", "821", "822", "823", "824", "825", "826", "827", "828", "829",
    "830", "831", "832", "833", "834", "835", "836", "837", "838", "839",
    "840", "841", "842", "843", "844", "845", "846", "847", "848", "849",
    "850", "851", "852", "853", "854", "855", "856", "857", "858", "859",
    "860", "861", "862", "863", "864", "865", "866", "867", "868", "869",
    "870", "871", "872", "873", "874", "875", "876", "877", "878", "879",
    "880", "881", "882", "883", "884", "885", "886", "887", "888", "889",
    "890", "891", "892", "893", "894", "895", "896", "897", "898", "899",

    "900", "901", "902", "903", "904", "905", "906", "907", "908", "909",
    "910", "911", "912", "913", "914", "915", "916", "917", "918", "919",
    "920", "921", "922", "923", "924", "925", "926", "927", "928", "929",
    "930", "931", "932", "933", "934", "935", "936", "937", "938", "939",
    "940", "941", "942", "943", "944", "945", "946", "947", "948", "949",
    "950", "951", "952", "953", "954", "955", "956", "957", "958", "959",
    "960", "961", "962", "963", "964", "965", "966", "967", "968", "969",
    "970", "971", "972", "973", "974", "975", "976", "977", "978", "979",
    "980", "981", "982", "983", "984", "985", "986", "987", "988", "989",
    "990", "991", "992", "993", "994", "995", "996", "997", "998", "999",
} ;

static void get_char_num ( CHAR *str, INT32 i, INT32 str_size )
{
   if( 1000 > i && 0 < i )
   {
      memcpy( str, onethousand_num[i], 4 );
   }
   else
   {
      memset ( str, 0, str_size ) ;
      intToString ( i, str, 10 ) ;
   }
}

static CHAR* intToString ( INT32 value, CHAR *string, INT32 radix )
{
   CHAR tmp[33] ;
   CHAR *tp = tmp ;
   INT32 i ;
   UINT32 v ;
   INT32 sign ;
   CHAR *sp ;
   if( radix > 36 || radix <= 1 )
   {
      return NULL ;
   }
   sign = (radix == 10 && value < 0 ) ;
   if ( sign )
   {
      v = -value ;
   }
   else
   {
      v = (UINT32)value ;
   }
   while ( v || tp == tmp )
   {
      i = v % radix ;
      v = v / radix ;
      if ( i < 10 )
         *tp++ = i + '0' ;
      else
         *tp++ = i + 'a' - 10 ;
   }
   sp = string ;
   if ( sign )
      *sp++ = '-' ;
   while ( tp > tmp )
      *sp++ = *--tp ;
   *sp = 0 ;
   return string ;
}

static BOOLEAN jsonConvertBson( const CJSON_MACHINE *pMachine,
                                const cJson_iterator *pIter,
                                bson *pBson,
                                BOOLEAN *hasId,
                                BOOLEAN isObj,
                                INT32 decimalto )
{
   BOOLEAN flag = TRUE ;
   INT32 i = 0 ;
   CJSON_VALUE_TYPE cJsonType = CJSON_NONE ;
   const CHAR *pKey = NULL ;
   CVALUE arg1 ;
   CVALUE arg2 ;
   CHAR numKey[ INT_NUM_SIZE ] = {0} ;

   if ( hasId )
   {
      *hasId = FALSE ;
   }

   while( cJsonIteratorMore( pIter ) )
   {
      cJsonType = cJsonIteratorType( pIter ) ;
      if( isObj == TRUE )
      {
         pKey = cJsonIteratorKey( pIter ) ;

         if ( pKey && hasId && FALSE == *hasId &&
              ossStrlen( pKey ) == sizeof( RECORD_ID_NAME ) - 1 &&
              0 == ossStrncmp( pKey, RECORD_ID_NAME,
                               sizeof( RECORD_ID_NAME ) - 1 ) )
         {
            *hasId = TRUE ;
         }
      }
      if( isObj == FALSE || pKey == NULL )
      {
         ossMemset( numKey, 0, INT_NUM_SIZE ) ;
         get_char_num( numKey, i, INT_NUM_SIZE ) ;
         pKey = &( numKey[0] ) ;
      }
      switch( cJsonType )
      {
      case CJSON_FALSE:
      {
         /* for boolean */
         if( bson_append_bool( pBson, pKey, FALSE ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' bool", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_TRUE:
      {
         /* for boolean */
         if( bson_append_bool( pBson, pKey, TRUE ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' bool", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_NULL:
      {
         /* for null type */
         if( bson_append_null( pBson, pKey ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' null", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_INT32:
      {
         /* for 32 bit int */
         INT32 number = cJsonIteratorInt32( pIter ) ;
         if( bson_append_int( pBson, pKey, number ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' int", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_INT64:
      {
         /* for 64 bit int */
         INT64 number = cJsonIteratorInt64( pIter ) ;
         if( bson_append_long( pBson, pKey, number ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' int64", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_DOUBLE:
      {
         /* for 64 bit float */
         FLOAT64 number = cJsonIteratorDouble( pIter ) ;
         if( bson_append_double( pBson, pKey, number ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' double", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_STRING:
      {
         /* string type */
         const CHAR *pString = cJsonIteratorString( pIter ) ;
         if( bson_append_string( pBson, pKey, pString ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' string", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_ARRAY:
      {
         const cJson_iterator *pIterSub = NULL ;
         /*
            for object type, we call jsonConvertBson recursively, and provide
         */
         if( bson_append_start_array( pBson, pKey ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to start append bson '%s' array", pKey ) ;
            goto error ;
         }
         pIterSub = cJsonIteratorSub( pIter ) ;
         if( pIterSub == NULL )
         {
            JSON_PRINTF_LOG( "Failed to get '%s' sub iterator", pKey ) ;
            goto error ;
         }
         if( jsonConvertBson( pMachine, pIterSub, pBson, NULL,
                              TRUE, decimalto ) == FALSE )
         {
            JSON_PRINTF_LOG( "Failed to convert '%s' value", pKey ) ;
            goto error ;
         }
         if( bson_append_finish_array( pBson ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to end append bson '%s' array", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_OBJECT:
      {
         const cJson_iterator *pIterSub = NULL ;
         /*
            for object type, we call jsonConvertBson recursively, and provide
         */
         if( bson_append_start_object( pBson, pKey ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to start append bson '%s' object", pKey ) ;
            goto error ;
         }
         pIterSub = cJsonIteratorSub( pIter ) ;
         if( pIterSub == NULL )
         {
            JSON_PRINTF_LOG( "Failed to get '%s' sub iterator", pKey ) ;
            goto error ;
         }
         if( jsonConvertBson( pMachine, pIterSub, pBson, NULL,
                              TRUE, decimalto ) == FALSE )
         {
            JSON_PRINTF_LOG( "Failed to convert '%s' value", pKey ) ;
            goto error ;
         }
         if( bson_append_finish_object( pBson ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to end append bson '%s' object", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_TIMESTAMP:
      {
         INT32 micros = 0 ;
         time_t timestamp = 0 ;

         cJsonIteratorTimestamp( pIter, &arg1 ) ;
         if( arg1.valType != CJSON_INT32 &&
             arg1.valType != CJSON_STRING )
         {
            JSON_PRINTF_LOG( "The '%s' value timestamp must "
                             "be an 32-bit integet or string", pKey ) ;
            goto error ;
         }
         if( arg1.valType == CJSON_INT32 )
         {
            timestamp = (time_t)arg1.valInt ;
            micros = 0 ;
         }
         else if( arg1.valType == CJSON_STRING )
         {
            if( date2Time( arg1.pValStr,
                           CJSON_TIMESTAMP,
                           &timestamp,
                           &micros ) == FALSE )
            {
               JSON_PRINTF_LOG( "Failed to convert '%s' timestamp, "
                                "timestamp format is "
                                "YYYY-MM-DD-HH.mm.ss.ffffff", pKey ) ;
               goto error ;
            }
         }
         if( bson_append_timestamp2( pBson,
                                     pKey,
                                     (INT32)timestamp,
                                     micros ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' timestamp", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_DATE:
      {
         bson_date_t dateTime = 0 ;
         cJsonIteratorDate( pIter, &arg1 ) ;
         if( arg1.valType != CJSON_INT32 &&
             arg1.valType != CJSON_INT64 &&
             arg1.valType != CJSON_STRING )
         {
            JSON_PRINTF_LOG( "The '%s' value date must "
                             "be an integet or string", pKey ) ;
            goto error ;
         }
         if( arg1.valType == CJSON_INT32 )
         {
            dateTime = (bson_date_t)arg1.valInt ;
         }
         else if( arg1.valType == CJSON_INT64 )
         {
            dateTime = (bson_date_t)arg1.valInt64 ;
         }
         else if( arg1.valType == CJSON_STRING )
         {
            INT32 valInt = 0 ;
            FLOAT64 valDouble = 0 ;
            INT64 valInt64 = 0 ;
            CJSON_VALUE_TYPE type = CJSON_NONE ;
            if( cJsonParseNumber( arg1.pValStr,
                                  arg1.length,
                                  &valInt,
                                  &valDouble,
                                  &valInt64,
                                  &type ) == TRUE )
            {

               if( type == CJSON_INT32 )
               {
                  dateTime = (bson_date_t)valInt ;
               }
               else if( type == CJSON_INT64 )
               {
                  dateTime = (bson_date_t)valInt64 ;
               }
               else
               {
                  JSON_PRINTF_LOG( "Failed to read date, the '%.*s' "
                                   "is out of the range of date time",
                                   arg1.length,
                                   arg1.pValStr ) ;
                  goto error ;
               }
            }
            else
            {
               INT32 micros = 0 ;
               time_t timestamp = 0 ;
               if( date2Time( arg1.pValStr,
                              CJSON_DATE,
                              &timestamp,
                              &micros ) == FALSE )
               {
                  JSON_PRINTF_LOG( "Failed to convert '%s' date, "
                                   "date format is YYYY-MM-DD", pKey ) ;
                  goto error ;
               }
               dateTime = (bson_date_t)timestamp * 1000 ;
               dateTime += (bson_date_t)( micros / 1000 ) ;
            }
         }
         if( bson_append_date( pBson, pKey, dateTime ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' date", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_REGEX:
      {
         const CHAR *pOptions = "" ;
         cJsonIteratorRegex( pIter, &arg1, &arg2 ) ;
         if( arg1.valType != CJSON_STRING )
         {
            JSON_PRINTF_LOG( "The '%s' value regex must "
                             "be a string", pKey ) ;
            goto error ;
         }
         if( arg2.valType != CJSON_STRING && arg2.valType != CJSON_NONE  )
         {
            JSON_PRINTF_LOG( "The '%s' value options must "
                             "be a string", pKey ) ;
            goto error ;
         }
         else if( arg2.valType == CJSON_STRING )
         {
            pOptions = arg2.pValStr ;
         }
         if( bson_append_regex( pBson,
                                pKey,
                                arg1.pValStr,
                                pOptions ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' regex", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_OID:
      {
         CHAR c = 0 ;
         INT32 i = 0 ;
         INT32 oidStart = 0 ;
         bson_oid_t bot ;
         CHAR pOid[25] ;

         ossMemset( pOid, '0', 24 ) ;
         pOid[24] = 0 ;
         cJsonIteratorObjectId( pIter, &arg1 ) ;
         if( arg1.valType != CJSON_STRING )
         {
            JSON_PRINTF_LOG( "The '%s' value objectId must "
                             "be a string", pKey ) ;
            goto error ;
         }
         else if( arg1.length < 0 || arg1.length > 24 )
         {
            CJSON_PRINTF_LOG( "The '%s' value objectId must be a string "
                              "of length 24", pKey ) ;
            goto error ;
         }
         oidStart = 24 - arg1.length ;
         for( i = 0; i < arg1.length; ++i )
         {
            c = *( arg1.pValStr + i ) ;
            if( ( c < '0' || c > '9' ) &&
                ( c < 'a' || c > 'f' ) &&
                ( c < 'A' || c > 'F' ) )
            {
               CJSON_PRINTF_LOG( "The '%s' value objectId "
                                 "must be a hex string", pKey ) ;
               goto error ;
            }
            pOid[oidStart + i] = c ;
         }
         bson_oid_from_string( &bot, pOid ) ;
         if( bson_append_oid( pBson, pKey, &bot ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' ObjectId", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_BINARY:
      {
         /* for binary type, user input base64 encoded string, which should be
          * 4 bytes aligned, and then call base64_decode to extract into binary
          * and store in BSON object */
         CHAR type = 0 ;
         INT32 base64DecodeLen = 0 ;
         CHAR *pBase64 = NULL ;

         cJsonIteratorBinary( pIter, &arg1, &arg2 ) ;
         if( arg1.valType != CJSON_STRING )
         {
            JSON_PRINTF_LOG( "The '%s' value binary must "
                             "be a string", pKey ) ;
            goto error ;
         }
         if( arg2.valType != CJSON_STRING && arg2.valType != CJSON_INT32  )
         {
            JSON_PRINTF_LOG( "The '%s' value type must "
                             "be a string or integer", pKey ) ;
            goto error ;
         }
         if( arg2.valType == CJSON_STRING )
         {
            INT32 numType = ossAtoi( arg2.pValStr ) ;
            if( numType < 0 || numType > 255 )
            {
               JSON_PRINTF_LOG( "The '%s' value binary type must "
                                "be an integer 0-255", pKey ) ;
               goto error ;
            }
            type = (CHAR)numType ;
         }
         else if( arg2.valType == CJSON_INT32 )
         {
            if( arg2.valInt < 0 || arg2.valInt > 255 )
            {
               JSON_PRINTF_LOG( "The '%s' value binary type must "
                                "be an integer 0-255", pKey ) ;
               goto error ;
            }
            type = (CHAR)arg2.valInt ;
         }
         /* first we calculate the expected size after extraction */
         if( arg1.length == 0 )
         {
            pBase64 = arg1.pValStr ;
            base64DecodeLen = arg1.length ;
         }
         else
         {
            base64DecodeLen = getDeBase64Size( arg1.pValStr ) ;
            if( base64DecodeLen < 0 )
            {
               JSON_PRINTF_LOG( "The '%s' value binary format error", pKey ) ;
               goto error ;
            }
            /* and allocate memory */
            pBase64 = (CHAR *)cJsonMalloc( base64DecodeLen, pMachine ) ;
            if( pBase64 == NULL )
            {
               JSON_PRINTF_LOG( "Failed to malloc base64 memory" ) ;
               goto error ;
            }
            ossMemset( pBase64, 0, base64DecodeLen ) ;
            /* and then decode into the buffer we just allocated */
            if( base64Decode( arg1.pValStr, pBase64, base64DecodeLen ) < 0 )
            {
               cJsonFree( pBase64, pMachine ) ;
               JSON_PRINTF_LOG( "Failed to decode '%s' value binary base64", pKey ) ;
               goto error ;
            }
            base64DecodeLen = base64DecodeLen - 1 ;
         }
         /* and then append into bson */
         if( bson_append_binary( pBson,
                                 pKey,
                                 type,
                                 pBase64,
                                 base64DecodeLen ) == BSON_ERROR )
         {
            cJsonFree( pBase64, pMachine ) ;
            JSON_PRINTF_LOG( "Failed to append bson '%s' binary", pKey ) ;
            goto error ;
         }
         if( arg1.length > 0 )
         {
            cJsonFree( pBase64, pMachine ) ;
         }
         break ;
      }
      case CJSON_MINKEY:
      {
         /* minkey type */
         if( bson_append_minkey( pBson, pKey ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' minKey", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_MAXKEY:
      {
         /* maxkey type */
         if( bson_append_maxkey( pBson, pKey ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' maxKey", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_UNDEFINED:
      {
         /* undefined type */
         if( bson_append_undefined( pBson, pKey ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' undefined", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_NUMBER_LONG:
      {
         /* for 64 bit int */
         INT64 number = 0 ;
         cJsonIteratorNumberLong( pIter, &arg1 ) ;
         if( arg1.valType == CJSON_INT32 )
         {
            number = (INT64)arg1.valInt ;
         }
         else if( arg1.valType == CJSON_INT64 )
         {
            number = arg1.valInt64 ;
         }
         else if( arg1.valType == CJSON_STRING )
         {
            INT32 valInt = 0 ;
            FLOAT64 valDouble = 0 ;
            INT64 valInt64 = 0 ;
            CJSON_VALUE_TYPE type = CJSON_NONE ;

            if( cJsonParseNumber( arg1.pValStr,
                                  arg1.length,
                                  &valInt,
                                  &valDouble,
                                  &valInt64,
                                  &type ) == FALSE )
            {
               JSON_PRINTF_LOG( "The numberLong '%.*s' is an invalid number ",
                                arg1.length,
                                arg1.pValStr ) ;
               goto error ;
            }
            if( type == CJSON_INT32 )
            {
               number = valInt ;
            }
            else if( type == CJSON_INT64 )
            {
               number = valInt64 ;
            }
            else if( type == CJSON_DECIMAL )
            {
               JSON_PRINTF_LOG( "Failed to read numberLong, the '%.*s' "
                                "is out of the range of numberLong",
                                arg1.length,
                                arg1.pValStr ) ;
               goto error ;
            }
            else
            {
               JSON_PRINTF_LOG( "Failed to read numberLong, the '%.*s' "
                                "must be integer type or string type",
                                arg1.length,
                                arg1.pValStr ) ;
               goto error ;
            }
         }
         else
         {
            JSON_PRINTF_LOG( "The '%s' value numberLong must "
                             "be a string or integer", pKey ) ;
            goto error ;
         }
         if( bson_append_long( pBson, pKey, number ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' int64", pKey ) ;
            goto error ;
         }
         break ;
      }
      case CJSON_DECIMAL:
      {
         bson_decimal bsonDecimal = SDB_DECIMAL_DEFAULT_VALUE ;
         cJsonIteratorDecimal( pIter, &arg1, &arg2 ) ;
         if( arg1.valType != CJSON_INT32 &&
             arg1.valType != CJSON_INT64 &&
             arg1.valType != CJSON_DOUBLE &&
             arg1.valType != CJSON_STRING )
         {
            JSON_PRINTF_LOG( "The '%s' value decimal must "
                             "be a number or string", pKey ) ;
            goto error ;
         }
         if( arg2.valType != CJSON_ARRAY && arg2.valType != CJSON_NONE  )
         {
            JSON_PRINTF_LOG( "The '%s' value precision must "
                             "be an array", pKey ) ;
            goto error ;
         }
         if( arg2.valType == CJSON_ARRAY )
         {
            CVALUE precisionVal ;
            CVALUE scaleVal ;
            if( cJsonIteratorSubNum2( &arg2 ) != 2 )
            {
               JSON_PRINTF_LOG( "The '%s' value precision must "
                                "be an array of two integet element", pKey ) ;
               goto error ;
            }
            cJsonIteratorPrecision( pIter, &precisionVal, &scaleVal ) ;
            if( precisionVal.valType != CJSON_INT32 )
            {
               JSON_PRINTF_LOG( "The '%s' value precision must "
                                "be an array of two integet element", pKey ) ;
               goto error ;
            }
            if( scaleVal.valType != CJSON_INT32 )
            {
               JSON_PRINTF_LOG( "The '%s' value precision must "
                                "be an array of two integet element", pKey ) ;
               goto error ;
            }
            if( sdb_decimal_init1( &bsonDecimal,
                                   precisionVal.valInt,
                                   scaleVal.valInt ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to init decimal, key: %s", pKey ) ;
               goto error ;
            }
         }

         if( arg1.valType == CJSON_INT32 )
         {
            if( sdb_decimal_from_int( arg1.valInt, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal int, key: %s", pKey ) ;
               goto error ;
            }
         }
         else if( arg1.valType == CJSON_INT64 )
         {
            if( sdb_decimal_from_long( arg1.valInt64, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal int64, key: %s",
                                pKey ) ;
               goto error ;
            }
         }
         else if( arg1.valType == CJSON_DOUBLE )
         {
            if( sdb_decimal_from_double( arg1.valDouble, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal double, key: %s",
                                pKey ) ;
               goto error ;
            }
         }
         else if( arg1.valType == CJSON_STRING )
         {
            if( sdb_decimal_from_str( arg1.pValStr, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal string, key: %s",
                                pKey ) ;
               goto error ;
            }
         }

         if( JSON_DECIMAL_NO_CONVERT == decimalto )
         {
            //to decimal
            if( bson_append_decimal( pBson, pKey, &bsonDecimal ) == BSON_ERROR )
            {
               JSON_PRINTF_LOG( "Failed to append bson '%s' decimal", pKey ) ;
               sdb_decimal_free( &bsonDecimal ) ;
               goto error ;
            }
         }
         else if( JSON_DECIMAL_TO_DOUBLE == decimalto )
         {
            //to double
            FLOAT64 number = sdb_decimal_to_double( &bsonDecimal ) ;

            if( bson_append_double( pBson, pKey, number ) == BSON_ERROR )
            {
               JSON_PRINTF_LOG( "Failed to append bson '%s' double", pKey ) ;
               goto error ;
            }
         }
         else if( JSON_DECIMAL_TO_STRING == decimalto )
         {
            //to string
            INT32 strLen = 0 ;
            CHAR *pString = NULL ;

            sdb_decimal_to_str_get_len( &bsonDecimal, &strLen ) ;

            pString = (CHAR *)cJsonMalloc( strLen + 1, pMachine ) ;
            if( pString == NULL )
            {
               JSON_PRINTF_LOG( "Failed to malloc base64 memory" ) ;
               goto error ;
            }

            ossMemset( pString, 0, strLen + 1 ) ;

            if( 0 != sdb_decimal_to_str( &bsonDecimal, pString, strLen ) )
            {
               cJsonFree( pString, pMachine ) ;
               JSON_PRINTF_LOG( "Failed to convert '%s' to string", pKey ) ;
               goto error ;
            }

            if( bson_append_string( pBson, pKey, pString ) == BSON_ERROR )
            {
               cJsonFree( pString, pMachine ) ;
               JSON_PRINTF_LOG( "Failed to append bson '%s' string", pKey ) ;
               goto error ;
            }

            cJsonFree( pString, pMachine ) ;
         }

         sdb_decimal_free( &bsonDecimal ) ;
         break ;
      }
      case CJSON_NONE:
      {
         break ;
      }
      case CJSON_NUMBER:
      case CJSON_OPTIONS:
      case CJSON_TYPE:
      case CJSON_PRECISION:
      default:
         JSON_PRINTF_LOG( "Internal type can not be used", pKey ) ;
         goto error ;
      }
      ++i ;
      cJsonIteratorNext( pIter ) ;
   }

done:
   return flag ;
error:
   flag = FALSE ;
   goto done ;
}


/*
 * copy data to pbuf
 * pbuf : output variable
 * left : the remaining size
 * data : input variable
 * return : void
*/
static void bsonConvertJsonRawConcat( CHAR **pbuf,
                                      INT32 *left,
                                      const CHAR *data,
                                      BOOLEAN isString )
{
   UINT32 tempsize = 0 ;
   CHAR *pTempBuf = *pbuf ;
   if ( isString )
      tempsize = strlen_a ( data ) ;
   else
      tempsize = strlen ( data ) ;
   tempsize = tempsize > ( UINT32 )(*left) ? ( UINT32 )(*left) : tempsize ;
   if ( isString )
   {
      UINT32 i = 0 ;
      for ( i = 0; i < tempsize; ++i )
      {
         switch ( *data )
         {
         //the JSON standard does not need to be escaped single quotation marks
         /*case '\'':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = '\'' ;
           break ;
         }*/
         case '\"':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = '\"' ;
           break ;
         }
         case '\\':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = '\\' ;
           break ;
         }
         case '\b':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = 'b' ;
           break ;
         }
         case '\f':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = 'f' ;
           break ;
         }
         case '\n':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = 'n' ;
           break ;
         }
         case '\r':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = 'r' ;
           break ;
         }
         case '\t':
         {
           pTempBuf[i] = '\\' ;
           ++i ;
           pTempBuf[i] = 't' ;
           break ;
         }
         default :
         {
            pTempBuf[i] = *data ;
            break ;
         }
         }
         ++data ;
      }
   }
   else
   {
      memcpy ( *pbuf, data, tempsize ) ;
   }
   *left -= tempsize ;
   *pbuf += tempsize ;
}

/*
 * Bson convert Json
 * pbuf : output variable
 * left : the pbuf size
 * data : input bson's data variable
 * isobj : determine the current status
 * return : the conversion result
*/
static BOOLEAN bsonConvertJson ( CHAR **pbuf,
                                 INT32 *left,
                                 const CHAR *data ,
                                 INT32 isobj,
                                 BOOLEAN toCSV,
                                 BOOLEAN skipUndefined,
                                 BOOLEAN isStrict )
{
   bson_iterator i ;
   const CHAR *key ;
   bson_timestamp_t ts ;
   CHAR oidhex [ 25 ] ;
   struct tm psr ;
   INT32 first = 1 ;
   if ( *left <= 0 || !pbuf || !data )
      return FALSE ;
   bson_iterator_from_buffer( &i, data ) ;
   if ( !toCSV )
   {
      if ( isobj )
      {
         bsonConvertJsonRawConcat ( pbuf, left, "{ ", FALSE ) ;
         CHECK_LEFT ( left )
      }
      else
      {
         bsonConvertJsonRawConcat ( pbuf, left, "[ ", FALSE ) ;
         CHECK_LEFT ( left )
      }
   }
   while ( bson_iterator_next( &i ) )
   {
      bson_type t = bson_iterator_type( &i ) ;
      /* if BSON_EOO == t ( which is 0 ), that means we hit end of object */
      if ( !t )
      {
         break ;
      }
      if ( skipUndefined && BSON_UNDEFINED == t )
      {
         continue ;
      }
      /* do NOT concat "," for first entrance */
      if ( !first )
      {
         bsonConvertJsonRawConcat ( pbuf, left, toCSV?"|":", ", FALSE ) ;
         CHECK_LEFT ( left )
      }
      else
         first = 0 ;
      /* get key string */
      key = bson_iterator_key( &i ) ;
      /* for object, we always display { " key" : "value" }, so we have to
       * display double quotes and key and comma */
      if ( isobj && !toCSV )
      {
         bsonConvertJsonRawConcat ( pbuf, left, "\"", FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, key, TRUE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, "\"", FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, ": ", FALSE ) ;
         CHECK_LEFT ( left )
      }
      /* then we check the data type */
      switch ( t )
      {
      case BSON_DOUBLE:
      {
         /* for double type, we use 64 bytes string for such big value */
         INT32 sign = 0 ;
         FLOAT64 valNum = bson_iterator_double( &i ) ;
         if( bson_is_inf( valNum, &sign ) == FALSE )
         {
            CHAR temp[ BSON_TEMP_SIZE_512 ] ;
            FLOAT64 z = 0.0;
            memset ( temp, 0, BSON_TEMP_SIZE_512 ) ;
            z = valNum;
            if ( valNum == z )
            {
#ifdef WIN32
               _snprintf ( temp,
                           BSON_TEMP_SIZE_512,
                           _precision, bson_iterator_double( &i ) ) ;
#else
               snprintf ( temp,
                          BSON_TEMP_SIZE_512,
                          _precision, bson_iterator_double( &i ) ) ;
#endif
               bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
               CHECK_LEFT ( left )

               if( strchr( temp, '.') == 0 && strchr( temp, 'E') == 0
                   && strchr( temp, 'N') == 0 && strchr( temp, 'e') == 0
                   && strchr( temp, 'n') == 0 )
               {
                  bsonConvertJsonRawConcat ( pbuf, left, ".0", FALSE ) ;
                  CHECK_LEFT ( left )
               }
            }
            else
            {
               (void) ossStrncpy ( temp, "NaN", BSON_TEMP_SIZE_512) ;
               bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
               CHECK_LEFT ( left )
            }
         }
         else
         {
            if( sign == 1 )
            {
               bsonConvertJsonRawConcat( pbuf, left, "Infinity", FALSE ) ;
            }
            else
            {
               bsonConvertJsonRawConcat( pbuf, left, "-Infinity", FALSE ) ;
            }
            CHECK_LEFT ( left )
         }
         break;
      }
      case BSON_STRING:
      case BSON_SYMBOL:
      {
         /* for string type, we output double quote and string data */
         const CHAR *temp = bson_iterator_string( &i ) ;
         if ( !toCSV )
         {
            bsonConvertJsonRawConcat ( pbuf, left, "\"", FALSE ) ;
            CHECK_LEFT ( left )
         }
         bsonConvertJsonRawConcat ( pbuf, left, temp, TRUE ) ;
         CHECK_LEFT ( left )
         if ( !toCSV )
         {
            bsonConvertJsonRawConcat ( pbuf, left, "\"", FALSE ) ;
            CHECK_LEFT ( left )
         }
         break ;
      }
      case BSON_OID:
      {
         /* for oid type, we always display { $oid : "<12 bytes string>" }. So
          * we have to display first part, then concat oidhex, then the last
          * part */
         bson_oid_to_string( bson_iterator_oid( &i ), oidhex );
         if ( !toCSV )
         {
            bsonConvertJsonRawConcat ( pbuf, left, "{ \"$oid\": \"", FALSE ) ;
            CHECK_LEFT ( left )
         }
         bsonConvertJsonRawConcat ( pbuf, left, oidhex, FALSE ) ;
         CHECK_LEFT ( left )
         if ( !toCSV )
         {
            bsonConvertJsonRawConcat ( pbuf, left, "\" }", FALSE ) ;
            CHECK_LEFT ( left )
         }
         break ;
      }
      case BSON_BOOL:
      {
         /* for boolean type, we display either true or false */
         bsonConvertJsonRawConcat ( pbuf,
                                    left,
                                    (bson_iterator_bool( &i )?"true":"false"),
                                    FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_DATE:
      {
         /* for date type, DATE_OUTPUT_FORMAT is the format we need to use, and
          * use snprintf to display */
         CHAR temp[ BSON_TEMP_SIZE_64 ] ;
         struct tm psr;
         time_t timer = bson_iterator_date( &i ) / 1000 ;
         memset ( temp, 0, BSON_TEMP_SIZE_64 ) ;
         local_time ( &timer, &psr ) ;
         if( psr.tm_year + RELATIVE_YEAR >= INT64_FIRST_YEAR &&
             psr.tm_year + RELATIVE_YEAR <= INT64_LAST_YEAR )
         {
#ifdef WIN32
            _snprintf ( temp,
                        BSON_TEMP_SIZE_64,
                        toCSV?DATE_OUTPUT_CSV_FORMAT:DATE_OUTPUT_FORMAT,
                        psr.tm_year + RELATIVE_YEAR,
                        psr.tm_mon + 1,
                        psr.tm_mday ) ;
#else
            snprintf ( temp,
                       BSON_TEMP_SIZE_64,
                       toCSV?DATE_OUTPUT_CSV_FORMAT:DATE_OUTPUT_FORMAT,
                       psr.tm_year + RELATIVE_YEAR,
                       psr.tm_mon + 1,
                       psr.tm_mday ) ;
#endif
            bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
            CHECK_LEFT ( left )
         }
         else
         {
            CHAR temp[ BSON_TEMP_SIZE_512 ] ;
            memset ( temp, 0, BSON_TEMP_SIZE_512 ) ;
#ifdef WIN32
            _snprintf ( temp,
                        BSON_TEMP_SIZE_512,
                        "%lld",
                        (UINT64)bson_iterator_date( &i ) ) ;
#else
            snprintf ( temp,
                       BSON_TEMP_SIZE_512,
                       "%lld",
                       (UINT64)bson_iterator_date( &i ) ) ;
#endif
            bsonConvertJsonRawConcat ( pbuf, left, "{ \"$date\": ", FALSE ) ;
            CHECK_LEFT ( left )
            bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
            CHECK_LEFT ( left )
            bsonConvertJsonRawConcat ( pbuf, left, " }", FALSE ) ;
            CHECK_LEFT ( left )
         }
         break ;
      }
      case BSON_BINDATA:
      {
         CHAR bin_type ;
         CHAR *bin_data ;
         INT32 bin_size ;
         INT32 len = 0 ;
         CHAR *temp = NULL ;
         CHAR *out = NULL ;
         /* TODO: We have to remove malloc here later */
         /* for BINDATA type, user need to input base64 encoded string, which is
          * supposed to be 4 bytes aligned, and we use base64_decode to extract
          * and store in database. For display, we use base64_encode to encode
          * the data stored in database, and format it into base64 encoded
          * string */
         if ( toCSV )
         {
            // we don't support BIN DATA in csv output
            break ;
         }
         bin_type = bson_iterator_bin_type( &i ) ;
         bin_data = (CHAR *)bson_iterator_bin_data( &i ) ;
         bin_size = bson_iterator_bin_len ( &i ) ;
         if( bin_size > 0 )
         {
            /* first we need to calculate how much space we need to put the new
             * data */
            len = getEnBase64Size ( bin_size ) ;
            /* and then we allocate memory for the display string, which includes
             * { $binary : xxxxx, $type : xxx }, so we have to put another 40
             * bytes */
            temp = (CHAR *)malloc( len + 48 ) ;
            if ( !temp )
            {
               return FALSE ;
            }
            memset ( temp, 0, len + 48 ) ;
            /* then we have to allocate another piece of memory for base64 encoding
             */
            out = (CHAR *)malloc( len + 1 ) ;
            if ( !out )
            {
               free( temp ) ;
               return FALSE ;
            }
            memset ( out, 0, len ) ;
            /* encode bin_data to out, with size len */
            if ( base64Encode( bin_data, bin_size, out, len ) < 0 )
            {
               free ( temp ) ;
               free ( out ) ;
               return FALSE ;
            }
   #ifdef WIN32
            _snprintf ( temp,
                        len + 48,
                        "{ \"$binary\": \"%s\", \"$type\" : \"%d\" }",
                        out, (UINT8)bin_type ) ;
   #else
            snprintf ( temp,
                       len + 48,
                       "{ \"$binary\": \"%s\", \"$type\" : \"%d\" }",
                       out, (UINT8)bin_type ) ;
   #endif
            bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
            free( temp ) ;
            free( out ) ;
            CHECK_LEFT ( left )
         }
         else
         {
            temp = (CHAR *)malloc( 48 ) ;
            if ( !temp )
            {
               return FALSE ;
            }
            memset ( temp, 0, 48 ) ;
   #ifdef WIN32
            _snprintf ( temp,
                        48,
                        "{ \"$binary\": \"\", \"$type\" : \"%d\" }",
                        (UINT8)bin_type ) ;
   #else
            snprintf ( temp,
                       48,
                       "{ \"$binary\": \"\", \"$type\" : \"%d\" }",
                       (UINT8)bin_type ) ;
   #endif
            bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
            free( temp ) ;
            CHECK_LEFT ( left )
         }
         break ;
      }
      case BSON_UNDEFINED:
      {
         const CHAR *temp = "{ \"$undefined\": 1 }" ;
         /* we don't know how to deal with undefined value at the moment, let's
          * just output it as UNDEFINED, we may change it later */
         if ( toCSV )
         {
            break ;
         }
         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_NULL:
      {
         const CHAR *temp = "null" ;
         /* display "null" for null type */
         if ( toCSV )
         {
            break ;
         }
         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_MINKEY:
      {
         const CHAR *temp = "{ \"$minKey\": 1 }" ;
         /* display "null" for null type */
         if ( toCSV )
         {
            break ;
         }
         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_MAXKEY:
      {
         const CHAR *temp = "{ \"$maxKey\": 1 }" ;
         /* display "null" for null type */
         if ( toCSV )
         {
            break ;
         }
         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_REGEX:
      {
         /* for regular expression type, we need to display both regex and
          * options. In raw data format we have 1 byte type, 4 byte length,
          * which includes both pattern and options, and then pattern string and
          * options string. */
         if ( toCSV )
         {
            // we don't support CSV for regex
            break ;
         }
         bsonConvertJsonRawConcat ( pbuf, left, "{ \"$regex\": \"", FALSE ) ;
         CHECK_LEFT ( left )
         /* get pattern string */
         bsonConvertJsonRawConcat ( pbuf, left, bson_iterator_regex ( &i ), TRUE ) ;
         CHECK_LEFT ( left )
         /* bson_iterator_regex_opts get options by "p+strlen(p)+1", which means
          * we don't need to move iterator to next element. So we use
          * bson_iterator_regex_opts directly on &i */
         bsonConvertJsonRawConcat ( pbuf, left, "\", \"$options\": \"", FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, bson_iterator_regex_opts ( &i ), FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, "\" }", FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_CODE:
      {
         /* we don't know how to deal with code at the moment, let's just
          * display it as normal string */
         if ( toCSV )
         {
            break ;
         }
         bsonConvertJsonRawConcat ( pbuf, left, "{ \"$code\": \"", FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, bson_iterator_code( &i ), TRUE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, "\" }", FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_INT:
      {
         /* format integer. Instead of using snprintf, we call get_char_num(),
          * which uses static string to improve performance ( when value < 1000
          * ) */
         CHAR temp[ BSON_TEMP_SIZE_32 ] = {0} ;
         get_char_num ( temp, bson_iterator_int( &i ), BSON_TEMP_SIZE_32 ) ;
         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_LONG:
      {
         /* for 64 bit integer, most likely it's more than 1000, so we always
          * snprintf */
         CHAR temp[ BSON_TEMP_SIZE_512 ] ;
         CHAR *format ;
         int64_t val = bson_iterator_long( &i ) ;
         memset ( temp, 0, BSON_TEMP_SIZE_512 ) ;
         if ( isStrict == TRUE ||
              ( val < LONG_JS_MIN || val > LONG_JS_MAX ) )
         {
            format = "{ \"$numberLong\": \"%lld\" }" ;
         }
         else
         {
            format = "%lld" ;
         }

#ifdef WIN32
         _snprintf ( temp,
                     BSON_TEMP_SIZE_512,
                     format, val ) ;
#else
         snprintf ( temp,
                    BSON_TEMP_SIZE_512,
                     format, val ) ;
#endif

         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_DECIMAL:
      {
         bson_decimal decimal = SDB_DECIMAL_DEFAULT_VALUE ;
         int rc        = 0 ;
         CHAR *value   = NULL ;
         int size      = 0 ;

         // get decimal
         bson_iterator_decimal( &i, &decimal ) ;

         sdb_decimal_to_jsonstr_len( decimal.sign, decimal.weight,
                                     decimal.dscale,
                                     decimal.typemod, &size ) ;
         value = malloc( size ) ;
         if ( NULL == value )
         {
            sdb_decimal_free( &decimal ) ;
            return FALSE ;
         }

         rc = sdb_decimal_to_jsonstr( &decimal, value, size ) ;
         if ( 0 != rc )
         {
            free( value ) ;
            sdb_decimal_free( &decimal ) ;
            return FALSE ;
         }

         bsonConvertJsonRawConcat ( pbuf, left, value, FALSE ) ;
         sdb_decimal_free( &decimal ) ;
         free( value ) ;
         CHECK_LEFT ( left ) ;
         break ;
      }
      case BSON_TIMESTAMP:
      {
         /* for timestamp, it's yyyy-mm-dd-hh.mm.ss.uuuuuu */
         CHAR temp[ BSON_TEMP_SIZE_64 ] = {0} ;
         time_t timer ;
         memset ( temp, 0, BSON_TEMP_SIZE_64 ) ;
         ts = bson_iterator_timestamp( &i ) ;
         timer = (time_t)( ts.t ) ;
         local_time ( &timer, &psr ) ;
#ifdef WIN32
         _snprintf ( temp,
                     BSON_TEMP_SIZE_64,
                     toCSV?TIME_OUTPUT_CSV_FORMAT:TIME_OUTPUT_FORMAT,
                     psr.tm_year + RELATIVE_YEAR,
                     psr.tm_mon + 1,
                     psr.tm_mday,
                     psr.tm_hour,
                     psr.tm_min,
                     psr.tm_sec,
                     ts.i ) ;
#else
         snprintf ( temp,
                    BSON_TEMP_SIZE_64,
                    toCSV?TIME_OUTPUT_CSV_FORMAT:TIME_OUTPUT_FORMAT,
                    psr.tm_year + RELATIVE_YEAR,
                    psr.tm_mon + 1,
                    psr.tm_mday,
                    psr.tm_hour,
                    psr.tm_min,
                    psr.tm_sec,
                    ts.i ) ;
#endif
         bsonConvertJsonRawConcat ( pbuf, left, temp, FALSE ) ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_OBJECT:
      {
         /* for object type, we do recursive call with TRUE */
         if ( toCSV )
         {
            break ;
         }
         if ( !bsonConvertJson( pbuf, left, bson_iterator_value( &i ) ,
                                1, toCSV, skipUndefined, isStrict ) )
            return  FALSE ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_ARRAY:
      {
         /* for array type, we do recursive call with FALSE */
         if ( toCSV )
         {
            break ;
         }
         if ( !bsonConvertJson( pbuf, left, bson_iterator_value( &i ),
                                0, toCSV, skipUndefined, isStrict ) )
            return FALSE ;
         CHECK_LEFT ( left )
         break ;
      }
      case BSON_DBREF:
      {
         bson_oid_to_string( bson_iterator_dbref_oid( &i ), oidhex ) ;

         bsonConvertJsonRawConcat ( pbuf, left, "{ \"$db\" : \"", FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, bson_iterator_dbref( &i ),
                                    TRUE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, "\", \"$id\" : \"", FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, oidhex, FALSE ) ;
         CHECK_LEFT ( left )
         bsonConvertJsonRawConcat ( pbuf, left, "\" }", 0 ) ;
         CHECK_LEFT ( left )

         break ;
      }
      default:
         return FALSE ;
      }
   }
   if ( !toCSV )
   {
      if ( isobj )
      {
         bsonConvertJsonRawConcat ( pbuf, left, " }", FALSE ) ;
         CHECK_LEFT ( left )
      }
      else
      {
         bsonConvertJsonRawConcat ( pbuf, left, " ]", FALSE ) ;
         CHECK_LEFT ( left )
      }
   }
   return TRUE ;
}

static INT32 strlen_a( const CHAR *data )
{
   INT32 len = 0 ;
   if ( !data )
   {
      return 0 ;
   }
   while ( data && *data )
   {
      //the JSON standard does not need to be escaped single quotation marks
      if ( data[0] == '\"' ||
           data[0] == '\\' ||
           data[0] == '\b' ||
           data[0] == '\f' ||
           data[0] == '\n' ||
           data[0] == '\r' ||
           data[0] == '\t' )
      {
         ++len ;
      }
      ++len ;
      ++data ;
   }
   return len ;
}

/*
 * time_t convert tm
 * Time: the second time conversion
 * TM : the date structure
 * return : the data structure
*/
static void local_time( time_t *Time, struct tm *TM )
{
   if ( !Time || !TM )
      return ;
#if defined (__linux__ ) || defined (_AIX)
   localtime_r( Time, TM ) ;
#elif defined (_WIN32)
   // The Time represents the seconds elapsed since midnight (00:00:00),
   // January 1, 1970, UTC. This value is usually obtained from the time
   // function.
   localtime_s( TM, Time ) ;
#else
#error "unimplemented local_time()"
#endif
}
