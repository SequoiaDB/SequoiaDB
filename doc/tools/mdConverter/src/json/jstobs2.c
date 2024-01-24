/*******************************************************************************
   Copyright (C) 2012-2014 SequoiaDB Ltd.

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

#include "jstobs2.h"
#include "cJSON_ext.h"
#include "base64c.h"
#include "timestamp.h"

#define INT_NUM_SIZE 32

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

#define TIME_FORMAT "%d-%d-%d-%d.%d.%d.%d"
#define DATE_FORMAT "%d-%d-%d"

#define DATE_OUTPUT_CSV_FORMAT "%04d-%02d-%02d"
#define DATE_OUTPUT_FORMAT "{ \"$date\": \"" DATE_OUTPUT_CSV_FORMAT "\" }"

#define TIME_OUTPUT_CSV_FORMAT "%04d-%02d-%02d-%02d.%02d.%02d.%06d"
#define TIME_OUTPUT_FORMAT "{ \"$timestamp\": \"" TIME_OUTPUT_CSV_FORMAT "\" }"

#define TIME_STAMP_TIMESTAMP_MIN (-2147483648LL)
#define TIME_STAMP_TIMESTAMP_MAX  (2147483647LL)

static void get_char_num ( CHAR *str, INT32 i, INT32 str_size ) ;
static CHAR* intToString ( INT32 value, CHAR *string, INT32 radix ) ;
static BOOLEAN date2Time( const CHAR *pDate,
                          CJSON_VALUE_TYPE valType,
                          time_t *pTimestamp,
                          INT32 *pMicros ) ;

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
                                BOOLEAN isObj ) ;

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

/*
 * json convert bson interface
 * pJson : json string
 * pBson : bson object
 * return : the conversion result
*/
SDB_EXPORT BOOLEAN json2bson2( const CHAR *pJson, bson *pBson )
{
   return json2bson( pJson, NULL, CJSON_RIGOROUS_PARSE, TRUE, pBson ) ;
}

/*
 * json convert bson interface
 * pJson : json string
 * pMachine : cJSON state machine
 * parseMode: 0 - loose mode
 *            1 - rigorous mode
 * pBson : bson object
 * return : the conversion result
*/
/* THIS IS EXTERNAL FUNCTION TO CONVERT FROM JSON STRING INTO BSON OBJECT */
SDB_EXPORT BOOLEAN json2bson( const CHAR *pJson,
                              CJSON_MACHINE *pMachine,
                              INT32 parseMode,
                              BOOLEAN isCheckEnd,
                              bson *pBson )
{
   BOOLEAN flag = TRUE ;
   BOOLEAN isOwn = FALSE ;
   static BOOLEAN isInit = FALSE ;
   const cJson_iterator *pIter = NULL ;

   if( isInit == FALSE )
   {
      if( cJsonExtAppendFunction() == FALSE )
      {
         JSON_PRINTF_LOG( "Failed to append extend function" ) ;
         goto error ;
      }
      isInit = TRUE ;
   }

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

   cJsonInit( pMachine, parseMode, isCheckEnd ) ;

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

   bson_init( pBson ) ;

   if( jsonConvertBson( pMachine, pIter, pBson, TRUE ) == FALSE )
   {
      JSON_PRINTF_LOG( "Failed to convert json to bson" ) ;
      goto error ;
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
   return flag ;
error:
   flag = FALSE ;
   goto done ;
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
      date min 1900-01-01-00.00.00.000000
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
         if( !sscanf ( pDate,
                       TIME_FORMAT,
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
         //[1900,9999]
         if( year > INT64_LAST_YEAR )
         {
            JSON_PRINTF_LOG( "Date year not greater than %d",
                             INT64_LAST_YEAR ) ;
            goto error ;
         }
         else if( year < RELATIVE_YEAR )
         {
            JSON_PRINTF_LOG( "Date year not less than %d", RELATIVE_YEAR ) ;
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
      timep = mktime( &t ) ;
   }

   if( valType == CJSON_TIMESTAMP )
   {
      if( (INT64)timep > TIME_STAMP_TIMESTAMP_MAX )
      {
         JSON_PRINTF_LOG( "Timestamp not greater than "
                          "2038-01-19-03.14.07.999999 +/- TZ" ) ;
         goto error ;
      }
      else if( (INT64)timep < TIME_STAMP_TIMESTAMP_MIN )
      {
         JSON_PRINTF_LOG( "Timestamp not less than "
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
                                BOOLEAN isObj )
{
   BOOLEAN flag = TRUE ;
   INT32 i = 0 ;
   CJSON_VALUE_TYPE cJsonType = CJSON_NONE ;
   const CHAR *pKey = NULL ;
   CVALUE arg1 ;
   CVALUE arg2 ;
   CHAR numKey[ INT_NUM_SIZE ] = {0} ;

   while( cJsonIteratorMore( pIter ) )
   {
      cJsonType = cJsonIteratorType( pIter ) ;
      if( isObj == TRUE )
      {
         pKey = cJsonIteratorKey( pIter ) ;
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
         if( jsonConvertBson( pMachine, pIterSub, pBson, TRUE ) == FALSE )
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
         if( jsonConvertBson( pMachine, pIterSub, pBson, TRUE ) == FALSE )
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
               JSON_PRINTF_LOG( "Failed to read NumberLong, "
                                "the No.1 argument is an invalid number" ) ;
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
            else
            {
               JSON_PRINTF_LOG( "Failed to read NumberLong, the No.1 argument "
                                "must be integer type or string type" ) ;
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
         bson_decimal bsonDecimal ;
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
            if( decimal_init1( &bsonDecimal,
                               precisionVal.valInt,
                               scaleVal.valInt ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to init decimal, key: %s", pKey ) ;
               goto error ;
            }
         }
         else
         {
            decimal_init( &bsonDecimal ) ;
         }
         if( arg1.valType == CJSON_INT32 )
         {
            if( decimal_from_int( arg1.valInt, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal int, key: %s", pKey ) ;
               goto error ;
            }
         }
         else if( arg1.valType == CJSON_INT64 )
         {
            if( decimal_from_long( arg1.valInt64, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal int64, key: %s",
                                pKey ) ;
               goto error ;
            }
         }
         else if( arg1.valType == CJSON_DOUBLE )
         {
            if( decimal_from_double( arg1.valDouble, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal double, key: %s",
                                pKey ) ;
               goto error ;
            }
         }
         else if( arg1.valType == CJSON_STRING )
         {
            if( decimal_from_str( arg1.pValStr, &bsonDecimal ) != 0 )
            {
               JSON_PRINTF_LOG( "Failed to build decimal string, key: %s",
                                pKey ) ;
               goto error ;
            }
         }
         if( bson_append_decimal( pBson, pKey, &bsonDecimal ) == BSON_ERROR )
         {
            JSON_PRINTF_LOG( "Failed to append bson '%s' decimal", pKey ) ;
            decimal_free( &bsonDecimal ) ;
            goto error ;
         }
         decimal_free( &bsonDecimal ) ;
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
