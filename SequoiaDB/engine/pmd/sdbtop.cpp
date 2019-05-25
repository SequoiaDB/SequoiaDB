/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbtop.cpp

   Descriptive Name = Import Utility

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbimprt
   which is used to do data import

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/25/2014  YB R  Initial Draft

   Last Changed =

*******************************************************************************/
#include "../client/client.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"

#include <sys/select.h>
#include <termios.h>
#include <string.h>
#include "curses.h"
#include "sptCommon.hpp"
#include "utilPath.hpp"
#include "ossVer.hpp"
#include <sys/time.h>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp>

using namespace sdbclient;
using namespace bson;
using namespace std;
using namespace boost::property_tree;
namespace po = boost::program_options;


#define SDBTOP_SAFE_DELETE(p) \
   do {                    \
      if (p) {             \
         SDB_OSS_DEL []p ;   \
         p = NULL ;        \
      }                    \
   } while (0)

#define STDIN 0

#define SDBTOP_VERSION "sdbtop 1.0"
#define SDBTOP_DEFAULT_CONFPATH "../conf/samples/sdbtop.xml"
#define SDBTOP_DEFAULT_HOSTNAME "localhost"
#define SDBTOP_DEFAULT_SERVICENAME "11810"
#define SDBTOP_REFRESH_QUIT_HELP "Refresh: F5, Quit: q, Help: h"
#define NULLSTRING ""
#define STRING_NULL "NULL"
#define DIVIDINGCHAR "-"
#define OUTPUT_FORMATTING "%.3f"

#define ZOOM_MODE_ALL "ZOOM_MODE_ALL"
#define ZOOM_MODE_NONE "ZOOM_MODE_NONE"

#define ZOOM_MODE_POS "ZOOM_MODE_POS"
#define ZOOM_MODE_ROW_POS "ZOOM_MODE_ROW_POS"
#define ZOOM_MODE_COL_POS "ZOOM_MODE_COL_POS"

#define ZOOM_MODE_POS_X "ZOOM_MODE_POS_X"
#define ZOOM_MODE_ROW_POS_X "ZOOM_MODE_ROW_POS_X"
#define ZOOM_MODE_COL_POS_X "ZOOM_MODE_COL_POS_X"

#define ZOOM_MODE_POS_Y "ZOOM_MODE_POS_Y"
#define ZOOM_MODE_ROW_POS_Y "ZOOM_MODE_ROW_POS_Y"
#define ZOOM_MODE_COL_POS_Y "ZOOM_MODE_COL_POS_Y"

#define ZOOM_MODE_ROW_COL "ZOOM_MODE_ROW_COL"
#define ZOOM_MODE_COL "ZOOM_MODE_COL"
#define ZOOM_MODE_ROW "ZOOM_MODE_ROW"

#define OCCUPY_MODE_NONE "OCCUPY_MODE_NONE"
#define OCCUPY_MODE_WINDOW_BELOW "OCCUPY_MODE_WINDOW_BELOW"

#define JUMPTYPE_PANEL "JUMPTYPE_PANEL"
#define JUMPTYPE_FUNC "JUMPTYPE_FUNC"
#define JUMPTYPE_FIXED "JUMPTYPE_FIXED"
#define JUMPTYPE_GLOBAL "JUMPTYPE_GLOBAL"
#define JUMPTYPE_GROUP "JUMPTYPE_GROUP"
#define JUMPTYPE_NODE "JUMPTYPE_NODE"
#define JUMPTYPE_ASC "JUMPTYPE_ASC"
#define JUMPTYPE_DESC "JUMPTYPE_DESC"
#define JUMPTYPE_FILTER_CONDITION "JUMPTYPE_FILTER_CONDITION"
#define JUMPTYPE_NO_FILTER_CONDITION "JUMPTYPE_NO_FILTER_CONDITION"
#define JUMPTYPE_FILTER_NUMBER "JUMPTYPE_FILTER_NUMBER"
#define JUMPTYPE_NO_FILTER_NUMBER "JUMPTYPE_NO_FILTER_NUMBER"
#define JUMPTYPE_REFRESHINTERVAL "JUMPTYPE_REFRESHINTERVAL"

#define SORTINGWAY_ASC "1"
#define SORTINGWAY_DESC "-1"

#define BODYTYPE_MAIN "BODYTYPE_MAIN"
#define BODYTYPE_NORMAL "BODYTYPE_NORMAL"
#define BODYTYPE_HELP_DYNAMIC "BODYTYPE_HELP_DYNAMIC"


#define TABLE "TABLE"
#define LIST "LIST"

#define DISPLAYTYPE_NULL "DISPLAYTYPE_NULL"
#define DISPLAYTYPE_STATICTEXT_HELP_Header "DISPLAYTYPE_STATICTEXT_HELP_Header"
#define DISPLAYTYPE_STATICTEXT_LICENSE "DISPLAYTYPE_STATICTEXT_LICENSE"
#define DISPLAYTYPE_STATICTEXT_MAIN "DISPLAYTYPE_STATICTEXT_MAIN"
#define DISPLAYTYPE_DYNAMIC_HELP "DISPLAYTYPE_DYNAMIC_HELP"
#define DISPLAYTYPE_DYNAMIC_EXPRESSION "DISPLAYTYPE_DYNAMIC_EXPRESSION"
#define DISPLAYTYPE_DYNAMIC_SNAPSHOT "DISPLAYTYPE_DYNAMIC_SNAPSHOT"

#define SERIALNUMBER_LENGTH 4

#define STATIC_EXPRESSION "STATIC_EXPRESSION"
#define DYNAMIC_EXPRESSION "DYNAMIC_EXPRESSION"

#define EXPRESSION_BODY_LABELNAME "EXPRESSION_BODY_LABELNAME"
#define EXPRESSION_VERSION "EXPRESSION_VERSION"
#define EXPRESSION_REFRESH_QUIT_HELP "EXPRESSION_REFRESH_QUIT_HELP"
#define EXPRESSION_REFRESH_TIME "EXPRESSION_REFRESH_TIME"
#define EXPRESSION_HOSTNAME "EXPRESSION_HOSTNAME"
#define EXPRESSION_SERVICENAME "EXPRESSION_SERVICENAME"
#define EXPRESSION_USRNAME "EXPRESSION_USRNAME"
#define EXPRESSION_DISPLAYMODE "EXPRESSION_DISPLAYMODE"
#define EXPRESSION_SNAPSHOTMODE "EXPRESSION_SNAPSHOTMODE"
#define EXPRESSION_FILTER_NUMBER "EXPRESSION_FILTER_NUMBER"
#define EXPRESSION_SORTINGWAY "EXPRESSION_SORTINGWAY"
#define EXPRESSION_SORTINGFIELD "EXPRESSION_SORTINGFIELD"
#define EXPRESSION_SNAPSHOTMODE_INPUTNAME "EXPRESSION_SNAPSHOTMODE_INPUTNAME"

#define UPPER_LEFT "UPPER_LEFT"
#define MIDDLE_LEFT "MIDDLE_LEFT"
#define LOWER_LEFT "LOWER_LEFT"
#define UPPER_MIDDLE "UPPER_MIDDLE"
#define MIDDLE "MIDDLE"
#define LOWER_MIDDLE "LOWER_MIDDLE"
#define UPPER_RIGHT "UPPER_RIGHT"
#define MIDDLE_RIGHT "MIDDLE_RIGHT"
#define LOWER_RIGHT "LOWER_RIGHT"

#define LEFT "LEFT"
#define CENTER "CENTER"
#define RIGHT "RIGHT"

#define SDB_SNAP_NULL "SDB_SNAP_NULL"
#define SDB_SNAP_CONTEXTS_TOP "SDB_SNAP_CONTEXTS_TOP"
#define SDB_SNAP_CONTEXTS_CURRENT_TOP "SDB_SNAP_CONTEXTS_CURRENT_TOP"
#define SDB_SNAP_SESSIONS_TOP "SDB_SNAP_SESSIONS_TOP"
#define SDB_SNAP_SESSIONS_CURRENT_TOP "SDB_SNAP_SESSIONS_CURRENT_TOP"
#define SDB_SNAP_COLLECTIONS_TOP "SDB_SNAP_COLLECTIONS_TOP"
#define SDB_SNAP_COLLECTIONSPACES_TOP "SDB_SNAP_COLLECTIONSPACES_TOP"
#define SDB_SNAP_DATABASE_TOP "SDB_SNAP_DATABASE_TOP"
#define SDB_SNAP_SYSTEM_TOP "SDB_SNAP_SYSTEM_TOP"
#define SDB_SNAP_CATALOG_TOP "SDB_SNAP_CATALOG_TOP"


#define DELTA "DELTA"
#define ABSOLUTE "ABSOLUTE"
#define AVERAGE "AVERAGE"
const INT32 DISPLAYMODENUMBER = 3;
const string DISPLAYMODECHOOSER[DISPLAYMODENUMBER] = { ABSOLUTE,
                                                       DELTA,
                                                       AVERAGE
                                                     };

#define ANYVALUE 0

#define GLOBAL "GLOBAL"
#define GROUP "GROUP"
#define NODE "NODE"

#define SDB_ERROR -1
#define HEADER_NULL -1
#define FOOTER_NULL -1

#define SDB_SDBTOP_DONE 1

#define REFRESH 0
#define NOTREFRESH 1

#define COLOR_MULTIPLE 8
#define BUTTON_LEFT 4479771
#define BUTTON_RIGHT 4414235
#define BUTTON_TAB 9
#define BUTTON_ENTER 13
#define BUTTON_ESC 27
#define BUTTON_H_LOWER 'h'
#define BUTTON_Q_LOWER 'q'
#define BUTTON_F5 542058306331

#define PREFIX_TAB      " Tab : "
#define PREFIX_LEFT     "  <- : "
#define PREFIX_RIGHT    "  -> : "
#define PREFIX_ENTER    "Enter: "
#define PREFIX_ESC      " ESC : "
#define PREFIX_F5       " F5  : "
#define PREFIX_NULL     "NULL : "
#define PREFIX_FORMAT   "  %c  : "

#define REFERUPPERLEFT_X "referUpperLeft_X"
#define REFERUPPERLEFT_Y "referUpperLeft_Y"
#define LENGTH_X "length_X"
#define LENGTH_Y "length_Y"
#define AUTOSETTYPE_XML "autoSetType"
#define COLOUR_FOREGROUNDCOLOR "colour.foreGroundColor"
#define COLOUR_BACKGROUNDCOLOR "colour.backGroundColor"
#define PREFIXCOLOUR_FOREGROUNDCOLOR "prefixColour.foreGroundColor"
#define PREFIXCOLOUR_BACKGROUNDCOLOR "prefixColour.backGroundColor"
#define CONTENTCOLOUR_FOREGROUNDCOLOR "contentColour.foreGroundColor"
#define CONTENTCOLOUR_BACKGROUNDCOLOR "contentColour.backGroundColor"
#define WNDROW "wndOptionRow"
#define WNDCOLUMN "wndOptionColumn"
#define OPTIONROW "optionRow"
#define OPTIONCOL "optionColumn"
#define CELLLENGTH "cellLength"
#define EXPRESSIONNUMBER "expressionNumber"
#define ROWNUMBER "rowNumber"
#define EXPRESSIONCONTENT "ExpressionContent"
#define ALIGNMENT "alignment"
#define ROWLOCATION "rowLocation"
#define EXPRESSIONTYPE "expressionType"
#define EXPRESSIONLENGTH "expressionLength"
#define EXPRESSIONVALUE_TEXT "expressionValue.text"
#define EXPRESSIONVALUE_EXPRESSION "expressionValue.expression"
#define GLOBAL_AUTOSETTYPE "globalAutoSetType"
#define GROUP_AUTOSETTYPE "groupAutoSetType"
#define NODE_AUTOSETTYPE "nodeAutoSetType"
#define GLOBAL_STYLE "globalStyle"
#define GROUP_STYLE "groupStyle"
#define NODE_STYLE "nodeStyle"
#define GLOBAL_ROW "globalRow"
#define GROUP_ROW "groupRow"
#define NODE_ROW "nodeRow"
#define GLOBAL_COL "globalCol"
#define GROUP_COL "groupCol"
#define NODE_COL "nodeCol"
#define TABLE_CELLLENGTH "tableCellLength"
#define BASEFIELD "baseField"
#define FIELDLENGTH "fieldLength"
#define FIXED "fixed"
#define MOBILE "mobile"
#define FIELDSTRUCT "FieldStruct"
#define ABSOLUTENAME "absoluteName"
#define SOURCEFIELD "sourceField"
#define CONTENTLENGTH "contentLength"
#define ABSOLUTECOLOUR_FOREGROUNDCOLOR "absoluteColour.foreGroundColor"
#define ABSOLUTECOLOUR_BACKGROUNDCOLOR "absoluteColour.backGroundColor"
#define WARNINGVALUE_ABSOLUTE_MAX_LIMITVALUE "warningValue.absoluteMaxLimitValue"
#define WARNINGVALUE_ABSOLUTE_MIN_LIMITVALUE "warningValue.absoluteMinLimitValue"
#define WARNINGVALUE_DELTA_MAX_LIMITVALUE "warningValue.deltaMaxLimitValue"
#define WARNINGVALUE_DELTA_MIN_LIMITVALUE "warningValue.deltaMinLimitValue"
#define WARNINGVALUE_AVERAGE_MAX_LIMITVALUE "warningValue.averageMaxLimitValue"
#define WARNINGVALUE_AVERAGE_MIN_LIMITVALUE "warningValue.averageMinLimitValue"
#define CANSWITCH "canSwitch"
#define DELTANAME "deltaName"
#define AVERAGENAME "averageName"
#define DELTACOLOUR_FOREGROUNDCOLOR "deltaColour.foreGroundColor"
#define DELTACOLOUR_BACKGROUNDCOLOR "deltaColour.backGroundColor"
#define AVERAGECOLOUR_FOREGROUNDCOLOR "averageColour.foreGroundColor"
#define AVERAGECOLOUR_BACKGROUNDCOLOR "averageColour.backGroundColor"
#define NUMOFSUBWINDOW "numOfSubWindow"
#define NODEWINDOW "NodeWindow"
#define ACTUALWINDOWMINROW "actualWindowMinRow"
#define ACTUALWINDOWMINCOLUMN "actualWindowMinColumn"
#define ZOOMMODE "zoomMode"
#define DISPLAYTYPE "displayType"
#define DISPLAYCONTENT "displayContent"
#define OCCUPYMODE "occupyMode"
#define POSITION "position"
#define EVENT "Event"
#define ROOTWINDOW "RootWindow"
#define REFERWINDOWROW "referWindowRow"
#define REFERWINDOWCOLUMN "referWindowColumn"
#define ACTUALWINDOWMINROW "actualWindowMinRow"
#define ACTUALWINDOWMINCOLUMN "actualWindowMinColumn"
#define REFRESHINTERVAL "refreshInterval"
#define COLOUROFTHECHANGE_FOREGROUNDCOLOR "colourOfTheChange.foreGroundColor"
#define COLOUROFTHECHANGE_BACKGROUNDCOLOR "colourOfTheChange.backGroundColor"
#define COLOUROFTHEDIVIDINGLINE_FOREGROUNDCOLOR "colourOfTheDividingLine.foreGroundColor"
#define COLOUROFTHEDIVIDINGLINE_BACKGROUNDCOLOR "colourOfTheDividingLine.backGroundColor"
#define COLOUROFTHEMAX_FOREGROUNDCOLOR "colourOfTheMax.foreGroundColor"
#define COLOUROFTHEMAX_BACKGROUNDCOLOR "colourOfTheMax.backGroundColor"
#define COLOUROFTHEMIN_FOREGROUNDCOLOR "colourOfTheMin.foreGroundColor"
#define COLOUROFTHEMIN_BACKGROUNDCOLOR "colourOfTheMin.backGroundColor"
#define KEYSUITES "KeySuites"
#define KEYSUITELENGTH "keySuiteLength"
#define KEYSUITE "KeySuite"
#define MARK "mark"
#define HOTKEYLENGTH "hotKeyLength"
#define HOTKEY "HotKey"
#define BUTTON "button"
#define JUMPTYPE "jumpType"
#define JUMPNAME "jumpName"
#define KEYDESC "desc"
#define WNDTYPE "wndtype"
#define HEADERS "Headers"
#define HEADERLENGTH "headerLength"
#define HEADTAILMAP "HeadTailMap"
#define KEY "key"
#define VALUE "value"
#define BODIES "Bodies"
#define BODYLENGTH "bodyLength"
#define BODYMAP "BodyMap"
#define HEADERKEY "headerKey"
#define FOOTERKEY "footerKey"
#define LABELNAME "labelName"
#define BODYPANELTYPE "bodyPanelType"
#define HOTKEYSUITETYPE "hotKeySuiteType"
#define HELPPANELTYPE "helpPanelType"
#define SOURCESNAPSHOT "sourceSnapShot"
#define FOOTERS "Footers"
#define FOOTERLENGTH "footerLength"

CHAR* HELP_DETAIL = "[Help for SDBTOP]";
CHAR* SDB_TOP_LICENSE =
      "Licensed Materials - Property of SequoiaDB"OSS_NEWLINE
      "Copyright SequoiaDB Corp. 2013-2015 All Rights Reserved.";
CHAR* SDB_TOP_DESC =
" #### ####  ####  #####  ###  ####   For help type h or ..."OSS_NEWLINE
"#     #   # #   #   #   #   # #   #  sdbtop -h: usage"OSS_NEWLINE
" ###  #   # ####    #   #   # ####"OSS_NEWLINE
"    # #   # #   #   #   #   # #"OSS_NEWLINE
"####  ####  ####    #    ###  #"OSS_NEWLINE
OSS_NEWLINE
"SDB Interactive Snapshot Monitor V2.0"OSS_NEWLINE
"Use these keys to ENTER:";

#define BUFFERSIZE         256
CHAR sdbtopBuffer[BUFFERSIZE] = {0} ;
const INT32 errStrLength = 1024 ;
CHAR errStr[errStrLength] = {0} ;
CHAR errStrBuf[errStrLength] = {0} ;
CHAR progPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
string confPath = SDBTOP_DEFAULT_CONFPATH ;
string hostname = SDBTOP_DEFAULT_HOSTNAME ;
string serviceName = SDBTOP_DEFAULT_SERVICENAME ;
string usrName = NULLSTRING ;
string password = NULLSTRING ;
BOOLEAN useSSL = FALSE ;

#define OPTION_HELP          "help"
#define OPTION_CONFPATH      "confpath"
#define OPTION_HOSTNAME      "hostname"
#define OPTION_SERVICENAME   "servicename"
#define OPTION_USRNAME       "usrname"
#define OPTION_PASSWORD      "password"
#define OPTION_VERSION       "version"
#define OPTION_SSL           "ssl"


#define ADD_PARAM_OPTIONS_BEGIN( desc )\
        desc.add_options()

#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()
#define COMMANDS_OPTIONS \
       ( COMMANDS_STRING(OPTION_HELP, ",h"), "help" )\
       ( COMMANDS_STRING(OPTION_VERSION, ",v"), "version" ) \
       ( COMMANDS_STRING(OPTION_CONFPATH, ",c"),boost::program_options::value<string>(), "configuration file path, default: conf/samples/sdbtop.xml" ) \
       ( COMMANDS_STRING(OPTION_HOSTNAME, ",i"), boost::program_options::value<string>(), "host name, default: localhost" ) \
       ( COMMANDS_STRING(OPTION_SERVICENAME, ",s"), boost::program_options::value<string>(), "service name, default: 11810" ) \
       ( COMMANDS_STRING(OPTION_USRNAME, ",u"), boost::program_options::value<string>(), "username, default: \"\"" ) \
       ( COMMANDS_STRING(OPTION_PASSWORD, ",p"),boost::program_options::value<string>(), "password, default: \"\"" )

struct Colours
{
   INT32 foreGroundColor ;
   INT32 backGroundColor ;
} ;

struct StaticTextOutPut
{
   CHAR *outputText ;
   string   autoSetType ;
   Colours colour ;
} ;

struct ExpValueStruct
{
   string text ;
   string expression ;
} ;

class ExpressionContent : public SDBObject
{
public:
   string expressionType ;

   INT32 expressionLength ;

   ExpValueStruct expressionValue ;

   string alignment ;

   Colours colour ;

   INT32 rowLocation ;
} ;

struct DynamicExpressionOutPut
{
   ExpressionContent *content ;

   string autoSetType ;

   INT32 expressionNumber ;

   INT32 rowNumber ;
} ;

struct FiledWarningValue
{
   INT64 absoluteMaxLimitValue ;
   INT64 absoluteMinLimitValue ;

   INT64 deltaMaxLimitValue ;
   INT64 deltaMinLimitValue ;

   INT64 averageMaxLimitValue ;
   INT64 averageMinLimitValue ;
} ;


class FieldStruct : public SDBObject
{
public:

   string deltaName ;
   string absoluteName ;
   string averageName ;

   string sourceField ;
   INT32 contentLength ;
   string alignment ;
   Colours deltaColour ;
   Colours absoluteColour ;
   Colours averageColour ;

   BOOLEAN canSwitch ;

   FiledWarningValue warningValue ;
} ;

struct DynamicSnapshotOutPut
{
   FieldStruct* fixedField ;
   FieldStruct* mobileField ;
   INT32 actualFixedFieldLength ;
   INT32 actualMobileFieldLength ;
   INT32 fieldLength ;
   string globalAutoSetType ;
   string groupAutoSetType ;
   string nodeAutoSetType ;
   string baseField ;
   INT32 tableCellLength ;
   string globalStyle ;
   string groupStyle ;
   string nodeStyle ;
   INT32 globalRow ;
   INT32 globalCol ;
   INT32 groupRow ;
   INT32 groupCol ;
   INT32 nodeRow ;
   INT32 nodeCol ;
} ;

struct DynamicHelp
{
   INT32 wndOptionRow ;
   INT32 wndOptionCol ;
   INT32 optionsRow ;
   INT32 optionsCol ;
   INT32 cellLength ;
   Colours prefixColour ;
   Colours contentColour ;
   string autoSetType ;
} ;

struct DisplayContent
{
   StaticTextOutPut staticTextOutPut ;
   DynamicExpressionOutPut dyExOutPut ;
   DynamicSnapshotOutPut dySnapshotOutPut ;
   DynamicHelp dynamicHelp ;
} ;

struct Position
{
   INT32 referUpperLeft_X ;
   INT32 referUpperLeft_Y ;

   INT32 length_X ;
   INT32 length_Y ;
} ;

class NodeWindow : public SDBObject
{
public:
   INT32 actualWindowMinRow ;
   INT32 actualWindowMinColumn ;

   string zoomMode ;

   string displayType ;

   DisplayContent displayContent ;

   Position position ;

   string occupyMode;
};

struct Panel
{
   NodeWindow* subWindow ;
   INT32 numOfSubWindow ;
};

class HotKey : public SDBObject
{
public:

   INT64 button ;
   string jumpType ;
   string jumpName ;
   string desc ;
   BOOLEAN wndType ;
} ;

class KeySuite : public SDBObject
{
public:

   INT64 mark ;

   INT32 hotKeyLength ;

   INT32 hotKeyLengthFromConf ;

   HotKey *hotKey ;
} ;

class HeadTailMap : public SDBObject
{
public:
   INT32 key ;
   Panel value ;
} ;

class BodyMap : public SDBObject
{
public:
   INT32 headerKey ;
   Panel value ;
   INT32 footerKey ;
   string labelName ;
   INT64 hotKeySuiteType ;
   string sourceSnapShot ;
   string bodyPanelType ;
   string helpPanelType ;
} ;

struct InputPanel
{
   INT32 displayModeChooser ;
   string snapshotModeChooser;
   string groupName ;
   string nodeName ;
   INT32 fieldPosition ;
   BOOLEAN isFirstGetSnapshot ;
   vector<BSONObj> last_Snapshot ;
   vector<BSONObj> cur_Snapshot ;
   map<string, string> last_absoluteMap ;
   map<string, string> last_deltaMap ;
   map<string, string> last_averageMap ;
   map<string, string> cur_absoluteMap ;
   map<string, string> cur_deltaMap ;
   map<string, string> cur_averageMap ;
   string confPath ;
   INT32 refreshInterval ;
   BOOLEAN forcedToRefresh_Local ;
   BOOLEAN forcedToRefresh_Global ;
   BodyMap* activatedPanel ;
   Colours colourOfTheMax ;
   Colours colourOfTheMin ;
   Colours colourOfTheChange ;
   Colours colourOfTheDividingLine ;
   string sortingWay ;
   string sortingField ;
   string filterCondition ;
   INT32 filterNumber ;
} ;

struct RootWindow
{
   INT32 referWindowRow ;
   INT32 referWindowColumn ;
   INT32 actualWindowMinRow ;
   INT32 actualWindowMinColumn ;
   HeadTailMap *header ;
   INT32 headerLength ;
   BodyMap *body ;
   INT32 bodyLength ;
   HeadTailMap *footer ;
   INT32 footerLength ;
   InputPanel input ;
   KeySuite *keySuite ;
   INT32 keySuiteLength ;
} ;

class Event : public SDBObject
{
public: // features
   RootWindow root ;
   sdb* coord ;
public://consturct function
   Event() ;
   ~Event() ;
public: // operation

   INT32 assignActivatedPanelByLabelName( BodyMap **activatedPanel,
                                          string labelName ) ;

   INT32 assignActivatedPanel( BodyMap **activatedPanel,
                               string bodyPanelType ) ;

   INT32 getActivatedHeadTailMap(  BodyMap *activatedPanel,
                                   HeadTailMap **header,
                                   HeadTailMap **footer ) ;

   INT32 getActualPosition( Position &actualPosition, Position &referPosition,
                            const string zoomMode, const string occupyMode ) ;

   INT32 getActivatedKeySuite( KeySuite **keySuite ) ;

   INT32 mvprintw_SDBTOP( const string &expression, INT32 expressionLength,
                          const string &alignment,
                          INT32 start_row, INT32 start_col ) ;

   INT32 mvprintw_SDBTOP( const char *expression, INT32 expressionLength,
                          const string &alignment,
                          INT32 start_row, INT32 start_col ) ;

   void getColourPN( Colours colour, INT32 &colourPairNumber );

   INT32 getResultFromBSONObj( const BSONObj &bsonobj,
                               const string &sourceField,
                               const string &displayMode,
                               string &result, BOOLEAN canSwitch,
                               const string& baseField,
                               const FiledWarningValue& waringValue,
                               INT32 &colourPairNumber ) ;

   INT32 getExpression( string& expression, string& result ) ;

   INT32 getCurSnapshot() ;

   INT32 fixedOutputLocation( INT32 start_row, INT32 start_col,
                              INT32 &fixed_row, INT32 &fixed_col,
                              INT32 referRowLength, INT32 referColLength,
                              const string &autoSetType ) ;

   INT32 getFieldNameAndColour( const FieldStruct &fieldStruct,
                                const string &displayMode, string &fieldName,
                                Colours &fieldColour ) ;

   INT32 refreshDH( DynamicHelp &DH, Position &position ) ;

   INT32 refreshDE( DynamicExpressionOutPut &DE, Position &position ) ;

   INT32 refreshDS_Table( DynamicSnapshotOutPut &DS, INT32 ROW, INT32 COL,
                          Position &position, string autoSetType ) ;
   INT32 refreshDS_List( DynamicSnapshotOutPut &DS,
                                Position &position,
                                const string &autoSetType ) ;
   INT32 refreshDS( DynamicSnapshotOutPut &DS,
                    Position &position ) ;

   INT32 refreshDisplayContent( DisplayContent &displayContent,
                                string displayType,
                                Position &actualPosition ) ;

   INT32 refreshNodeWindow( NodeWindow &window ) ;

   INT32 refreshHT( HeadTailMap *headtail ) ;

   INT32 refreshBD( BodyMap *body ) ;

   void initAllColourPairs() ;

   INT32 addFixedHotKey() ;

   INT32 matchNameInFieldStruct( const FieldStruct *src,
                                 const string DisplayName ) ;
   INT32 matchSourceFieldByDisplayName( const string DisplayName ) ;

   INT32 eventManagement( INT64 key ,BOOLEAN isFirstStart ) ;
   INT32 refreshAll( HeadTailMap *header, BodyMap *body,
                     HeadTailMap *footer, BOOLEAN refreshAferClean ) ;

   INT32 runSDBTOP( BOOLEAN useSSL = FALSE ) ;
};


static OSS_INLINE std::string &ltrim ( std::string &s )
{
   s.erase ( s.begin(), std::find_if ( s.begin(), s.end(),
             std::not1 ( std::ptr_fun<int, int>(std::isspace)))) ;
   return s ;
}

static OSS_INLINE std::string &rtrim ( std::string &s )
{
   s.erase ( std::find_if ( s.rbegin(), s.rend(),
             std::not1 ( std::ptr_fun<int, int>(std::isspace))).base(),
             s.end() ) ;
   return s ;
}

static OSS_INLINE std::string &trim ( std::string &s )
{
   return ltrim ( rtrim ( s ) ) ;
}

static OSS_INLINE std::string &doubleQuotesTrim( std::string &s )
{
   const INT32 strLength = s.length() ;
   if( 2 >= strLength )
   {
      return s ;
   }
   if( '\"' == s[0] && '\"' == s[strLength-1] )
   {
      s = s.substr( 1, strLength-2 ) ;
   }
   return s ;
}

static OSS_INLINE BOOLEAN isExist( map<string, string> &src, const string &key )
{
   return ( src.find( key ) != src.end() ) ;
}

static OSS_INLINE std::string getDividingLine( const string &dividingChar,
                                           INT32 dividingLength )
{
   string line = NULLSTRING ;
   while( dividingLength-- )
   {
      line += dividingChar ;
   }
   return line ;
}

INT32 strToNum( const CHAR *str, INT32 &number )
{
   INT32 rc           = SDB_OK ;
   INT32 pos          = 0 ;
   BOOLEAN isPositive = TRUE ;
   number    = 0 ;
   while( str[pos] )
   {
      if( 0 == pos )
      {
         if( '-' == str[pos] )
         {
            isPositive = FALSE ;
            ++pos ;
         }
         else if ( '+' == str[pos] )
         {
            isPositive = TRUE ;
            ++pos ;
         }
      }
      if( '0' > str[pos] ||'9' < str[pos] )
      {
         number = 0;
         rc = SDB_ERROR ;
         goto error;
      }
      number *= 10 ;
      number = number + str[pos] -'0' ;
      ++pos;
   }
   if( !isPositive )
   {
      number *= -1 ;
   }
done:
   return rc ;
error :
   goto done ;
}

INT32 getSdbTopKey( const CHAR *keyBuffer, INT64 &key )
{
   INT32 rc         = SDB_OK ;
   UINT32 bufLength = 0 ;
   bufLength = ( UINT32 )ossStrlen( keyBuffer ) ;
   if( 0 < bufLength )
   {
      if( 3 <= bufLength && 91 == keyBuffer[bufLength-2]
                         && 27 == keyBuffer[bufLength-3] )
      {
         if( 68 == keyBuffer[bufLength-1] )
         {
            key = BUTTON_LEFT ;
         }
         else if(  67 == keyBuffer[bufLength-1] )
         {
            key = BUTTON_RIGHT ;
         }
         else
         {
            key = 0 ;
         }
      }
      else if( 5 <= bufLength && 53 == keyBuffer[bufLength-2]
                              && 49 == keyBuffer[bufLength-3]
                              && 91 == keyBuffer[bufLength-4]
                              && 27 == keyBuffer[bufLength-5] )
      {
         if( 126 == keyBuffer[bufLength-1] )
         {
            key = BUTTON_F5 ;
         }
         else
         {
            key = 0 ;
         }
      }
      else
      {
         key = keyBuffer[bufLength-1] ;
      }
   }
   else
   {
      rc = SDB_ERROR ;
      key = 0 ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

static OSS_INLINE INT32 formattingOutput( CHAR *pBuffer, const INT32 fixedLength,
                                      const CHAR *pSrc )
{
   INT32 rc = SDB_OK ;
   INT32 i  = 0 ;
   try
   {
      for( i = 0; i < fixedLength && '\0' != pSrc[i]; ++i )
      {
         pBuffer[i] = pSrc[i] ;
      }
      pBuffer[i] = '\0' ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s SNPRINTF_TOP failed,"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

static OSS_INLINE INT32 MVPRINTW( const INT32 start_row, INT32 start_col,
                              const INT32 fixedLength, const char *printf_str,
                              const string &alignment )
{
   INT32 rc           = SDB_OK ;
   INT32 col_offset   = 0 ;
   INT32 printfLength = ossStrlen( printf_str ) ;
   if( LEFT == alignment )
   {
      mvprintw( start_row, start_col, printf_str ) ;
   }
   else if( RIGHT == alignment )
   {
      col_offset = fixedLength -  printfLength ;
      start_col += col_offset ;
      mvprintw( start_row, start_col, printf_str ) ;
   }
   else if( CENTER == alignment )
   {
      col_offset = ( fixedLength - printfLength ) / 2 ;
      start_col += col_offset ;
      mvprintw( start_row, start_col, printf_str ) ;
   }
   else
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s MVPRINTW wrong alignment:%s"OSS_NEWLINE,
                   errStrBuf, alignment.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 storePosition( ptree pt_position, Position& position )
{
   INT32 rc = SDB_OK ;
   try
   {
      position.referUpperLeft_X =
            pt_position.get<INT32>( REFERUPPERLEFT_X ) ;
      position.referUpperLeft_Y =
            pt_position.get<INT32>( REFERUPPERLEFT_Y ) ;
      position.length_X         =
            pt_position.get<INT32>( LENGTH_X ) ;
      position.length_Y         =
            pt_position.get<INT32>( LENGTH_Y ) ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readPosition failed,"
                   "e.what():%s"OSS_NEWLINE, errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 storeDE( ptree pt_display,
               DynamicExpressionOutPut &dEContent )
{
   INT32 rc                     = SDB_OK ;
   INT32 exNumber               = 0 ;
   ExpressionContent *pEContent = NULL ;
   try
   {
      dEContent.autoSetType =
            pt_display.get<string>( AUTOSETTYPE_XML ) ;
      dEContent.expressionNumber =
            pt_display.get<INT32>( EXPRESSIONNUMBER ) ;
      dEContent.rowNumber =
            pt_display.get<INT32>( ROWNUMBER ) ;
      dEContent.content =
            SDB_OSS_NEW ExpressionContent[dEContent.expressionNumber] ;
      exNumber = 0 ;
      for( BOOST_AUTO( child_display, pt_display.begin() );
           child_display != pt_display.end();
           ++child_display )
      {
         if( child_display->first == EXPRESSIONCONTENT )
         {
            pEContent = &dEContent.content[exNumber] ;
            if( exNumber >= dEContent.expressionNumber )
            {
               rc = SDB_ERROR ;
               goto error;
            }
            pEContent->alignment =
                  child_display->
                        second.get<string>( ALIGNMENT ) ;
            pEContent->colour.foreGroundColor =
                  child_display->
                        second.get<INT32>( COLOUR_FOREGROUNDCOLOR ) ;
            pEContent->colour.backGroundColor =
                  child_display->
                        second.get<INT32>( COLOUR_BACKGROUNDCOLOR ) ;
            pEContent->rowLocation =
                  child_display->
                        second.get<INT32>( ROWLOCATION ) ;
            pEContent->expressionType =
                  child_display->
                        second.get<string>( EXPRESSIONTYPE ) ;
            pEContent->expressionLength =
                  child_display->second.get<INT32>( EXPRESSIONLENGTH ) ;
            if( 1 > pEContent->expressionLength )
            {
               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s readDisplayContent failed"
                            "expressionLength too short:%d"OSS_NEWLINE,
                            errStrBuf,
                            pEContent->expressionLength ) ;
               goto error ;
            }
            if( DYNAMIC_EXPRESSION ==
                pEContent->expressionType )
            {
               pEContent->expressionValue.expression =
               child_display->
                     second.get<string>( EXPRESSIONVALUE_EXPRESSION ) ;
            }
            else if( pEContent->expressionType ==
                     STATIC_EXPRESSION )
            {
               pEContent->expressionValue.text =
                     child_display->
                           second.get<string>( EXPRESSIONVALUE_TEXT ) ;
            }
            else
            {
               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s readDisplayContent failed"
                            "expressionType == %s"OSS_NEWLINE,
                            errStrBuf,
                            pEContent->expressionType.c_str() ) ;
               rc = SDB_ERROR ;
               goto error ;
            }
            ++exNumber ;
         }
      }
      dEContent.expressionNumber = exNumber ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,"%s readDisplayContent failed"
                   "(displayType == DISPLAYTYPE_DYNAMIC_EXPRESSION),"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 storeDS( ptree pt_display,
               DynamicSnapshotOutPut & dSContent)
{
   INT32 rc              = SDB_OK ;
   INT32 actualFixedNum  = 0;
   INT32 actualMobileNum = 0;
   FieldStruct *fixed    = NULL ;
   FieldStruct *mobile   = NULL ;
   try
   {
      dSContent.globalAutoSetType =
            pt_display.get<string>( GLOBAL_AUTOSETTYPE ) ;
      dSContent.groupAutoSetType =
            pt_display.get<string>( GROUP_AUTOSETTYPE ) ;
      dSContent.nodeAutoSetType =
            pt_display.get<string>( NODE_AUTOSETTYPE ) ;

      dSContent.globalStyle = pt_display.get<string>( GLOBAL_STYLE ) ;
      dSContent.groupStyle = pt_display.get<string>( GROUP_STYLE ) ;
      dSContent.nodeStyle = pt_display.get<string>( NODE_STYLE ) ;
      if( TABLE == dSContent.globalStyle )
      {
         dSContent.globalRow = pt_display.get<INT32>( GLOBAL_ROW ) ;
         dSContent.globalCol = pt_display.get<INT32>( GLOBAL_COL ) ;
         dSContent.tableCellLength = pt_display.get<INT32>( TABLE_CELLLENGTH ) ;
      }

      if( TABLE == dSContent.groupStyle )
      {
         dSContent.groupRow = pt_display.get<INT32>( GROUP_ROW ) ;
         dSContent.groupCol = pt_display.get<INT32>( GROUP_COL ) ;
         dSContent.tableCellLength = pt_display.get<INT32>( TABLE_CELLLENGTH ) ;
      }

      if( TABLE == dSContent.nodeStyle )
      {
         dSContent.nodeRow = pt_display.get<INT32>( NODE_ROW ) ;
         dSContent.nodeCol = pt_display.get<INT32>( NODE_COL ) ;
         dSContent.tableCellLength = pt_display.get<INT32>( TABLE_CELLLENGTH ) ;
      }

      dSContent.baseField = pt_display.get<string>( BASEFIELD ) ;
      dSContent.fieldLength = pt_display.get<INT32>( FIELDLENGTH ) ;
      dSContent.fixedField =
            SDB_OSS_NEW FieldStruct[dSContent.fieldLength] ;
      dSContent.mobileField =
            SDB_OSS_NEW FieldStruct[dSContent.fieldLength] ;

   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,"%s readDisplayContent failed"
                   "(displayType == DISPLAYTYPE_DYNAMIC_SNAPSHOT),"
                   "e.what():%s"OSS_NEWLINE, errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error;
   }

   actualFixedNum = 0;
   actualMobileNum = 0;
   for( BOOST_AUTO( child_display, pt_display.begin() );
        child_display != pt_display.end();
        ++child_display )
   {
      if( child_display->first == FIXED )
      {
         for( BOOST_AUTO( child_pFixed, child_display->second.begin() );
              child_pFixed != child_display->second.end();
              ++child_pFixed )
         {
            if( child_pFixed->first == FIELDSTRUCT )
            {
               fixed = &dSContent.fixedField[actualFixedNum] ;
               if( actualFixedNum >= dSContent.fieldLength )
               {
                  break ;
               }
               fixed->absoluteName =
                     child_pFixed->second.get<string>( ABSOLUTENAME ) ;
               fixed->sourceField =
                     child_pFixed->second.get<string>( SOURCEFIELD ) ;
               fixed->contentLength =
                     child_pFixed->second.get<INT32>( CONTENTLENGTH ) ;
               fixed->alignment =
                     child_pFixed->second.get<string>( ALIGNMENT ) ;

               fixed->absoluteColour.foreGroundColor =
                     child_pFixed->
                           second.get<INT32>(
                                 ABSOLUTECOLOUR_FOREGROUNDCOLOR ) ;
               fixed->absoluteColour.backGroundColor =
                     child_pFixed->
                           second.get<INT32>(
                                 ABSOLUTECOLOUR_BACKGROUNDCOLOR ) ;

               try
               {
                  fixed->warningValue.absoluteMaxLimitValue =
                     child_pFixed->
                           second.get<INT64>(
                                 WARNINGVALUE_ABSOLUTE_MAX_LIMITVALUE ) ;
                  fixed->warningValue.absoluteMinLimitValue =
                     child_pFixed->
                           second.get<INT64>(
                                 WARNINGVALUE_ABSOLUTE_MIN_LIMITVALUE ) ;
               }
               catch( std::exception &e )
               {
                  fixed->warningValue.absoluteMaxLimitValue = 0 ;
                  fixed->warningValue.absoluteMinLimitValue = 0 ;
               }

               fixed->canSwitch= child_pFixed->
                                       second.get<INT32>( CANSWITCH ) ;
               if( 1 == fixed->canSwitch )
               {
                  fixed->deltaName =
                        child_pFixed->
                              second.get<string>(
                                    DELTANAME ) ;
                  fixed->averageName =
                        child_pFixed->
                              second.get<string>(
                                    AVERAGENAME ) ;
                  fixed->deltaColour.foreGroundColor =
                        child_pFixed->
                              second.get<INT32>(
                                    DELTACOLOUR_FOREGROUNDCOLOR ) ;
                  fixed->deltaColour.backGroundColor =
                        child_pFixed->
                              second.get<INT32>(
                                    DELTACOLOUR_BACKGROUNDCOLOR ) ;

                  fixed->averageColour.foreGroundColor =
                        child_pFixed->
                              second.get<INT32>(
                                    AVERAGECOLOUR_FOREGROUNDCOLOR ) ;
                  fixed->averageColour.backGroundColor =
                        child_pFixed->second.get<INT32>(
                              AVERAGECOLOUR_BACKGROUNDCOLOR ) ;
                  try
                  {
                     fixed->warningValue.deltaMaxLimitValue =
                           child_pFixed->
                                 second.get<INT64>(
                                       WARNINGVALUE_DELTA_MAX_LIMITVALUE ) ;
                     fixed->warningValue.deltaMinLimitValue =
                           child_pFixed->
                                 second.get<INT64>(
                                       WARNINGVALUE_DELTA_MIN_LIMITVALUE ) ;
                     fixed->warningValue.averageMaxLimitValue =
                           child_pFixed->
                                 second.get<INT64>(
                                       WARNINGVALUE_AVERAGE_MAX_LIMITVALUE ) ;
                     fixed->warningValue.averageMinLimitValue =
                           child_pFixed->
                                 second.get<INT64>(
                                       WARNINGVALUE_AVERAGE_MIN_LIMITVALUE ) ;
                  }
                  catch( std::exception &e )
                  {
                     fixed->warningValue.deltaMaxLimitValue = 0 ;
                     fixed->warningValue.deltaMinLimitValue = 0 ;
                     fixed->warningValue.averageMaxLimitValue = 0 ;
                     fixed->warningValue.averageMinLimitValue= 0 ;

                  }

               }
            }
            ++actualFixedNum ;
         }
      }
      else if( MOBILE == child_display->first )
      {
         for( BOOST_AUTO( child_pMobile, child_display->second.begin() );
              child_pMobile != child_display->second.end();
              ++child_pMobile )
         {
            if( child_pMobile->first == FIELDSTRUCT )
            {
               mobile = &dSContent.mobileField[actualMobileNum] ;
               if( actualMobileNum >=
                     dSContent.fieldLength )
               {
                  break ;
               }
               mobile->absoluteName =
                     child_pMobile->
                           second.get<string>(
                                 ABSOLUTENAME ) ;
               mobile->sourceField =
                     child_pMobile->
                           second.get<string>(
                                 SOURCEFIELD ) ;
               mobile->contentLength =
                     child_pMobile->
                           second.get<INT32>(
                                 CONTENTLENGTH ) ;
               mobile->alignment =
                     child_pMobile->
                           second.get<string>(
                                 ALIGNMENT ) ;

               mobile->absoluteColour.foreGroundColor =
                     child_pMobile->
                           second.get<INT32>(
                                 ABSOLUTECOLOUR_FOREGROUNDCOLOR ) ;
               mobile->absoluteColour.backGroundColor =
                     child_pMobile->
                           second.get<INT32>(
                                 ABSOLUTECOLOUR_BACKGROUNDCOLOR ) ;
               try
               {
                  mobile->warningValue.absoluteMaxLimitValue =
                        child_pMobile->
                              second.get<INT64>(
                                    WARNINGVALUE_ABSOLUTE_MAX_LIMITVALUE ) ;
                  mobile->warningValue.absoluteMinLimitValue =
                        child_pMobile->
                              second.get<INT64>(
                                    WARNINGVALUE_ABSOLUTE_MIN_LIMITVALUE ) ;
               }
               catch( std::exception &e )
               {
                  mobile->warningValue.absoluteMaxLimitValue = 0 ;
                  mobile->warningValue.absoluteMinLimitValue = 0 ;
               }

               mobile->canSwitch=
                     child_pMobile->
                           second.get<INT32>(
                                 CANSWITCH ) ;
               if( 1 == mobile->canSwitch )
               {
                  mobile->deltaName =
                        child_pMobile->
                              second.get<string>(
                                    DELTANAME ) ;
                  mobile->averageName =
                        child_pMobile->
                              second.get<string>(
                                    AVERAGENAME ) ;
                  mobile->deltaColour.foreGroundColor =
                        child_pMobile->
                              second.get<INT32>(
                                    DELTACOLOUR_FOREGROUNDCOLOR ) ;
                  mobile->deltaColour.backGroundColor =
                        child_pMobile->
                              second.get<INT32>(
                                    DELTACOLOUR_BACKGROUNDCOLOR ) ;
                  mobile->averageColour.foreGroundColor =
                        child_pMobile->
                              second.get<INT32>(
                                    AVERAGECOLOUR_FOREGROUNDCOLOR ) ;
                  mobile->averageColour.backGroundColor =
                        child_pMobile->
                              second.get<INT32>(
                                    AVERAGECOLOUR_BACKGROUNDCOLOR ) ;

                  try
                  {
                     mobile->warningValue.deltaMaxLimitValue =
                           child_pMobile->
                                 second.get<INT64>(
                                       WARNINGVALUE_DELTA_MAX_LIMITVALUE ) ;
                     mobile->warningValue.deltaMinLimitValue =
                           child_pMobile->
                                 second.get<INT64>(
                                       WARNINGVALUE_DELTA_MIN_LIMITVALUE ) ;
                     mobile->warningValue.averageMaxLimitValue =
                           child_pMobile->
                                 second.get<INT64>(
                                       WARNINGVALUE_AVERAGE_MAX_LIMITVALUE ) ;
                     mobile->warningValue.averageMinLimitValue =
                           child_pMobile->
                                 second.get<INT64>(
                                       WARNINGVALUE_AVERAGE_MIN_LIMITVALUE ) ;
                  }
                  catch( std::exception &e )
                  {
                     mobile->warningValue.deltaMaxLimitValue= 0 ;
                     mobile->warningValue.deltaMinLimitValue= 0 ;
                     mobile->warningValue.averageMaxLimitValue= 0 ;
                     mobile->warningValue.averageMinLimitValue= 0 ;
                  }
               }
            }
               ++actualMobileNum ;
         }
      }
   }
   dSContent.actualFixedFieldLength= actualFixedNum ;
   dSContent.actualMobileFieldLength= actualMobileNum ;
done:
   return rc ;
error:
   goto done ;
}

INT32 storeDisplayContent( ptree pt_displayContent,
                           DisplayContent &display, string displayType )
{
   INT32 rc                           = SDB_OK ;
   StaticTextOutPut &sTContent        = display.staticTextOutPut ;
   DynamicExpressionOutPut &dEContent = display.dyExOutPut ;
   DynamicSnapshotOutPut &dSContent   = display.dySnapshotOutPut ;
   DynamicHelp &dHContent             = display.dynamicHelp ;
   if( DISPLAYTYPE_STATICTEXT_HELP_Header == displayType ||
       DISPLAYTYPE_STATICTEXT_LICENSE     == displayType ||
       DISPLAYTYPE_STATICTEXT_MAIN        == displayType )
   {
      try
      {
         sTContent.autoSetType =
               pt_displayContent.get<string>( AUTOSETTYPE_XML ) ;
         sTContent.colour.foreGroundColor =
               pt_displayContent.get<INT32>( COLOUR_FOREGROUNDCOLOR ) ;
         sTContent.colour.backGroundColor =
               pt_displayContent.get<INT32>( COLOUR_BACKGROUNDCOLOR ) ;
         if( DISPLAYTYPE_STATICTEXT_HELP_Header == displayType )
            sTContent.outputText = HELP_DETAIL ;
         else if( DISPLAYTYPE_STATICTEXT_LICENSE == displayType )
            sTContent.outputText = SDB_TOP_LICENSE ;
         else if( DISPLAYTYPE_STATICTEXT_MAIN == displayType )
            sTContent.outputText = SDB_TOP_DESC ;
      }
      catch( std::exception &e )
      {
         ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
         ossSnprintf( errStr, errStrLength,"%s readDisplayContent failed"
                      "(displayType == DISPLAYTYPE_STATICTEXT_HELP_Header), "
                      "e.what():%s"OSS_NEWLINE,
                      errStrBuf, e.what() ) ;
         rc = SDB_ERROR ;
         goto error ;
      }
   }
   else if( DISPLAYTYPE_DYNAMIC_HELP == displayType )
   {
      try
      {
         dHContent.autoSetType =
               pt_displayContent.get<string>( AUTOSETTYPE_XML ) ;
         dHContent.prefixColour.foreGroundColor =
               pt_displayContent.get<INT32>( PREFIXCOLOUR_FOREGROUNDCOLOR ) ;
         dHContent.prefixColour.backGroundColor =
               pt_displayContent.get<INT32>( PREFIXCOLOUR_BACKGROUNDCOLOR ) ;
         dHContent.contentColour.foreGroundColor =
               pt_displayContent.get<INT32>( CONTENTCOLOUR_FOREGROUNDCOLOR ) ;
         dHContent.contentColour.backGroundColor =
               pt_displayContent.get<INT32>( CONTENTCOLOUR_BACKGROUNDCOLOR ) ;
         dHContent.wndOptionRow = pt_displayContent.get<INT32>( WNDROW ) ;
         dHContent.wndOptionCol= pt_displayContent.get<INT32>( WNDCOLUMN ) ;
         dHContent.optionsRow = pt_displayContent.get<INT32>( OPTIONROW ) ;
         dHContent.optionsCol= pt_displayContent.get<INT32>( OPTIONCOL ) ;
         dHContent.cellLength= pt_displayContent.get<INT32>( CELLLENGTH ) ;
      }
      catch( std::exception &e )
      {
         ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
         ossSnprintf( errStr, errStrLength,"%s readDisplayContent failed"
                      "(displayType == DISPLAYTYPE_DYNAMIC_HELP),"
                      "e.what():%s"OSS_NEWLINE,
                      errStrBuf, e.what() ) ;
         rc = SDB_ERROR ;
         goto error ;
      }
   }
   else if( DISPLAYTYPE_DYNAMIC_EXPRESSION == displayType )
   {
      rc = storeDE( pt_displayContent, dEContent ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else if( DISPLAYTYPE_DYNAMIC_SNAPSHOT == displayType )
   {
      rc = storeDS( pt_displayContent, dSContent ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,"%s readDisplayContent failed,"
                   "displayType is wrong\n", errStrBuf ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 storePanelValue( ptree pt_value, Panel& value )
{
   INT32 rc             = SDB_OK ;
   INT32 numOfSubWindow = 0 ;
   try
   {
      value.numOfSubWindow = pt_value.get<INT32>( NUMOFSUBWINDOW ) ;
      value.subWindow = SDB_OSS_NEW NodeWindow[value.numOfSubWindow] ;
      numOfSubWindow = 0 ;
      for( BOOST_AUTO( child_value, pt_value.begin() );
           child_value != pt_value.end(); ++child_value )
      {
         if( child_value->first == NODEWINDOW )
         {
            if( numOfSubWindow >= value.numOfSubWindow )
            {
               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s readPanelValue failed,"
                            " numOfSubWindow >= value.numOfSubWindow\n",
                            errStrBuf ) ;
               rc = SDB_ERROR ;
               goto error ;
            }
            value.subWindow[numOfSubWindow].actualWindowMinRow =
                  child_value->second.get<INT32>( ACTUALWINDOWMINROW ) ;
            value.subWindow[numOfSubWindow].actualWindowMinColumn =
                  child_value->second.get<INT32>( ACTUALWINDOWMINCOLUMN ) ;
            value.subWindow[numOfSubWindow].zoomMode =
                  child_value->second.get<string>( ZOOMMODE ) ;
            value.subWindow[numOfSubWindow].displayType =
                  child_value->second.get<string>( DISPLAYTYPE ) ;
            try
            {
               value.subWindow[numOfSubWindow].occupyMode =
                     child_value->second.get<string>( OCCUPYMODE ) ;
            }
            catch( std::exception &e )
            {
               value.subWindow[numOfSubWindow].occupyMode = OCCUPY_MODE_NONE;
            }
            for( BOOST_AUTO( child_nodewindow, child_value->second.begin() );
                 child_nodewindow != child_value->second.end();
                 ++child_nodewindow )
            {
               if( child_nodewindow->first == DISPLAYCONTENT )
               {
                  rc =
                        storeDisplayContent(
                              child_nodewindow->second,
                              value.subWindow[numOfSubWindow].displayContent,
                              value.subWindow[numOfSubWindow].displayType ) ;
                  if( rc)
                  {
                     ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
                     ossSnprintf( errStr, errStrLength,
                                  "%s readPanelValue failed, "
                                  "can't readDisplayContent\n", errStrBuf ) ;
                     goto error ;
                  }
               }
               else if( child_nodewindow->first == POSITION )
               {
                  rc = storePosition(
                              child_nodewindow->second,
                              value.subWindow[numOfSubWindow].position ) ;
                  if( rc )
                  {
                     ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
                     ossSnprintf( errStr, errStrLength,
                                  "%s readPanelValue failed, "
                                  "can't readPosition\n", errStrBuf ) ;
                     goto error ;
                  }
               }
            }
            ++numOfSubWindow ;
         }
      }
      value.numOfSubWindow = numOfSubWindow ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,"%s readPanelValue failed,"
                   "e.what():%s\n", errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 storeHeaders( ptree pt_HDs, RootWindow &root )
{
   INT32 rc           = SDB_OK ;
   INT32 headerLength = 0 ;
   HeadTailMap *heder = NULL ;
   try
   {
      root.headerLength = pt_HDs.get<INT32>( HEADERLENGTH ) ;
      root.header = SDB_OSS_NEW HeadTailMap[root.headerLength] ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,
                   "%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed,"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   headerLength = 0 ;
   for( BOOST_AUTO( pt_header, pt_HDs.begin() );
        pt_header != pt_HDs.end();
        ++pt_header )
   {
      if( HEADTAILMAP == pt_header->first )
      {
         heder = &root.header[headerLength] ;
         if( headerLength >= root.headerLength )
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "scope: headerLength>="
                         "root.headerLength"OSS_NEWLINE,
                         errStrBuf ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         try
         {
            heder->key = pt_header->second.get<INT32>( KEY ) ;
         }
         catch( std::exception &e )
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "e.what():%s"OSS_NEWLINE,
                      errStrBuf, e.what() ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         for( BOOST_AUTO( pt_HValue, pt_header->second.begin() );
              pt_HValue != pt_header->second.end();
              ++pt_HValue )
         {
            if( VALUE == pt_HValue->first )
            {
               rc = storePanelValue( pt_HValue->second, heder->value ) ;
               if( rc )
               {
                  goto error ;
               }
            }
         }
         ++headerLength ;
      }
   }
   root.headerLength = headerLength ;
done :
   return rc ;
error :
   goto done ;
}

INT32 storeBodies( ptree pt_BDs, RootWindow &root )
{
   INT32 rc         = SDB_OK ;
   INT32 bodyLength = 0 ;
   BodyMap *body    = NULL ;
   try
   {
      root.bodyLength = pt_BDs.get<INT32>( BODYLENGTH ) ;

      root.body = SDB_OSS_NEW BodyMap[root.bodyLength] ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,
                   "%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed,"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   bodyLength = 0 ;
   for( BOOST_AUTO( pt_body, pt_BDs.begin() );
        pt_body != pt_BDs.end();
        ++pt_body )
   {
      if( pt_body->first == BODYMAP )
      {
         body = &root.body[bodyLength] ;
         if( bodyLength >= root.bodyLength )
         {
            ossSnprintf( errStrBuf, errStrLength,
                         "%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "scope: bodyLength>="
                         "root.bodyLength"OSS_NEWLINE,
                         errStrBuf ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         try
         {
            body->headerKey =
                  pt_body->
                        second.get<INT32>(
                              HEADERKEY ) ;
            body->footerKey =
                  pt_body->
                        second.get<INT32>(
                              FOOTERKEY ) ;
            body->labelName =
                  pt_body->
                        second.get<string>(
                              LABELNAME ) ;
            body->bodyPanelType =
                  pt_body->
                        second.get<string>(
                              BODYPANELTYPE ) ;
            if( BODYTYPE_MAIN   == body->bodyPanelType ||
                BODYTYPE_NORMAL == body->bodyPanelType )
            {
               body->hotKeySuiteType =
                     pt_body->
                           second.get<INT64>(
                                 HOTKEYSUITETYPE ) ;
               body->helpPanelType =
                     pt_body->
                           second.get<string>(
                                 HELPPANELTYPE ) ;
               if( BODYTYPE_NORMAL == body->bodyPanelType )
               {
                  body->sourceSnapShot =
                        pt_body->
                              second.get<string>(
                                    SOURCESNAPSHOT ) ;
               }
               else
               {
                  body->sourceSnapShot = SDB_SNAP_NULL ;
               }
            }
         }
         catch( std::exception &e )
         {
            ossSnprintf( errStrBuf, errStrLength,
                         "%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "e.what():%s"OSS_NEWLINE,
                         errStrBuf, e.what() ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         for( BOOST_AUTO( pt_BValue, pt_body->second.begin() );
              pt_BValue != pt_body->second.end();
              ++pt_BValue )
         {
            if( VALUE == pt_BValue->first )
            {
               rc = storePanelValue( pt_BValue->second,
                                      body->value ) ;
               if( rc )
               {
                  goto error ;
               }
            }
         }
         ++bodyLength ;
      }
   }
   root.bodyLength = bodyLength ;
done :
   return rc ;
error :
   goto done ;
}

INT32 storeFooters( ptree pt_FTs, RootWindow &root )
{
   INT32 rc            = SDB_OK ;
   INT32 footerLength  = 0 ;
   HeadTailMap *footer = NULL ;
   try
   {
      root.footerLength = pt_FTs.get<INT32>( FOOTERLENGTH ) ;

      root.footer = SDB_OSS_NEW HeadTailMap[root.footerLength] ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,
                   "%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed,"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   footerLength = 0 ;
   for( BOOST_AUTO( pt_footer, pt_FTs.begin() );
        pt_footer != pt_FTs.end();
        ++pt_footer )
   {
      if( HEADTAILMAP == pt_footer->first )
      {
         footer = &root.footer[footerLength] ;
         if( footerLength >= root.footerLength )
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "scope: footerLength >="
                         "root.footerLength"OSS_NEWLINE,
                         errStrBuf ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         footer->key = pt_footer->second.get<INT32>( KEY ) ;
         for( BOOST_AUTO( pt_FValue, pt_footer->second.begin() );
              pt_FValue != pt_footer->second.end();
              ++pt_FValue )
         {
            if( pt_FValue->first == VALUE )
            {
               rc = storePanelValue( pt_FValue->second,
                                     footer->value ) ;
               if( rc )
               {
                  goto error;
               }
            }
         }
         ++footerLength;
      }
   }
   root.footerLength = footerLength;
done :
   return rc ;
error :
   goto done ;
}

INT32 storeKeySuites( ptree pt_KSs, RootWindow &root )
{
   INT32 rc             = SDB_OK ;
   INT32 keySuiteLength = 0 ;
   INT32 hotKeyLength   = 0 ;
   KeySuite *keySuite   = NULL ;
   HotKey *hotkey       = NULL ;
   try
   {
      root.keySuiteLength = pt_KSs.get<INT32>( KEYSUITELENGTH ) ;

      root.keySuite =
            SDB_OSS_NEW KeySuite[root.keySuiteLength] ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed,"
                   " scope: child_root->first ==..... "
                   ",e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   keySuiteLength = 0 ;
   for( BOOST_AUTO( pt_keySuite, pt_KSs.begin() );
        pt_keySuite != pt_KSs.end();
        ++pt_keySuite )
   {
      if( KEYSUITE == pt_keySuite->first )
      {
         keySuite = &root.keySuite[keySuiteLength] ;
         if( keySuiteLength >= root.keySuiteLength )
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         " keySuiteLength >="
                         "root.keySuiteLength"OSS_NEWLINE,
                         errStrBuf ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         try
         {
            keySuite->mark = pt_keySuite->second.get<INT64>( MARK ) ;
            keySuite->hotKeyLength = pt_keySuite->second.get<INT32>(
                                           HOTKEYLENGTH ) ;
            keySuite->hotKeyLengthFromConf = keySuite->hotKeyLength ;
            keySuite->hotKey = SDB_OSS_NEW HotKey[keySuite->hotKeyLength ] ;
         }
         catch( std::exception &e)
         {
            ossSnprintf( errStrBuf, errStrLength,
                         "%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "e.what():%s\n",
                         errStrBuf, e.what() ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         hotKeyLength = 0 ;
         for( BOOST_AUTO( pt_hotkey, pt_keySuite->second.begin() );
              pt_hotkey != pt_keySuite->second.end();
              ++pt_hotkey )
         {
            if( pt_hotkey->first == HOTKEY )
            {
               hotkey = &keySuite->hotKey[hotKeyLength] ;
               if( hotKeyLength >= keySuite->hotKeyLength )
               {
                  ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
                  ossSnprintf( errStr, errStrLength,
                               "%s readConfiguration failed, "
                               "hotKeyLength >= "
                               "keySuite->hotKeyLength"
                               OSS_NEWLINE,
                               errStrBuf ) ;
                  rc = SDB_ERROR ;
                  goto error ;
               }
               try
               {
                  hotkey->button =
                        pt_hotkey->
                              second.get<CHAR>(
                                    BUTTON ) ;
                  hotkey->jumpType =
                        pt_hotkey->
                              second.get<string>(
                                    JUMPTYPE ) ;
                  hotkey->jumpName =
                        pt_hotkey->
                              second.get<string>(
                                    JUMPNAME ) ;
                  hotkey->desc = pt_hotkey->
                                 second.get<string>(KEYDESC) ;
                  string wndtype = pt_hotkey->
                                 second.get<string>(WNDTYPE) ;
                  if ( 0 == ossStrncmp( wndtype.c_str(),
                                        "true", wndtype.length() ) )
                  {
                     hotkey->wndType = TRUE ;
                  }
                  else
                  {
                     hotkey->wndType = FALSE ;
                  }
               }
               catch( std::exception &e )
               {
                  ossSnprintf( errStrBuf, errStrLength,
                               "%s", errStr ) ;
                  ossSnprintf( errStr, errStrLength,
                               "%s readConfiguration failed,%s",
                               errStrBuf, e.what() ) ;
                  rc = SDB_ERROR ;
                  goto error ;
               }
               ++hotKeyLength ;
            }
         }
         keySuite->hotKeyLength = hotKeyLength ;
         ++keySuiteLength ;
      }
   }
   root.keySuiteLength = keySuiteLength ;
done :
   return rc ;
error :
   goto done ;
}

INT32 storeRootWindow( RootWindow &root )
{
   INT32 rc = SDB_OK ;
   ptree pt_sdbtopXML ;
   ptree pt_Event ;
   root.input.confPath = confPath ;
   InputPanel &input = root.input ;
   try
   {
      read_xml( root.input.confPath, pt_sdbtopXML ) ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed,"
                   "check configuration file %s is exist,"
                   "e.what():%s\n",
                   errStrBuf,
                   root.input.confPath.c_str(),
                   e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   try
   {
      pt_Event = pt_sdbtopXML.get_child( EVENT ) ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,
                   "%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed,"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   for( BOOST_AUTO( child_event, pt_Event.begin() );
        child_event != pt_Event.end(); ++child_event )
   {
      if( child_event->first == ROOTWINDOW )
      {
         try
         {
            root.referWindowRow =
                  child_event->
                        second.get<INT32>(
                              REFERWINDOWROW ) ;
            root.referWindowColumn =
                  child_event->
                        second.get<INT32>(
                              REFERWINDOWCOLUMN ) ;
            root.actualWindowMinRow =
                  child_event->
                        second.get<INT32>(
                              ACTUALWINDOWMINROW );
            root.actualWindowMinColumn =
                  child_event->
                        second.get<INT32>(
                              ACTUALWINDOWMINCOLUMN ) ;
            input.refreshInterval =
                  child_event->
                        second.get<INT32>(
                              REFRESHINTERVAL ) ;
            input.colourOfTheDividingLine.foreGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHEDIVIDINGLINE_FOREGROUNDCOLOR ) ;
            input.colourOfTheDividingLine.backGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHEDIVIDINGLINE_BACKGROUNDCOLOR ) ;
            input.colourOfTheChange.foreGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHECHANGE_FOREGROUNDCOLOR ) ;
            input.colourOfTheChange.backGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHECHANGE_BACKGROUNDCOLOR ) ;
            input.colourOfTheMax.foreGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHEMAX_FOREGROUNDCOLOR ) ;
            input.colourOfTheMax.backGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHEMAX_BACKGROUNDCOLOR ) ;
            input.colourOfTheMin.foreGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHEMIN_FOREGROUNDCOLOR ) ;
            input.colourOfTheMin.backGroundColor =
                  child_event->
                        second.get<INT32>(
                              COLOUROFTHEMIN_BACKGROUNDCOLOR ) ;
         }
         catch( std::exception &e )
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s readConfiguration failed,"
                         "e.what():%s"OSS_NEWLINE,
                         errStrBuf, e.what() ) ;
            rc = SDB_ERROR ;
            goto error ;
         }
         for( BOOST_AUTO( child_root, child_event->second.begin() );
              child_root != child_event->second.end();
              ++child_root )
         {
            if( KEYSUITES == child_root->first )
            {
               rc = storeKeySuites( child_root->second, root ) ;
               if( rc )
               {
                  goto error ;
               }
            }
            else if( HEADERS == child_root->first )
            {
               rc = storeHeaders( child_root->second, root ) ;
               if( rc )
               {
                  goto error ;
               }
            }
            else if( child_root->first == BODIES )
            {
               rc = storeBodies( child_root->second, root ) ;
               if( rc )
               {
                  goto error ;
               }
            }
            else if( child_root->first == FOOTERS )
            {
               rc = storeFooters( child_root->second, root ) ;
               if( rc )
               {
                  goto error ;
               }
            }
         }
      }
   }
done :
   return rc ;
error :
   goto done ;
}

Event::Event(): coord( NULL )
{
   root.input.activatedPanel                 = NULL ;
   root.input.displayModeChooser             = 0 ;
   root.input.sortingWay                     = NULLSTRING;
   root.input.sortingField                   = NULLSTRING ;
   root.input.filterCondition                = NULLSTRING ;
   root.input.filterNumber                   = 0 ;
   root.input.snapshotModeChooser            = GLOBAL ;
   root.input.forcedToRefresh_Global         = NOTREFRESH ;
   root.input.forcedToRefresh_Global         = NOTREFRESH ;
   root.input.fieldPosition                  = 0 ;
   root.input.groupName                      = NULLSTRING ;
   root.input.nodeName                       = NULLSTRING ;
   root.input.confPath                       = SDBTOP_DEFAULT_CONFPATH ;
   root.input.refreshInterval                = 0;
   root.input.isFirstGetSnapshot             = TRUE ;
   root.input.colourOfTheMax.foreGroundColor = COLOR_BLACK ;
   root.input.colourOfTheMax.backGroundColor = COLOR_WHITE ;
   root.input.colourOfTheMin.foreGroundColor = COLOR_BLACK ;
   root.input.colourOfTheMin.backGroundColor = COLOR_WHITE ;
   root.keySuiteLength                       = 0 ;
   root.headerLength                         = 0 ;
   root.bodyLength                           = 0 ;
   root.footerLength                         = 0;
   root.input.last_Snapshot.clear() ;
   root.input.cur_Snapshot.clear() ;
   root.input.last_absoluteMap.clear() ;
   root.input.last_averageMap.clear() ;
   root.input.last_deltaMap.clear() ;
   root.input.cur_absoluteMap.clear() ;
   root.input.cur_averageMap.clear() ;
   root.input.cur_deltaMap.clear() ;
}

Event::~Event()
{
   INT32 headerLength   = root.headerLength ;
   INT32 bodyLength     = root.bodyLength ;
   INT32 footerLength   = root.footerLength ;
   INT32 keySuiteLength = root.keySuiteLength;
   INT32 numOfSubWindow = 0 ;
   HeadTailMap *header  = NULL ;
   BodyMap *body        = NULL ;
   HeadTailMap *footer  = NULL ;
   NodeWindow *window   = NULL ;

   while( keySuiteLength ) // free the memory root.keySuite
   {
      SDBTOP_SAFE_DELETE( root.keySuite[keySuiteLength -1].hotKey ) ;
      --keySuiteLength ;
   }
   if( root.keySuiteLength )
      SDB_OSS_DEL []root.keySuite ;

   while( headerLength ) // free the memory root.header
   {
      header = &root.header[headerLength - 1] ;
      numOfSubWindow = header->value.numOfSubWindow;
      while( numOfSubWindow )
      {
         window = &header->value.subWindow[numOfSubWindow - 1] ;
         if( DISPLAYTYPE_DYNAMIC_EXPRESSION ==
               window->displayType )
         {
            SDBTOP_SAFE_DELETE( window->displayContent.dyExOutPut.content ) ;
         }
         else if( DISPLAYTYPE_DYNAMIC_SNAPSHOT ==
                  window->displayType )
         {
            SDBTOP_SAFE_DELETE(
                  window->displayContent.dySnapshotOutPut.fixedField ) ;
            SDBTOP_SAFE_DELETE(
                  window->displayContent.dySnapshotOutPut.mobileField ) ;
         }
         --numOfSubWindow ;
      }
      SDBTOP_SAFE_DELETE( header->value.subWindow ) ;
      --headerLength ;
   }
   if( root.headerLength )
      SDB_OSS_DEL []root.header;

   while( bodyLength ) // free the memory root.body
   {
      body = &root.body[bodyLength - 1] ;
      numOfSubWindow = body->value.numOfSubWindow ;
      while( numOfSubWindow )
      {
         window = &body->value.subWindow[numOfSubWindow - 1] ;
         if( DISPLAYTYPE_DYNAMIC_EXPRESSION ==
             window->displayType )
         {
            SDBTOP_SAFE_DELETE( window->displayContent.dyExOutPut.content ) ;
         }
         else if( DISPLAYTYPE_DYNAMIC_SNAPSHOT ==
                  window->displayType )
         {
            SDBTOP_SAFE_DELETE(
                  window->displayContent.dySnapshotOutPut.fixedField ) ;
            SDBTOP_SAFE_DELETE(
                  window->displayContent.dySnapshotOutPut.mobileField ) ;
         }
         --numOfSubWindow ;
      }
      SDBTOP_SAFE_DELETE( body->value.subWindow ) ;
      --bodyLength ;
   }
   if( root.bodyLength )
      SDBTOP_SAFE_DELETE( root.body ) ;

   while( footerLength ) // free the memory root.footer
   {
      footer = &root.footer[footerLength - 1] ;
      numOfSubWindow = footer->value.numOfSubWindow ;
      while( numOfSubWindow )
      {
         window = &footer->value.subWindow[numOfSubWindow - 1] ;
         if( DISPLAYTYPE_DYNAMIC_EXPRESSION ==
             window->displayType )
         {
            SDBTOP_SAFE_DELETE( window->displayContent.dyExOutPut.content ) ;
         }
         else if( DISPLAYTYPE_DYNAMIC_SNAPSHOT ==
                  window->displayType )
         {
            SDBTOP_SAFE_DELETE(
                  window->displayContent.dySnapshotOutPut.fixedField ) ;
            SDBTOP_SAFE_DELETE(
                  window->displayContent.dySnapshotOutPut.mobileField ) ;
         }
         --numOfSubWindow ;
      }
      SDBTOP_SAFE_DELETE( footer->value.subWindow ) ;
      --footerLength ;
   }
   if( root.footerLength )
      SDBTOP_SAFE_DELETE( root.footer ) ;

   SAFE_OSS_DELETE( coord ) ;
}

INT32 Event::assignActivatedPanel( BodyMap **activatedPanel,
                                   string bodyPanelType )
{
   INT32 rc        = SDB_OK ;
   INT32 i         = 0 ;
   *activatedPanel = NULL ;
   for( i = 0; i < root.bodyLength; ++i )
   {
      if( root.body[i].bodyPanelType == bodyPanelType )
      {
         *activatedPanel = &root.body[i] ;
         break ;
      }
   }
   if( NULL == *activatedPanel )
   {
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::assignActivatedPanelByLabelName( BodyMap **activatedPanel,
                                              string labelName )
{
   INT32 rc = SDB_OK ;
   INT32 i  = 0 ;
   for( i = 0; i < root.bodyLength; ++i )
   {
      if( root.body[i].labelName== labelName )
      {
         *activatedPanel = &root.body[i] ;
         break ;
      }
   }
   if( NULL == *activatedPanel )
   {
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   root.input.fieldPosition = 0 ;
   return rc ;
error :
   goto done ;
}

INT32 Event::getActivatedHeadTailMap( BodyMap *activatedPanel,
                                      HeadTailMap **header,
                                      HeadTailMap **footer )
{
   INT32 rc           = SDB_OK ;
   *header            = NULL ;
   *footer            = NULL ;
   INT32 index_header = 0 ;
   INT32 index_footer = 0 ;
   for( index_header = 0; index_header < root.headerLength; ++index_header )
   {
      if( root.header[index_header].key == activatedPanel->headerKey )
      {
         *header = &root.header[index_header] ;
         break ;
      }
   }
   for( index_footer = 0; index_footer < root.footerLength; ++index_footer )
   {
      if( root.footer[index_footer].key == activatedPanel->footerKey )
      {
         *footer = &root.footer[index_footer] ;
         break ;
      }
   }
   if( NULL == *header )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getActivatedHeadTailMap failed,"
                   "SDB_HEADER_NULL"OSS_NEWLINE,
                   errStrBuf ) ;
      rc = SDB_ERROR;
      goto error ;
   }
   if( NULL == *footer )
   {
      if( FOOTER_NULL != activatedPanel->footerKey )
      {
         if( NULL == *header )
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s getActivatedHeadTailMap failed, "
                         "SDB_HEADER_FOOTER_NULL\n", errStrBuf ) ;
            rc = SDB_ERROR;
            goto error ;
         }
         else
         {
            ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
            ossSnprintf( errStr, errStrLength,
                         "%s getActivatedHeadTailMap failed, "
                         "SDB_FOOTER_NULL\n", errStrBuf ) ;
            rc = SDB_ERROR;
            goto error ;
         }
      }
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::getActualPosition( Position &actualPosition,
                                Position &referPosition,
                                const string zoomMode,
                                const string occupyMode )
{
   INT32 rc          = SDB_OK ;
   INT32 row         = 0 ;
   INT32 col         = 0 ;
   FLOAT32 SCALE_ROW = 0.0f ;
   getmaxyx( stdscr, row, col ) ;
   if( row < root.actualWindowMinRow || col < root.actualWindowMinColumn )
   {
      rc = SDB_ERROR ;
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s Minimum window size:"
                   "%dx%d, found %dx%d"OSS_NEWLINE,
                   errStrBuf, root.actualWindowMinRow,
                   root.actualWindowMinColumn, row, col ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   SCALE_ROW = (FLOAT32)row / (FLOAT32)root.referWindowRow ;
   if( zoomMode == ZOOM_MODE_ALL )
   {
      actualPosition.length_X =
            ( INT32 )( referPosition.length_X * (FLOAT32)col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.length_Y =
            ( INT32 )( referPosition.length_Y * (FLOAT32)row /
            ( FLOAT32 )root.referWindowRow ) ;
      actualPosition.referUpperLeft_X =
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn) ;
      actualPosition.referUpperLeft_Y =
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32)root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_NONE )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X = referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y = referPosition.referUpperLeft_Y ;
   }
   else if( zoomMode == ZOOM_MODE_POS )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X =
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.referUpperLeft_Y =
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_ROW_POS )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y =
            ( INT32 )( referPosition.length_Y * SCALE_ROW ) ;
      actualPosition.referUpperLeft_X =
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.referUpperLeft_Y =
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32)root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_COL_POS )
   {
      actualPosition.length_X =
            ( INT32 )( referPosition.length_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X=
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.referUpperLeft_Y=
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_POS_X )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X=
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            (FLOAT32)root.referWindowColumn ) ;
      actualPosition.referUpperLeft_Y= referPosition.referUpperLeft_Y ;
   }
   else if( zoomMode == ZOOM_MODE_ROW_POS_X )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y =
            ( INT32 )( referPosition.length_Y * SCALE_ROW ) ;
      actualPosition.referUpperLeft_X=
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.referUpperLeft_Y= referPosition.referUpperLeft_Y ;
   }
   else if( zoomMode == ZOOM_MODE_COL_POS_X )
   {
      actualPosition.length_X =
            ( INT32 )( referPosition.length_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X =
            ( INT32 )( referPosition.referUpperLeft_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.referUpperLeft_Y = referPosition.referUpperLeft_Y ;
   }
   else if( zoomMode == ZOOM_MODE_POS_Y )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X= referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y=
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_ROW_POS_Y )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y =
            ( INT32 )( referPosition.length_Y * SCALE_ROW ) ;
      actualPosition.referUpperLeft_X= referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y=
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_COL_POS_Y )
   {
      actualPosition.length_X =
            ( INT32 )( referPosition.length_X * (FLOAT32)col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X = referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y =
            ( INT32 )( referPosition.referUpperLeft_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
   }
   else if( zoomMode == ZOOM_MODE_ROW_COL )
   {
      actualPosition.length_X =
            ( INT32 )( referPosition.length_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.length_Y =
            ( INT32 )( referPosition.length_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
      actualPosition.referUpperLeft_X = referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y = referPosition.referUpperLeft_Y ;
   }
   else if( zoomMode == ZOOM_MODE_COL )
   {
      actualPosition.length_X =
            ( INT32 )( referPosition.length_X * ( FLOAT32 )col /
            ( FLOAT32 )root.referWindowColumn ) ;
      actualPosition.length_Y = referPosition.length_Y ;
      actualPosition.referUpperLeft_X = referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y = referPosition.referUpperLeft_Y ;
   }
   else if( zoomMode == ZOOM_MODE_ROW )
   {
      actualPosition.length_X = referPosition.length_X ;
      actualPosition.length_Y =
            ( INT32 )( referPosition.length_Y * ( FLOAT32 )row /
            ( FLOAT32 )root.referWindowRow ) ;
      actualPosition.referUpperLeft_X = referPosition.referUpperLeft_X ;
      actualPosition.referUpperLeft_Y = referPosition.referUpperLeft_Y ;
   }
   else
   {
      rc = SDB_ERROR ;
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                "%s getActualPosition failed:"
                "wrong zoomMode:%s"OSS_NEWLINE,
                errStrBuf, zoomMode.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   if( occupyMode != OCCUPY_MODE_NONE )
   {
      if( occupyMode == OCCUPY_MODE_WINDOW_BELOW )
      {
         actualPosition.length_Y = row - actualPosition.referUpperLeft_Y ;
      }
      else
      {
         rc = SDB_ERROR ;
         ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
         ossSnprintf( errStr, errStrLength,
                      "%s getActualPosition failed:"
                      "wrong occupyMode:%s"OSS_NEWLINE,
                      errStrBuf, occupyMode.c_str() ) ;
         rc = SDB_ERROR ;
         goto error ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}


INT32 Event::getActivatedKeySuite( KeySuite **keySuite )
{
   INT32 rc = SDB_OK ;
   INT32 i  = 0 ;
   try
   {
      for( i = 0; i < root.keySuiteLength; ++i )
      {
         if( root.keySuite[i].mark ==
             root.input.activatedPanel->hotKeySuiteType )
         {
            break ;
         }
      }
      if( i != root.keySuiteLength )
      {
         keySuite[0] = root.keySuite ;
         goto done ;
      }
      else
      {
         *keySuite = NULL ;
         rc = SDB_ERROR ;
         goto error ;
      }
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getActivatedKeySuite failed,"
                   "e.what():%s"OSS_NEWLINE,
                   errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::mvprintw_SDBTOP( const string &expression, INT32 expressionLength,
                              const string &alignment, INT32 start_row,
                              INT32 start_col )
{
   INT32 rc           = SDB_OK ;

   rc = formattingOutput( sdbtopBuffer, expressionLength, expression.c_str() ) ;
   if( rc )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s MVPRINTW_TOP failed,"
                   "SNPRINTF_TOP failed"OSS_NEWLINE,
                   errStrBuf ) ;
      goto error ;
   }
   rc = MVPRINTW( start_row, start_col, expressionLength, sdbtopBuffer,
                  alignment ) ;
   if( rc )
   {
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::mvprintw_SDBTOP( const char *expression, INT32 expressionLength,
                              const string &alignment, INT32 start_row,
                              INT32 start_col )
{
   INT32 rc           = SDB_OK ;

   rc = formattingOutput( sdbtopBuffer, expressionLength, expression ) ;
   if( rc )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s MVPRINTW_TOP failed,"
                   "SNPRINTF_TOP failed"OSS_NEWLINE,
                   errStrBuf ) ;
      goto error ;
   }
   rc = MVPRINTW( start_row, start_col, expressionLength, sdbtopBuffer,
                  alignment ) ;
   if( rc )
   {
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

void Event::getColourPN( Colours colour, INT32 &colourPairNumber )
{
   colourPairNumber = colour.foreGroundColor + colour.backGroundColor *
                                               COLOR_MULTIPLE ;
}

INT32 Event::getResultFromBSONObj( const BSONObj &bsonobj,
                                   const string &sourceField,
                                   const string &displayMode,
                                   string &result, BOOLEAN canSwitch,
                                   const string &baseField,
                                   const FiledWarningValue &waringValue,
                                   INT32 &colourPairNumber )
{
   INT32 rc                          = SDB_OK ;
   UINT32 pos_last                   = 0 ;
   const char *sourceFieldbuf        = sourceField.c_str();
   const char *baseFieldbuf          = baseField.c_str();
   InputPanel &input                 = root.input ;
   const INT64 absoluteMaxLimitValue = waringValue.absoluteMaxLimitValue ;
   const INT64 absoluteMinLimitValue = waringValue.absoluteMinLimitValue ;
   const INT64 averageMaxLimitValue  = waringValue.averageMaxLimitValue ;
   const INT64 averageMinLimitValue  = waringValue.averageMinLimitValue ;
   const INT64 deltaMaxLimitValue    = waringValue.deltaMaxLimitValue ;
   const INT64 deltaMinLimitValue    = waringValue.deltaMinLimitValue ;
   FLOAT64 elementDouble             = 0.0f ;
   BSONElement element ;
   BSONElement last_element ;
   BSONElement baseElement  ;
   BSONElement baseElement_last ;
   string new_                       = NULLSTRING ;
   string old_                       = NULLSTRING ;
   INT32 maxPairNumber               = 0 ;
   INT32 minPairNumber               = 0 ;
   INT32 changePairNumber            = 0 ;
   getColourPN( input.colourOfTheMax, maxPairNumber ) ;
   getColourPN( input.colourOfTheMin, minPairNumber ) ;
   getColourPN( input.colourOfTheChange, changePairNumber ) ;
   try
   {
      element = bsonobj.getFieldDotted( sourceFieldbuf ) ;
      if( !canSwitch )
      {
         if( element.isNumber() )
         {
            elementDouble = element.Number() ;
            if( NumberLong == element.type() || NumberInt == element.type() )
            {
               result = element.toString( FALSE ) ;
            }
            else
            {
               ossSnprintf( sdbtopBuffer, BUFFERSIZE,
                            OUTPUT_FORMATTING, elementDouble ) ;
               result = sdbtopBuffer ;
            }
            if( absoluteMaxLimitValue != 0 &&
                elementDouble > absoluteMaxLimitValue )
            {
               colourPairNumber = maxPairNumber ;
            }
            else if( absoluteMinLimitValue != 0 &&
                     elementDouble < absoluteMinLimitValue )
            {
               colourPairNumber = minPairNumber ;
            }
         }
         else
         {
            result = element.toString( FALSE ) ;
         }
         input.cur_absoluteMap[new_+sourceField] = result ;
      }
      else
      {
         baseElement = bsonobj.getFieldDotted( baseFieldbuf ) ;
         new_ = baseElement.toString( FALSE ) ;
         while( pos_last < input.last_Snapshot.size() )
         {
            baseElement_last =
                  input.last_Snapshot[pos_last].getFieldDotted(
                        baseFieldbuf ) ;
            old_ = baseElement_last.toString( FALSE ) ;
            if( new_ == old_ )
               break ;
            ++pos_last ;
         }
         if( pos_last == input.last_Snapshot.size() )
         {
            if( DELTA == displayMode || AVERAGE == displayMode )
            {
               ossSnprintf( sdbtopBuffer, BUFFERSIZE, "%d", 0 ) ;
               result = sdbtopBuffer ;
               input.cur_deltaMap[new_+sourceField] = sdbtopBuffer ;
               input.cur_averageMap[new_+sourceField] = sdbtopBuffer ;
            }
            else if( ABSOLUTE == displayMode )
            {
               if( element.isNumber() )
               {
                  elementDouble = element.Number() ;
                  if( NumberLong == element.type() ||
                      NumberInt == element.type() )
                  {
                     result = element.toString( FALSE ) ;
                  }
                  else
                  {
                     ossSnprintf( sdbtopBuffer, BUFFERSIZE,
                                  OUTPUT_FORMATTING, elementDouble ) ;
                     result = sdbtopBuffer ;
                  }
                  if( absoluteMaxLimitValue != 0 &&
                      elementDouble > absoluteMaxLimitValue )
                  {
                     colourPairNumber = maxPairNumber ;
                  }
                  else if( absoluteMinLimitValue != 0 &&
                           elementDouble < absoluteMinLimitValue )
                  {
                     colourPairNumber = minPairNumber ;
                  }
               }
               else
               {
                  result = element.toString( FALSE ) ;
               }
               input.cur_absoluteMap[new_+sourceField] = result ;
            }
            else
            {
               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s getResultFromBSONobj failed,"
                            "displayMode = %s"OSS_NEWLINE,
                            errStrBuf, displayMode.c_str() ) ;
               rc = SDB_ERROR ;
               goto error ;
            }
         }
         else
         {
            last_element =
                  input.last_Snapshot[pos_last].getFieldDotted(
                        sourceFieldbuf ) ;
            if( DELTA == displayMode )
            {
               if( element.isNumber() )
               {
                  elementDouble = element.Number() - last_element.Number() ;
                  if( element.type() == NumberInt ||
                      element.type() == NumberLong )
                  {
                     ossSnprintf( sdbtopBuffer, BUFFERSIZE, "%d",
                                  element.Long() - last_element.Long() ) ;
                  }
                  else
                  {
                     ossSnprintf( sdbtopBuffer, BUFFERSIZE,
                                  OUTPUT_FORMATTING, elementDouble ) ;
                  }
                  result = sdbtopBuffer ;
                  if( deltaMaxLimitValue != 0 &&
                      elementDouble > deltaMaxLimitValue )
                  {
                     colourPairNumber = maxPairNumber ;
                  }
                  else if( deltaMinLimitValue!= 0 &&
                           elementDouble < deltaMinLimitValue )
                  {
                     colourPairNumber = minPairNumber ;
                  }
                  else if( isExist( input.last_deltaMap, new_+sourceField ) &&
                           result != input.last_deltaMap[new_+sourceField] )
                  {
                     colourPairNumber = changePairNumber ;
                  }
               }
               else
               {
                  result = element.toString( FALSE ) ;
               }
               input.cur_deltaMap[new_+sourceField] = result ;
            }
            else if( ABSOLUTE == displayMode )
            {
               if( element.isNumber() )
               {
                  elementDouble = element.Number() ;
                  if( NumberLong == element.type() ||
                      NumberInt == element.type() )
                  {
                     result = element.toString( FALSE ) ;
                  }
                  else
                  {
                     ossSnprintf( sdbtopBuffer, BUFFERSIZE,
                                  OUTPUT_FORMATTING, elementDouble ) ;
                     result = sdbtopBuffer ;
                  }
                  if( absoluteMaxLimitValue != 0 &&
                      elementDouble > absoluteMaxLimitValue )
                  {
                     colourPairNumber = maxPairNumber ;
                  }
                  else if( absoluteMinLimitValue != 0 &&
                           elementDouble < absoluteMinLimitValue )
                  {
                     colourPairNumber = minPairNumber ;
                  }
                  else if(
                        isExist( input.last_absoluteMap, new_+sourceField ) &&
                           result != input.last_absoluteMap[new_+sourceField] )
                  {
                     colourPairNumber = changePairNumber ;
                  }
               }
               else
               {
                  result = element.toString( FALSE ) ;
               }
               input.cur_absoluteMap[new_+sourceField] = result ;
            }
            else if( AVERAGE == displayMode )
            {
               if( element.isNumber() )
               {
                  elementDouble =
                        ( element.Number() - last_element.Number() ) /
                         root.input.refreshInterval ;
                  ossSnprintf( sdbtopBuffer, BUFFERSIZE,
                               OUTPUT_FORMATTING, elementDouble ) ;
                  result = sdbtopBuffer ;
                  if( averageMaxLimitValue!= 0 &&
                      elementDouble > averageMaxLimitValue )
                  {
                     colourPairNumber = maxPairNumber ;
                  }
                  else if( averageMinLimitValue!= 0 &&
                           elementDouble < averageMinLimitValue )
                  {
                     colourPairNumber = minPairNumber ;
                  }
                  else if(
                        isExist( input.last_averageMap, new_+sourceField ) &&
                        result != input.last_averageMap[new_+sourceField] )
                  {
                     colourPairNumber = changePairNumber ;
                  }
               }
               else
               {
                  result = element.toString( FALSE ) ;
               }
               input.cur_averageMap[new_+sourceField] = result ;
            }
            else
            {
               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s getResultFromBSONobj failed,"
                            "displayMode = %s"OSS_NEWLINE,
                            errStrBuf, displayMode.c_str() ) ;
               rc = SDB_ERROR ;
               goto error ;
            }
         }
      }
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getResultFromBSONobj failed, e.what():%s ,"
                   "sourceField = %s"OSS_NEWLINE,
                   errStrBuf, e.what(), sourceField.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }

   doubleQuotesTrim( result ) ;
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::getExpression( string& expression, string& result )
{
   INT32 rc  = SDB_OK ;
   if( EXPRESSION_BODY_LABELNAME == expression )
   {
      result = root.input.activatedPanel->labelName ;
   }
   else if( EXPRESSION_REFRESH_QUIT_HELP == expression )
   {
      result = SDBTOP_REFRESH_QUIT_HELP ;
   }
   else if( EXPRESSION_VERSION == expression )
   {
      INT32 version = 0 ;
      INT32 subVersion = 0 ;
      INT32 fixedVersion = 0 ;
      INT32 release = 0 ;
      const CHAR *pBuild = NULL ;
      CHAR strVersion[BUFFERSIZE] = { 0 } ;

      ossGetVersion( &version, &subVersion,
                     &fixedVersion, &release, &pBuild ) ;

      ossSnprintf( strVersion, BUFFERSIZE, "version %d.%d.%d",
                   version, subVersion, fixedVersion ) ;
      result = strVersion ;
   }
   else if( EXPRESSION_REFRESH_TIME == expression )
   {
      ossSnprintf( sdbtopBuffer, BUFFERSIZE, "%d", root.input.refreshInterval ) ;
      result = sdbtopBuffer ;
   }
   else if( EXPRESSION_HOSTNAME == expression )
   {
      result = hostname ;
   }
   else if( EXPRESSION_SERVICENAME == expression )
   {
      result = serviceName ;
   }
   else if( EXPRESSION_USRNAME == expression )
   {
      if( NULLSTRING == usrName )
      {
         result = STRING_NULL ;
      }
      else
      {
         result = usrName ;
      }
   }
   else if( EXPRESSION_FILTER_NUMBER == expression )
   {
      ossSnprintf( sdbtopBuffer, BUFFERSIZE, "%d", root.input.filterNumber ) ;
      result = sdbtopBuffer ;
   }
   else if( EXPRESSION_SORTINGWAY == expression )
   {
      result = root.input.sortingWay ;
   }
   else if( EXPRESSION_SORTINGFIELD == expression )
   {
      result = root.input.sortingField ;
   }
   else if( EXPRESSION_SNAPSHOTMODE_INPUTNAME == expression )
   {
      if( GLOBAL == root.input.snapshotModeChooser )
         result = STRING_NULL ;
      else if( GROUP == root.input.snapshotModeChooser )
         result = root.input.groupName ;
      else if( NODE == root.input.snapshotModeChooser )
         result = root.input.nodeName ;
      else
         result = STRING_NULL ;
   }
   else if( EXPRESSION_DISPLAYMODE == expression )
   {
      result = DISPLAYMODECHOOSER[root.input.displayModeChooser] ;
   }
   else if( EXPRESSION_SNAPSHOTMODE == expression )
   {
      result = root.input.snapshotModeChooser ;
   }
   else
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getExpression failed,"
                   "wrong expression:%s"OSS_NEWLINE,
                   errStrBuf, expression.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::getCurSnapshot()
{
   INT32 rc             = SDB_OK ;
   INT32 snapType       = -1 ;
   INT32 filterNum      = 0 ;
   string condition     = "{}" ;
   string selector      = "{}" ;
   string orderBy       = "{" ;
   string conditionPre  = NULLSTRING ;
   string conditionCon  = NULLSTRING ;
   string HostName      = NULLSTRING ;
   string svcname       = NULLSTRING ;
   sdbCursor cursor ;
   BSONObj bsonobj ;
   BSONObj conditionObj ;
   BSONObj selectorObj ;
   BSONObj orderByObj ;
   string::size_type pos ;
   fromjson( selector, selectorObj ) ;
   if( root.input.activatedPanel[0].sourceSnapShot ==
            SDB_SNAP_CONTEXTS_TOP )
   {
      snapType = SDB_SNAP_CONTEXTS ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_CONTEXTS_CURRENT_TOP )
   {
      snapType = SDB_SNAP_CONTEXTS_CURRENT ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_SESSIONS_TOP )
   {
      snapType = SDB_SNAP_SESSIONS ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_SESSIONS_CURRENT_TOP )
   {
      snapType = SDB_SNAP_SESSIONS_CURRENT ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_COLLECTIONS_TOP )
   {
      snapType = SDB_SNAP_COLLECTIONS ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_COLLECTIONSPACES_TOP )
   {
      snapType = SDB_SNAP_COLLECTIONSPACES ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_DATABASE_TOP )
   {
      snapType = SDB_SNAP_DATABASE ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_SYSTEM_TOP )
   {
      snapType = SDB_SNAP_SYSTEM ;
   }
   else if( root.input.activatedPanel[0].sourceSnapShot ==
                  SDB_SNAP_CATALOG_TOP )
   {
      snapType = SDB_SNAP_CATALOG ;
   }
   else
   {
      if( root.input.activatedPanel[0].bodyPanelType != BODYTYPE_NORMAL )
      {
         goto done ;
      }

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr );
      ossSnprintf( errStr, errStrLength,
                   "%s getCurSnapshot failed,"
                   "xml gave the wrong sourceSnapShot: %s"
                   OSS_NEWLINE,
                   errStrBuf,
                   root.input.activatedPanel[0].sourceSnapShot.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   if( NULLSTRING == root.input.sortingWay )
   {
      orderBy += "}" ;
   }
   else if( SORTINGWAY_ASC == root.input.sortingWay ||
            SORTINGWAY_DESC == root.input.sortingWay )
   {
      if( NULLSTRING != root.input.sortingField )
         orderBy += "\"" + root.input.sortingField + "\":" +
                    root.input.sortingWay + "}" ;
      else
         orderBy += "}" ;
   }
   else
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getCurSnapshot failed,"
                   "wrong sortingWay: %s"OSS_NEWLINE,
                   errStrBuf, root.input.sortingWay.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   fromjson( orderBy, orderByObj ) ;

   if( NULLSTRING != root.input.filterCondition )
   {
      pos = root.input.filterCondition.find( ":" ) ;
      if( string::npos != pos )
      {
         conditionPre = root.input.filterCondition.substr( 0, pos ) ;
         conditionCon = root.input.filterCondition.substr( pos + 1 ) ;
      }
      else
      {
         root.input.filterCondition = NULLSTRING;
      }
   }
   if( GLOBAL == root.input.snapshotModeChooser )
   {
      if( NULLSTRING != root.input.filterCondition )
      {
         condition = "{" + conditionPre + ":" + conditionCon + "}" ;
      }
      else
         condition = "{}" ;
      fromjson(condition, conditionObj ) ;
      rc = coord->getSnapshot( cursor, snapType,
                              conditionObj, selectorObj, orderByObj) ;
   }
   else if( GROUP == root.input.snapshotModeChooser )
   {
      condition = "{GroupName:\""  + root.input.groupName ;
      if( NULLSTRING != root.input.filterCondition )
      {
         condition += "\"," + conditionPre + ":" + conditionCon + "}" ;
      }
      else
         condition += "\"}" ;

      fromjson(condition, conditionObj ) ;
      rc = coord->getSnapshot( cursor, snapType,
                              conditionObj, selectorObj, orderByObj) ;
   }
   else if( NODE == root.input.snapshotModeChooser )
   {
      condition = "{HostName:\"" ;
      pos = root.input.nodeName.find( ":" ) ;
      if( string::npos != pos )
      {
         HostName = root.input.nodeName.substr( 0, pos ) ;
         svcname = root.input.nodeName.substr( pos + 1 ) ;
         condition += HostName + "\",svcname:\"" + svcname + "\"" ;
      }
      else
      {
         HostName = root.input.nodeName ;
         condition += HostName + "\",svcname:\" \"" ;
      }
      if( NULLSTRING != root.input.filterCondition )
      {
         condition += "," + conditionPre + ":" + conditionCon + "}" ;
      }
      else
         condition += "}" ;
      fromjson(condition, conditionObj ) ;
      rc = coord->getSnapshot( cursor, snapType,
                              conditionObj, selectorObj, orderByObj) ;
   }
   else
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getCurSnapshot failed,"
                   "wrong snapshotModeChooser = %s"OSS_NEWLINE,
                   errStrBuf, root.input.snapshotModeChooser.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   if( rc )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getCurSnapshot failed,can't getSnapshot,"
                   "rc = %d"OSS_NEWLINE,
                   errStrBuf, rc ) ;
      goto error ;
   }

   root.input.last_absoluteMap.clear() ;
   root.input.last_absoluteMap = root.input.cur_absoluteMap ;
   root.input.cur_absoluteMap.clear() ;

   root.input.last_deltaMap.clear() ;
   root.input.last_deltaMap = root.input.cur_deltaMap ;
   root.input.cur_deltaMap.clear() ;

   root.input.last_averageMap.clear() ;
   root.input.last_averageMap = root.input.cur_averageMap ;
   root.input.cur_averageMap.clear() ;

   root.input.last_Snapshot.clear() ;
   root.input.last_Snapshot = root.input.cur_Snapshot ;
   root.input.cur_Snapshot.clear() ;
   filterNum = root.input.filterNumber ;
   while( !( rc = cursor.next( bsonobj ) ) )
   {
      if( 0 < filterNum )
      {
         --filterNum ;
         continue ;
      }
      root.input.cur_Snapshot.push_back( bsonobj ) ;
   }
   if( SDB_DMS_EOC != rc && SDB_OK != rc )
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s refreshDisplayContent failed, "
                   "snapShotCursor.next( bsonobj ) failed,"
                   "rc = %d"OSS_NEWLINE,
                   errStrBuf, rc ) ;
      goto error ;
   }
   if( SDB_DMS_EOC == rc )
   {
      rc = SDB_OK ;
   }
   if( TRUE == root.input.isFirstGetSnapshot )
   {
      root.input.isFirstGetSnapshot = FALSE ;
      root.input.last_Snapshot.clear() ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::fixedOutputLocation( INT32 start_row, INT32 start_col,
                                  INT32 &fixed_row, INT32 &fixed_col,
                                  INT32 referRowLength, INT32 referColLength,
                                  const string &autoSetType )
{
   INT32 rc  = SDB_OK ;
   fixed_row = start_row ;
   fixed_col = start_col ;
   if( autoSetType == UPPER_LEFT )
   {
      fixed_row = start_row ;
      fixed_col = start_col ;
   }
   else if( autoSetType == MIDDLE_LEFT )
   {
      fixed_row = start_row + referRowLength / 2 ;
      fixed_col = start_col ;
   }
   else if( autoSetType == LOWER_LEFT )
   {
      fixed_row = start_row + referRowLength ;
      fixed_col = start_col ;
   }
   else if( autoSetType == UPPER_MIDDLE )
   {
      fixed_row = start_row ;
      fixed_col = start_col + referColLength / 2 ;
   }
   else if( autoSetType == MIDDLE)
   {
      fixed_row = start_row + referRowLength / 2 ;
      fixed_col = start_col + referColLength / 2 ;
   }
   else if( autoSetType == LOWER_MIDDLE)
   {
      fixed_row = start_row + referRowLength ;
      fixed_col = start_col + referColLength / 2 ;
   }
   else if( autoSetType == UPPER_RIGHT)
   {
      fixed_row = start_row ;
      fixed_col = start_col + referColLength ;
   }
   else if( autoSetType == MIDDLE_RIGHT)
   {
      fixed_row = start_row + referRowLength / 2 ;
      fixed_col = start_col + referColLength ;
   }
   else if( autoSetType == LOWER_RIGHT)
   {
      fixed_row = start_row + referRowLength ;
      fixed_col = start_col + referColLength ;
   }
   else
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s fixedOutputLocation failed,"
                   "wrong autoSetType:%s"OSS_NEWLINE,
                   errStrBuf, autoSetType.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::getFieldNameAndColour( const FieldStruct &fieldStruct,
                                    const string &displayMode,
                                    string &fieldName, Colours &fieldColour )
{
   INT32 rc = SDB_OK ;
   if( !fieldStruct.canSwitch )
   {
      fieldName = fieldStruct.absoluteName ;
      fieldColour.backGroundColor = fieldStruct.absoluteColour.backGroundColor ;
      fieldColour.foreGroundColor = fieldStruct.absoluteColour.foreGroundColor ;
   }
   else if( DELTA == displayMode )
   {
      fieldName = fieldStruct.deltaName ;
      fieldColour.backGroundColor = fieldStruct.deltaColour.backGroundColor ;
      fieldColour.foreGroundColor = fieldStruct.deltaColour.foreGroundColor ;
   }
   else if( ABSOLUTE == displayMode )
   {
      fieldName = fieldStruct.absoluteName ;
      fieldColour.backGroundColor = fieldStruct.absoluteColour.backGroundColor ;
      fieldColour.foreGroundColor = fieldStruct.absoluteColour.foreGroundColor ;
   }
   else if( AVERAGE == displayMode )
   {
      fieldName = fieldStruct.averageName ;
      fieldColour.backGroundColor = fieldStruct.averageColour.backGroundColor ;
      fieldColour.foreGroundColor = fieldStruct.averageColour.foreGroundColor ;
   }
   else
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s getFieldStructNameAndColour failed,"
                   "wrong displayMode:%s"OSS_NEWLINE,
                   errStrBuf, displayMode.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshDH( DynamicHelp &DH, Position &position )
{
   INT32 rc                = SDB_OK ;
   INT32 Y                 = position.referUpperLeft_Y ;
   INT32 X                 = position.referUpperLeft_X ;
   INT32 start_Y           = Y ;
   INT32 start_X           = X ;
   INT32 pos_X             = start_X ;
   KeySuite *keySuite      = NULL ;
   HotKey *hotkey          = NULL ;
   INT32 hotKey_pos        = 0 ;
   INT32 rowNumber         = 0 ;
   INT32 sum               = 0 ;
   INT32 colNumber         = 0 ;
   INT32 pairNumber        = 0 ;
   INT32 count             = 0;
   string printStr         = NULLSTRING ;
   INT32 cellLength        = DH.cellLength ;
   CHAR *pPrintfstr =
         ( CHAR * )SDB_OSS_MALLOC( cellLength * sizeof( CHAR ) ) ;
   if( !pPrintfstr )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s MVPRINTW_TOP failed,"
                   "can't malloc memory for printfstr :%d"OSS_NEWLINE,
                   errStrBuf, cellLength ) ;
      rc = SDB_OOM ;
      goto error ;
   }

   if( DH.wndOptionRow + DH.optionsRow > position.length_Y )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s refreshDisplayContent failed,"
                   "tableRow is too small"OSS_NEWLINE,
                   errStrBuf ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   rc = getActivatedKeySuite( &keySuite ) ;
   if( rc )
   {
      goto error ;
   }
   rc = fixedOutputLocation( Y, X, start_Y, start_X,
                             position.length_Y - DH.wndOptionRow - DH.optionsRow,
                             0, DH.autoSetType) ;
   if( rc )
   {
      goto error ;
   }

   {
      ossMemset( pPrintfstr, 0, cellLength );
      ossSnprintf(pPrintfstr, cellLength, "window options"
                  "(choose to enter window):" ) ;

      getColourPN( DH.prefixColour, pairNumber ) ;
      attron( COLOR_PAIR( pairNumber ) ) ;
      rc = mvprintw_SDBTOP( pPrintfstr, ossStrlen( pPrintfstr), LEFT,
                            start_Y, pos_X ) ;
      if( rc )
      {
         goto error ;
      }
      attroff( COLOR_PAIR( pairNumber ) ) ;
      ++rowNumber ;
   }

   hotKey_pos = 0 ;
   while ( rowNumber < DH.wndOptionRow && hotKey_pos < keySuite->hotKeyLength )
   {
      start_X = position.referUpperLeft_X ;
      start_Y += 1 ;
      sum = 0 ;
      for( colNumber = 0; colNumber< DH.wndOptionCol; ++ colNumber )
      {
         if( sum > position.length_X )
            break ;
         sum += cellLength ;
      }
      rc = fixedOutputLocation( start_Y, X, start_Y, start_X, 0,
                                position.length_X - sum, DH.autoSetType) ;
      if( rc )
      {
         goto error ;
      }

      ossMemset( pPrintfstr, 0, cellLength );
      while ( hotKey_pos < keySuite->hotKeyLength )
      {
         if( start_X + cellLength - X  > position.length_X )
         {
            break ;
         }
         hotkey = &keySuite->hotKey[hotKey_pos];
         if ( !hotkey->wndType )
         {
            ++hotKey_pos ;
            continue ;
         }

         pos_X = start_X ;
         if( JUMPTYPE_FIXED == hotkey->jumpType )
         {
            if( BUTTON_TAB == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_TAB ) ;
            else if( BUTTON_LEFT == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_LEFT ) ;
            else if( BUTTON_RIGHT == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_RIGHT ) ;
            else if( BUTTON_ENTER == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_ENTER ) ;
            else if( BUTTON_ESC == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_ESC ) ;
            else if( BUTTON_F5 == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_F5 ) ;
            else if( BUTTON_Q_LOWER == hotkey->button ||
                     BUTTON_H_LOWER == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength,
                            PREFIX_FORMAT, ( CHAR )hotkey->button ) ;
            else
               ossSnprintf( pPrintfstr, cellLength, PREFIX_NULL ) ;
         }
         else
            ossSnprintf( pPrintfstr, cellLength, PREFIX_FORMAT,
                         ( CHAR )hotkey->button ) ;

         ++count ;
         printStr = pPrintfstr ;
         getColourPN( DH.prefixColour, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( printStr, printStr.length(), LEFT,
                               start_Y, pos_X ) ;
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         pos_X += printStr.length() ;
         ossMemset( pPrintfstr, 0, cellLength ) ;
         getColourPN( DH.contentColour, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( hotkey->desc, hotkey->desc.length(),
                               LEFT, start_Y, pos_X );
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += DH.cellLength ;
         ++hotKey_pos ;
         if ( 0 == count % DH.wndOptionCol ||
              hotKey_pos + 1 >= keySuite->hotKeyLength )
         {
            ++rowNumber ;
            break ;
         }
      }
   }

   {
      ++start_Y ;
      start_X = position.referUpperLeft_X ;
      pos_X = start_X ;
      ossSnprintf(pPrintfstr, cellLength,
                  "options(use under window above): " ) ;

      getColourPN( DH.prefixColour, pairNumber ) ;
      attron( COLOR_PAIR( pairNumber ) ) ;
      rc = mvprintw_SDBTOP( pPrintfstr, ossStrlen( pPrintfstr), LEFT,
                            start_Y, pos_X ) ;
      if( rc )
      {
         goto error ;
      }
      attroff( COLOR_PAIR( pairNumber ) ) ;
   }

   hotKey_pos = 0 ;
   rowNumber = 0 ;
   count = 0 ;
   while ( rowNumber < DH.optionsRow )
   {
      start_Y += 1 ;
      sum = 0 ;
      for( colNumber = 0; colNumber< DH.optionsCol; ++ colNumber )
      {
         if( sum > position.length_X )
            break ;
         sum += cellLength ;
      }
      rc = fixedOutputLocation( start_Y, X, start_Y, start_X, 0,
                                position.length_X - sum, DH.autoSetType) ;
      if( rc )
      {
         goto error ;
      }

      ossMemset( pPrintfstr, 0, cellLength );
      while ( hotKey_pos < keySuite->hotKeyLength )
      {
         if( start_X + cellLength - X  > position.length_X )
         {
            break ;
         }
         hotkey = &keySuite->hotKey[hotKey_pos];
         if ( hotkey->wndType )
         {
            ++hotKey_pos ;
            continue ;
         }

         ++count ;
         pos_X = start_X ;
         if( JUMPTYPE_FIXED == hotkey->jumpType )
         {
            if( BUTTON_TAB == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_TAB ) ;
            else if( BUTTON_LEFT == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_LEFT ) ;
            else if( BUTTON_RIGHT == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_RIGHT ) ;
            else if( BUTTON_ENTER == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_ENTER ) ;
            else if( BUTTON_ESC == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_ESC ) ;
            else if( BUTTON_F5 == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength, PREFIX_F5 ) ;
            else if( BUTTON_Q_LOWER == hotkey->button ||
                     BUTTON_H_LOWER == hotkey->button )
               ossSnprintf( pPrintfstr, cellLength,
                            PREFIX_FORMAT, ( CHAR )hotkey->button ) ;
            else
               ossSnprintf( pPrintfstr, cellLength, PREFIX_NULL ) ;
         }
         else
            ossSnprintf( pPrintfstr, cellLength, PREFIX_FORMAT,
                         ( CHAR )hotkey->button ) ;

         printStr = pPrintfstr ;
         getColourPN( DH.prefixColour, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( printStr, printStr.length(), LEFT,
                               start_Y, pos_X ) ;
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         pos_X += printStr.length() ;
         ossMemset( pPrintfstr, 0, cellLength ) ;
         getColourPN( DH.contentColour, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( hotkey->desc, hotkey->desc.length(),
                               LEFT, start_Y, pos_X );
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += DH.cellLength ;
         ++hotKey_pos ;

         if ( 0 == count % DH.optionsCol ||
              hotKey_pos + 1 >= keySuite->hotKeyLength )
         {
            ++rowNumber ;
            break ;
         }
      }
   }

done :
   if( pPrintfstr )
      SDB_OSS_FREE( pPrintfstr );
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshDE( DynamicExpressionOutPut &DE, Position &position )
{
   INT32 rc                        = SDB_OK ;
   INT32 Y                         = position.referUpperLeft_Y ;
   INT32 X                         = position.referUpperLeft_X ;
   INT32 start_Y                   = Y ;
   INT32 start_X                   = X ;
   INT32 Sum                       = 0 ;
   INT32 expressionLength          = 0 ;
   INT32 pairNumber                = 0 ;
   string result                   = NULLSTRING ;
   INT32 expressionNumber          = 0 ;
   INT32 rowNumber                 = 0 ;
   ExpressionContent *EC           = NULL ;
   rc = fixedOutputLocation( Y, X,
                             start_Y, start_X,
                             position.length_Y - DE.rowNumber,
                             0,
                             DE.autoSetType) ;
   if( rc )
   {
      goto error ;
   }
   for( rowNumber = 0;
       rowNumber < DE.rowNumber && rowNumber < position.length_Y ;
       ++rowNumber )
   {
      start_Y += rowNumber ;
      Sum = 0 ;
      expressionLength = 0 ;
      pairNumber = 0 ;
      result = NULLSTRING ;
      for( expressionNumber = 0; expressionNumber < DE.expressionNumber;
           ++expressionNumber )
      {
         EC = &DE.content[expressionNumber] ;
         if( STATIC_EXPRESSION == EC->expressionType )
         {
            result = EC->expressionValue.text ;
         }
         else if( DYNAMIC_EXPRESSION == EC->expressionType )
         {
            rc = getExpression( EC->expressionValue.expression,
                                result ) ;
            if( rc )
            {
               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s refreshDisplayContent failed,"
                            "getExpression failed"OSS_NEWLINE,
                            errStrBuf ) ;
               goto error ;
            }
         }
         if( NULLSTRING == result )
            result = STRING_NULL ;
         expressionLength = result.length() ;
         if( Sum + expressionLength > position.length_X )
            continue ;
         if( EC->rowLocation != rowNumber + 1 )
         {
            continue ;
         }
         Sum += expressionLength ;
         ++Sum ; // add space
      }

      rc = fixedOutputLocation( start_Y, X, start_Y, start_X,
                                0, position.length_X -Sum ,
                                DE.autoSetType) ;
      if( rc )
      {
         goto error ;
      }
      for( expressionNumber = 0; expressionNumber < DE.expressionNumber;
           ++expressionNumber )
      {
         EC = &DE.content[expressionNumber] ;
         if( EC->rowLocation != rowNumber + 1 )
         {
            continue ;
         }
         if( STATIC_EXPRESSION == EC->expressionType )
         {
            result = EC->expressionValue.text ;
         }
         else if( DYNAMIC_EXPRESSION == EC->expressionType )
         {
            getExpression( EC->expressionValue.expression,
                           result ) ;
         }
         if( NULLSTRING == result )
            result = STRING_NULL ;
         expressionLength = result.length() ;
         if( start_X + expressionLength - X > position.length_X )
         {
            continue ;
         }
         getColourPN( EC->colour,pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( result, expressionLength, EC->alignment,
                               start_Y, start_X ) ;
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += expressionLength ;
         ++start_X ;
      }
      start_Y -= rowNumber ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshDS_Table( DynamicSnapshotOutPut &DS, INT32 ROW, INT32 COL,
                              Position &position, string autoSetType )
{
   INT32 rc = SDB_OK ;
   BSONObj bsonobj ;
   string baseField              = DS.baseField ;
   INT32 rowNumber               = 0;
   FieldStruct* Fixed            = NULL ;
   FieldStruct* Mobile           = NULL ;
   INT32 FLength                 = 0 ;
   INT32 MLength                 = 0 ;
   InputPanel &input             = root.input ;
   INT32 Y                       = position.referUpperLeft_Y ;
   INT32 X                       = position.referUpperLeft_X ;
   INT32 length_Y                = position.length_Y ;
   INT32 length_X                = position.length_X ;
   INT32 cellLength              = DS.tableCellLength ;
   INT32 start_Y                 = Y ;
   INT32 start_X                 = X ;

   INT32 index_COL               = 0 ;

   INT32 pos_Field               = 0 ;
   INT32 end_fixed_mobile        = 0 ;
   INT32 start_fixed_mobile      = 0 ;
   string displayMode            =
         DISPLAYMODECHOOSER[input.displayModeChooser] ;

   string fieldName              = NULLSTRING ;
   Colours fieldColour ;
   INT32 pairNumber              = 0 ;
   string result                 =  NULLSTRING ;
   UINT32 pos_snapshot           = 0 ;
   fieldColour.backGroundColor   = 6 ;
   fieldColour.foreGroundColor   = 0 ;
   FLength = DS.actualFixedFieldLength ;
   MLength = DS.actualMobileFieldLength ;
   rc = fixedOutputLocation( Y, X, start_Y, start_X, length_Y - ROW * 2, 0,
                             autoSetType ) ;
   if( rc )
   {
      goto error ;
   }

   for( index_COL = 0; index_COL <= COL; ++index_COL )
   {
      if(( index_COL + 1 ) * cellLength > length_X )
         break ;
   }
   ROW = ROW * COL / index_COL ;
   COL = index_COL ;

   for( rowNumber = 0; rowNumber < ROW; ++rowNumber )
   {
      start_Y += rowNumber ;
      if( start_Y - Y >= length_Y )
      {
         break ;
      }
      rc = fixedOutputLocation( start_Y, X, start_Y, start_X,
                                0, length_X -COL * cellLength,
                                autoSetType ) ;
      if( rc )
      {
         goto error ;
      }
      index_COL = 0 ;
      start_fixed_mobile = end_fixed_mobile ;
      while( 1 )
      {
         Fixed = &DS.fixedField[end_fixed_mobile] ;
         if( start_X + cellLength - X > length_X )
         {
            break ;
         }
         if( end_fixed_mobile >= FLength)
         {
            break ;
         }
         if( index_COL >= COL )
         {
            break ;
         }
         rc = getFieldNameAndColour( Fixed[0], displayMode,
                                     fieldName, fieldColour) ;
         if( rc )
         {
            goto error ;
         }
         getColourPN( fieldColour, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( fieldName, cellLength, Fixed->alignment,
                               start_Y, start_X ) ;
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += cellLength ;
         ++start_X ; // add separate space
         ++end_fixed_mobile ;
         ++index_COL ;
      }
      while( 1 )
      {
         Mobile = &DS.mobileField[end_fixed_mobile - FLength] ;
         if( start_X + cellLength - X > length_X )
            break ;
         if( end_fixed_mobile - FLength >= MLength )
            break ;
         if( index_COL >= COL )
            break ;
         rc = getFieldNameAndColour( Mobile[0], displayMode,
                                     fieldName, fieldColour ) ;
         if( rc )
         {
            goto error ;
         }
         getColourPN( fieldColour, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         rc = mvprintw_SDBTOP( fieldName, cellLength, Mobile->alignment,
                               start_Y, start_X ) ;
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += cellLength ;
         ++start_X ; // add separate space
         ++end_fixed_mobile ;
         ++index_COL ;
      }

      start_Y += 1 ;
      if( start_Y - Y >= length_Y )
      {
         goto done ;
      }

      for( pos_snapshot = 0;
           pos_snapshot < input.cur_Snapshot.size();
           ++pos_snapshot )
      {
         pos_Field = start_fixed_mobile ;
         bsonobj = input.cur_Snapshot[pos_snapshot] ;
         rc = fixedOutputLocation( start_Y, X, start_Y, start_X, 0,
                                   length_X -COL * cellLength, autoSetType ) ;
         if( rc )
         {
            goto error ;
         }
         while( 1 )
         {
            Fixed = &DS.fixedField[pos_Field] ;
            if( start_X + cellLength - X > length_X )
               break ;
            if( pos_Field >= FLength )
               break ;
            if( pos_Field >= end_fixed_mobile)
               break ;
            getFieldNameAndColour( Fixed[0], displayMode, fieldName,
                                   fieldColour) ;
            getColourPN( fieldColour, pairNumber ) ;
            result = NULLSTRING ;
            rc = getResultFromBSONObj( bsonobj, Fixed->sourceField,
                                       displayMode, result, Fixed->canSwitch,
                                       baseField, Fixed->warningValue,
                                       pairNumber) ;
            attron( COLOR_PAIR( pairNumber ) ) ;
            if( rc )
            {
               goto error ;
            }
            rc = mvprintw_SDBTOP( result, cellLength, Fixed->alignment,
                                  start_Y, start_X ) ;
            if( rc )
            {
               goto error ;
            }
            attroff( COLOR_PAIR( pairNumber ) ) ;
            start_X += cellLength ;
            ++start_X ; // add separate pace
            ++pos_Field ;
         }
         while( 1 )
         {
            Mobile = &DS.mobileField[pos_Field - FLength] ;
            if( start_X + cellLength - X > length_X )
               break ;
            if( pos_Field >= end_fixed_mobile )
               break ;
            getFieldNameAndColour( Mobile[0], displayMode, fieldName,
                                   fieldColour ) ;
            getColourPN( fieldColour, pairNumber ) ;
            result = NULLSTRING ;
            rc = getResultFromBSONObj( bsonobj, Mobile->sourceField,
                                       displayMode, result, Mobile->canSwitch,
                                       baseField, Mobile->warningValue,
                                       pairNumber );
            attron( COLOR_PAIR( pairNumber ) ) ;
            if( rc )
            {
               goto error ;
            }
            rc = mvprintw_SDBTOP( result, cellLength, Mobile->alignment,
                                  start_Y, start_X ) ;
            if( rc )
            {
               goto error ;
            }
            attroff( COLOR_PAIR( pairNumber ) ) ;
            start_X += cellLength;
            ++start_X ; // add separate pace
            ++pos_Field ;
         }
         start_Y += 1 ;
         if( start_Y - Y >= length_Y )
         {
            break ;
         }
      }
      start_Y -= rowNumber ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshDS_List( DynamicSnapshotOutPut &DS, Position &position,
                             const string &autoSetType )
{
   INT32 rc                      = SDB_OK ;
   BSONObj bsonobj ;
   FieldStruct* Fixed            = NULL ;
   FieldStruct* Mobile           = NULL ;
   INT32 FLength                 = 0 ;
   INT32 MLength                 = 0 ;
   InputPanel &input             = root.input ;
   INT32 Y                       = position.referUpperLeft_Y ;
   INT32 X                       = position.referUpperLeft_X ;
   INT32 length_Y                = position.length_Y ;
   INT32 length_X                = position.length_X ;
   INT32 start_Y                 = Y ;
   INT32 start_X                 = X ;

   string dividingChar           = DIVIDINGCHAR ;
   INT32 dividingColour          = 0 ;
   string dividingLine           = NULLSTRING ;
   string fieldName              = NULLSTRING ;
   Colours fieldColour ;
   fieldColour.backGroundColor   = 6 ;
   fieldColour.foreGroundColor   = 0 ;
   string displayMode            =
         DISPLAYMODECHOOSER[input.displayModeChooser] ;
   string baseField              = DS.baseField ;
   string serialNumberAlignment  = LEFT ;

   INT32 sum                     = 0 ;
   UINT32 pos_snapshot           = 0 ;
   INT32 pairNumber              = 0 ;
   string result                 =  NULLSTRING ;
   INT32 fLength                 = 0 ;
   INT32 mLength                 = 0 ;
   CHAR serialNumber[SERIALNUMBER_LENGTH] = {0} ;
   getColourPN( input.colourOfTheDividingLine, dividingColour );
   FLength = DS.actualFixedFieldLength ;
   MLength = DS.actualMobileFieldLength ;
   ossMemset( serialNumber, 0, SERIALNUMBER_LENGTH ) ;

   sum = 0 ;
   for( fLength = 0; fLength < FLength; ++fLength )
   {
      Fixed = &DS.fixedField[fLength] ;
      if( sum + Fixed->contentLength > length_X )
         break ;
      sum += Fixed->contentLength ;
   }
   if( input.fieldPosition >= MLength )
   {
      input.fieldPosition = MLength - 1 ;
   }
   for( mLength = input.fieldPosition;
        mLength < MLength; ++mLength )
   {
      Mobile = &DS.mobileField[mLength] ;
      if( sum + Mobile->contentLength  > length_X )
         break ;
      sum += Mobile->contentLength ;
   }
   rc = fixedOutputLocation( start_Y, X, start_Y, start_X,
                             0, length_X - sum, autoSetType );
   if( rc )
   {
      goto error ;
   }

   start_X += SERIALNUMBER_LENGTH ;
   ++start_X ; // add separate pace

   for( fLength = 0; fLength < FLength; ++fLength )
   {
      Fixed = &DS.fixedField[fLength] ;
      if( start_X + Fixed->contentLength - X > length_X )
         break ;
      rc = getFieldNameAndColour( Fixed[0], displayMode, fieldName,
                                  fieldColour) ;
      if( rc )
      {
         goto error ;
      }
      getColourPN( fieldColour, pairNumber ) ;
      attron( COLOR_PAIR( pairNumber ) ) ;
      rc = mvprintw_SDBTOP( fieldName, Fixed->contentLength, Fixed->alignment,
                            start_Y, start_X ) ;
      if( rc )
      {
         goto error;
      }
      attroff( COLOR_PAIR( pairNumber ) ) ;

      attron( COLOR_PAIR( dividingColour ) ) ;
      dividingLine = getDividingLine( DIVIDINGCHAR, Fixed->contentLength ) ;
      rc = mvprintw_SDBTOP( dividingLine, Fixed->contentLength,
                            Fixed->alignment, start_Y+1, start_X ) ;
      if( rc )
      {
         goto error;
      }
      attroff( COLOR_PAIR( dividingColour ) ) ;
      start_X += Fixed->contentLength ;
      ++start_X ; // add separate pace
   }

   if( input.fieldPosition >= MLength )
   {
      input.fieldPosition = MLength - 1 ;
   }

   for( mLength = input.fieldPosition; mLength < MLength; ++mLength )
   {
      Mobile = &DS.mobileField[mLength] ;
      if( start_X + Mobile->contentLength - X > length_X )
         break ;
      rc = getFieldNameAndColour( Mobile[0], displayMode,
                                  fieldName, fieldColour);
      if( rc )
      {
         goto error ;
      }
      getColourPN( fieldColour, pairNumber ) ;
      attron( COLOR_PAIR( pairNumber ) ) ;
      rc = mvprintw_SDBTOP( fieldName, Mobile->contentLength,Mobile->alignment,
                            start_Y, start_X ) ;
      if( rc )
      {
         goto error ;
      }
      attroff( COLOR_PAIR( pairNumber ) ) ;

      attron( COLOR_PAIR( dividingColour ) ) ;
      dividingLine = getDividingLine( DIVIDINGCHAR, Mobile->contentLength ) ;
      rc = mvprintw_SDBTOP( dividingLine, Mobile->contentLength,
                            Mobile->alignment, start_Y+1, start_X ) ;
      if( rc )
      {
         goto error;
      }
      attroff( COLOR_PAIR( dividingColour ) ) ;
      start_X += Mobile->contentLength ;
      ++start_X ; // add separate pace
   }

   start_Y += 2 ; //
   if( start_Y - Y >= length_Y )
   {
      goto done ;
   }
   fixedOutputLocation( start_Y, X, start_Y, start_X, 0, length_X -sum ,
                        autoSetType ) ;
   for( pos_snapshot = 0;pos_snapshot < input.cur_Snapshot.size();
        ++pos_snapshot )
   {

      ossMemset( serialNumber, 0, SERIALNUMBER_LENGTH ) ;

      ossSnprintf( serialNumber, SERIALNUMBER_LENGTH,"%3d", pos_snapshot + 1 ) ;
      rc = mvprintw_SDBTOP( serialNumber, SERIALNUMBER_LENGTH,
                            serialNumberAlignment, start_Y, start_X ) ;
      if( rc )
      {
         goto error;
      }

      start_X += SERIALNUMBER_LENGTH ;
      ++start_X ; // add separate pace

      bsonobj = input.cur_Snapshot[pos_snapshot] ;
      for( fLength = 0; fLength < FLength; ++fLength )
      {
         Fixed = &DS.fixedField[fLength] ;
         if( start_X + Fixed[fLength].contentLength - X > length_X )
            break ;
         getFieldNameAndColour( Fixed[0], displayMode, fieldName, fieldColour ) ;
         getColourPN( fieldColour, pairNumber ) ;
         result = NULLSTRING ;
         rc = getResultFromBSONObj( bsonobj, Fixed->sourceField, displayMode,
                                    result, Fixed->canSwitch, baseField,
                                    Fixed->warningValue, pairNumber );
         attron( COLOR_PAIR( pairNumber ) ) ;
         if( rc )
         {
            goto error ;
         }
         rc = mvprintw_SDBTOP( result, Fixed->contentLength, Fixed->alignment,
                               start_Y, start_X ) ;
         if( rc )
         {
            goto error;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += Fixed->contentLength ;
         ++start_X ; // add separate pace
      }
      if( input.fieldPosition >= MLength )
      {
         input.fieldPosition = MLength - 1 ;
      }
      for( mLength = input.fieldPosition; mLength < MLength; ++mLength )
      {
         Mobile = &DS.mobileField[mLength] ;
         if( start_X + Mobile->contentLength - X > length_X )
            break ;
         getFieldNameAndColour( Mobile[0], displayMode, fieldName, fieldColour) ;
         getColourPN( fieldColour, pairNumber ) ;
         result =  NULLSTRING ;
         rc = getResultFromBSONObj( bsonobj, Mobile->sourceField, displayMode,
                                    result, Mobile->canSwitch, baseField,
                                    Mobile->warningValue, pairNumber ) ;
         attron( COLOR_PAIR( pairNumber ) ) ;
         if( rc )
         {
            goto error ;
         }
         rc = mvprintw_SDBTOP( result, Mobile->contentLength, Mobile->alignment,
                               start_Y, start_X ) ;
         if( rc )
         {
            goto error ;
         }
         attroff( COLOR_PAIR( pairNumber ) ) ;
         start_X += Mobile->contentLength ;
         ++start_X ; // add separate pace
      }
      start_Y += 1 ;
      fixedOutputLocation( start_Y, X, start_Y, start_X, 0,length_X -sum ,
                           autoSetType );
      if( start_Y - Y >= length_Y )
      {
         break ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}
INT32 Event::refreshDS( DynamicSnapshotOutPut &DS, Position &position )
{
   INT32 rc                      = SDB_OK ;
   InputPanel &input             = root.input ;
   string AUTOSETTYPE            = NULLSTRING ;
   string STYLE                  = NULLSTRING ;
   INT32 ROW                     = 0 ;
   INT32 COL                     = 0 ;
   string fieldName              = NULLSTRING ;
   if( GLOBAL == input.snapshotModeChooser )
   {
      AUTOSETTYPE = DS.globalAutoSetType ;
      STYLE = DS.globalStyle ;
      if( TABLE == STYLE )
      {
         ROW = DS.globalRow ;
         COL = DS.globalCol ;
      }
   }
   else if( GROUP == input.snapshotModeChooser )
   {
      AUTOSETTYPE = DS.groupAutoSetType ;
      STYLE = DS.groupStyle ;
      if( TABLE == STYLE )
      {
         ROW = DS.groupRow ;
         COL = DS.groupCol ;
      }
   }
   else if( NODE == input.snapshotModeChooser )
   {
      AUTOSETTYPE = DS.nodeAutoSetType ;
      STYLE = DS.nodeStyle ;
      if( TABLE == STYLE )
      {
         ROW = DS.nodeRow ;
         COL = DS.nodeCol ;
      }
   }
   else
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s refreshDisplayContent failed, "
                   "wrong snapshotModeChooser: %s"OSS_NEWLINE,
                   errStrBuf,
                   input.snapshotModeChooser.c_str() ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   if( TABLE == STYLE )
   {
      rc = refreshDS_Table( DS, ROW, COL, position, AUTOSETTYPE ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else
   {
      refreshDS_List( DS, position, AUTOSETTYPE ) ;
      if( rc )
      {
         goto error ;
      }
   }
done :
   return rc ;
error :
   goto done ;

}

INT32 Event::refreshDisplayContent( DisplayContent &displayContent,
                                    string displayType,
                                    Position &actualPosition )
{
   INT32 rc = SDB_OK ;
   INT32 pairNumber  = 0 ;
   if( DISPLAYTYPE_STATICTEXT_HELP_Header == displayType ||
       DISPLAYTYPE_STATICTEXT_MAIN        == displayType ||
       DISPLAYTYPE_STATICTEXT_LICENSE     == displayType )
   {
      getColourPN(
            displayContent.staticTextOutPut.colour,
            pairNumber );
      attron( COLOR_PAIR( pairNumber ) ) ;
      mvprintw( actualPosition.referUpperLeft_Y,
                actualPosition.referUpperLeft_X,
                displayContent.staticTextOutPut.outputText ) ;
      attroff( COLOR_PAIR( pairNumber ) ) ;
   }
   else if( DISPLAYTYPE_DYNAMIC_HELP == displayType )
   {
      rc = refreshDH( displayContent.dynamicHelp,
                      actualPosition ) ;
      if( rc )
      {
         goto error ;
      }

   }
   else if( DISPLAYTYPE_DYNAMIC_EXPRESSION == displayType )
   {
      rc = refreshDE( displayContent.dyExOutPut,
                      actualPosition ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else if( DISPLAYTYPE_DYNAMIC_SNAPSHOT == displayType )
   {
      rc = refreshDS( displayContent.dySnapshotOutPut,
                      actualPosition ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                  "%s refreshDisplayContent failed,"
                  "wrong displayType:%s"OSS_NEWLINE,
                  errStrBuf, displayType.c_str() ) ;
      rc = SDB_ERROR;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshNodeWindow( NodeWindow &window )
{
   INT32 rc = SDB_OK ;
   Position actualPosition ;
   INT32 row = 0;
   INT32 col = 0;
   actualPosition.length_X = 0 ;
   actualPosition.length_Y = 0 ;
   actualPosition.referUpperLeft_X = 0 ;
   actualPosition.referUpperLeft_Y = 0 ;
   rc = getActualPosition( actualPosition, window.position,
                           window.zoomMode, window.occupyMode) ;
   if( rc )
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s refreshNodeWindow failed,"
                   "getActualPosition failed"OSS_NEWLINE,
                   errStrBuf ) ;
      goto error ;
   }
   getmaxyx( stdscr, row, col ) ;
   if( row < window.actualWindowMinRow || col < window.actualWindowMinColumn )
   {
      goto done ;
   }
   rc = refreshDisplayContent( window.displayContent, window.displayType,
                               actualPosition ) ;
   if( rc )
   {

      ossSnprintf( errStrBuf, errStrLength, "%s", errStr ) ;
      ossSnprintf( errStr, errStrLength, "%s refreshNodeWindow failed,"
                   "refreshDisplayContent failed"OSS_NEWLINE,
                   errStrBuf );
      goto error;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshHT( HeadTailMap *headtail )
{
   INT32 rc = SDB_OK ;
   INT32 numOfSubWindow = 0 ;
   if( !headtail )
      goto done ;
   for( numOfSubWindow = 0;
        numOfSubWindow < headtail->value.numOfSubWindow;
        ++numOfSubWindow )
   {
      rc = refreshNodeWindow( headtail->value.subWindow[numOfSubWindow] );
      if ( rc )
      {

         ossSnprintf( errStrBuf, errStrLength, "%s", errStr ) ;
         ossSnprintf( errStr, errStrLength, "%s refreshHeadTail failed,"
                      "refreshNodeWindow failed,"
                      "numOfSubWindow = %d"OSS_NEWLINE,
                      errStrBuf, numOfSubWindow ) ;
         goto error ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshBD( BodyMap *body )
{
   INT32 rc = SDB_OK ;
   INT32 numOfSubWindow = 0 ;
   for( numOfSubWindow = 0;
        numOfSubWindow < body->value.numOfSubWindow;
        ++numOfSubWindow )
   {
      rc = refreshNodeWindow( body->value.subWindow[numOfSubWindow] ) ;
      if ( rc )
      {

         ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
         ossSnprintf( errStr, errStrLength,
                      "%s refreshBody failed,"
                      "refreshNodeWindow failed,"
                      "numOfSubWindow = %d"OSS_NEWLINE,
                      errStrBuf, numOfSubWindow ) ;
         goto error ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}
void Event::initAllColourPairs()
{
   INT32 back = 0 ;
   INT32 fore = 0 ;
   INT32 pairNumber = 0 ;
   for( back = 0; back < COLOR_MULTIPLE; ++back )
   {
      for( fore = 0; fore < COLOR_MULTIPLE; ++fore )
      {
         pairNumber = fore + back * COLOR_MULTIPLE ;
         init_pair( pairNumber, fore, back ) ;
      }
   }
}

INT32 Event::addFixedHotKey()
{
   INT32 rc = SDB_OK ;
   INT32 keyLength = 0 ;
   INT32 keyLengthFromConf = 0 ;
   KeySuite *keySuite = NULL ;
   HotKey *hotkey = NULL ;
   for( INT32 i = 0; i < root.keySuiteLength; ++i )
   {
      keySuite = &root.keySuite[i] ;
      keyLength = keySuite->hotKeyLength ;
      keyLengthFromConf = keySuite->hotKeyLengthFromConf ;
      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_TAB ;
         hotkey->jumpType = JUMPTYPE_FIXED;
         hotkey->jumpName = "Evaluation Model" ;
         hotkey->desc = "switch display Model" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_LEFT;
         hotkey->jumpType = JUMPTYPE_FIXED;
         hotkey->jumpName = "Move left" ;
         hotkey->desc = "move left" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_RIGHT ;
         hotkey->jumpType = JUMPTYPE_FIXED ;
         hotkey->jumpName = "Move right" ;
         hotkey->desc = "move right" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_ENTER ;
         hotkey->jumpType = JUMPTYPE_FIXED ;
         hotkey->jumpName = "last view" ;
         hotkey->desc = "to last view, used under help window" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_ESC;
         hotkey->jumpType = JUMPTYPE_FIXED ;
         hotkey->jumpName = "cancel the operation" ;
         hotkey->desc = "cancel current operation" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_F5;
         hotkey->jumpType = JUMPTYPE_FIXED ;
         hotkey->jumpName = "refresh" ;
         hotkey->desc = "refresh immediately" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_H_LOWER;
         hotkey->jumpType = JUMPTYPE_FIXED ;
         hotkey->jumpName = "Help" ;
         hotkey->desc = "help" ;
         hotkey->wndType = TRUE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      if( keyLength < keyLengthFromConf )
      {
         hotkey = &keySuite->hotKey[keyLength] ;
         hotkey->button = BUTTON_Q_LOWER;
         hotkey->jumpType = JUMPTYPE_FIXED ;
         hotkey->jumpName = "Quit" ;
         hotkey->desc = "quit" ;
         hotkey->wndType = FALSE ;
         ++keyLength ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }

      keySuite->hotKeyLength = keyLength ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::matchNameInFieldStruct( const FieldStruct *src,
                                     const string DisplayName )
{
   INT32 rc           = SDB_OK ;
   InputPanel &input  = root.input ;
   string displayMode = DISPLAYMODECHOOSER[input.displayModeChooser] ;
   if( ABSOLUTE == displayMode )
   {
      if( DisplayName == src->absoluteName )
      {
         input.sortingField = src->sourceField ;
      }
      else
      {
         rc = SDB_ERROR ;
         goto error ;
      }
   }
   else if( DELTA == displayMode )
   {
      if(  src->canSwitch )
      {
         if( DisplayName == src->deltaName )
         {
            input.sortingField = src->sourceField ;
         }
         else
         {
            rc = SDB_ERROR ;
            goto error ;
         }
      }
      else
      {
         if( DisplayName == src->absoluteName )
         {
            input.sortingField = src->sourceField ;
         }
         else
         {
            rc = SDB_ERROR ;
            goto error ;
         }
      }
   }
   else if( AVERAGE == displayMode )
   {
      if(  src->canSwitch )
      {
         if( DisplayName == src->averageName )
         {
            input.sortingField = src->sourceField ;
         }
         else
         {
            rc = SDB_ERROR ;
            goto error ;
         }
      }
      else
      {
         if( DisplayName == src->absoluteName )
         {
            input.sortingField = src->sourceField ;
         }
         else
         {
            rc = SDB_ERROR ;
            goto error ;
         }
      }
   }
   else
   {
      rc = SDB_ERROR ;
      goto error ;
   }
 done :
   return rc ;
 error :
   goto done ;
}

INT32 Event::matchSourceFieldByDisplayName( const string DisplayName )
{
   INT32 rc = SDB_OK ;
   INT32 numOfSubWindow = 0 ;
   FieldStruct *Fixed = NULL ;
   FieldStruct *Mobile = NULL ;
   INT32 FixedLength = 0 ;
   INT32 MobileLength = 0 ;
   InputPanel &input = root.input ;
   input.sortingField = NULLSTRING ;
   Panel &panel = input.activatedPanel->value ;
   NodeWindow *window = NULL ;
   DynamicSnapshotOutPut *DS = NULL ;
   string displayMode = DISPLAYMODECHOOSER[input.displayModeChooser] ;
   for( numOfSubWindow = 0; numOfSubWindow < panel.numOfSubWindow;
        ++numOfSubWindow )
   {
      window = &panel.subWindow[numOfSubWindow] ;
      if( DISPLAYTYPE_DYNAMIC_SNAPSHOT == window->displayType )
      {
         DS = &window->displayContent.dySnapshotOutPut ;
         FixedLength =
               DS->actualFixedFieldLength ;
         MobileLength =
               DS->actualMobileFieldLength ;
         while( FixedLength > 0 )
         {
            --FixedLength ;
            Fixed = &DS->fixedField[FixedLength] ;
            rc = matchNameInFieldStruct( Fixed, DisplayName ) ;
            if( !rc )
            {
               goto done ;
            }
         }
         while( MobileLength> 0 )
         {
            --MobileLength ;
            Mobile = &DS->mobileField[MobileLength] ;
            rc = matchNameInFieldStruct( Mobile, DisplayName ) ;
            if( !rc )
            {
               goto done ;
            }
         }
      }
   }
done :
   return rc ;
}

INT32 Event::eventManagement( INT64 key ,BOOLEAN isFirstStart )
{
   INT32 rc = SDB_OK ;
   fd_set fds ;
   INT32 maxfd = STDIN + 1 ;
   KeySuite* keySuite = NULL ;
   HotKey* hotKey = NULL ;
   INT32 row = 0 ;
   INT32 col = 0 ;
   INT32 filterNum = 0 ;
   INT32 refreshInterval = 0 ;
   string note = NULLSTRING ;
   string displayName = NULLSTRING ; // use it when sorting
   HeadTailMap *header = NULL ;
   HeadTailMap *footer = NULL ;
   InputPanel &input = root.input ;
   BodyMap *activatedPanel = input.activatedPanel ;
   rc = getActivatedKeySuite( &keySuite ) ;
   if( rc )
   {
      goto error ;
   }
   if( 0 == key )
   {
      goto done ;
   }
   else if( 0 > key )
   {
      rc = SDB_ERROR ;
      goto error ;
   }
   else
   {
      for( INT32 i = 0; i < keySuite->hotKeyLength; ++i )
      {
         if( keySuite->hotKey[i].button == key )
         {
            hotKey = &keySuite->hotKey[i] ;
            break;
         }
      }
      if( !hotKey )
      {
         if( !isFirstStart )
         {
               rc = eventManagement( BUTTON_H_LOWER, FALSE );
         }
      }
      else
      {
         if( JUMPTYPE_PANEL == hotKey->jumpType )
         {
            rc = assignActivatedPanelByLabelName( &input.activatedPanel,
                        hotKey->jumpName ) ;
            if( rc )
            {

               ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
               ossSnprintf( errStr, errStrLength,
                            "%s assignPanelByLabelName failed"OSS_NEWLINE,
                            errStrBuf ) ;
               goto error ;
            }
            input.displayModeChooser = 0 ;
            input.snapshotModeChooser = GLOBAL ;
            input.forcedToRefresh_Global = REFRESH ;
            input.sortingField = NULLSTRING ;
            input.sortingWay = NULLSTRING ;
            input.filterNumber = 0 ;
            input.filterCondition= NULLSTRING;
            input.isFirstGetSnapshot = TRUE ;
         }
         else if( JUMPTYPE_FIXED == hotKey->jumpType )
         {
            if( hotKey->button >= 256 )
            {
               if( BUTTON_LEFT == hotKey->button )//left
               {
                  --root.input.fieldPosition ;
                  if( input.fieldPosition < 0 )
                  {
                     input.fieldPosition = 0 ;
                  }
                  input.forcedToRefresh_Local = REFRESH ;
               }
               else if( BUTTON_RIGHT == hotKey->button )//right
               {
                  ++input.fieldPosition ;
                  input.forcedToRefresh_Local = REFRESH ;
               }
               else if( BUTTON_F5 == hotKey->button )
               {
                  input.forcedToRefresh_Global= REFRESH ;
               }
            }
            else if( BUTTON_Q_LOWER == hotKey->button )
            {
               rc = SDB_SDBTOP_DONE ;
            }
            else if( BUTTON_ENTER == hotKey->button )
            {
               input.forcedToRefresh_Local= REFRESH ;
            }
            else if( BUTTON_ESC == hotKey->button )
            {
               input.forcedToRefresh_Local= REFRESH ;
            }
            else if( BUTTON_TAB == hotKey->button )
            {
               if( BODYTYPE_NORMAL !=
                   activatedPanel->bodyPanelType )
               {
                  goto done;
               }
               ++input.displayModeChooser ;
               input.displayModeChooser %= DISPLAYMODENUMBER ;
               input.forcedToRefresh_Local = REFRESH ;
            }
            else if( BUTTON_H_LOWER == hotKey->button )
            {
               rc = assignActivatedPanel( &activatedPanel,
                                          BODYTYPE_HELP_DYNAMIC) ;
               rc = getActivatedHeadTailMap( activatedPanel,
                                             &header, &footer) ;
               if( rc )
               {
                  goto error ;
               }
               rc = refreshAll( header, activatedPanel, footer, TRUE ) ;
               if( rc )
               {
                  goto error ;
               }
               maxfd = STDIN + 1 ;
               FD_ZERO ( &fds ) ;
               FD_SET ( STDIN, &fds ) ;
               rc = select ( maxfd, &fds, NULL, NULL, NULL ) ;
               if( 0 > rc )
               {
                  rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               }
               else if( 0 < rc )
               {
                  ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
                  read(STDIN, sdbtopBuffer, BUFFERSIZE ) ;
                  rc = getSdbTopKey( sdbtopBuffer, key) ;
                  if( rc )
                  {
                     goto error ;
                  }
                  rc = eventManagement( key, FALSE ) ;
               }
               else
               {
                  ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
                  ossSnprintf( errStr, errStrLength,
                               "%s buttonManagement failed,"
                               "select ( maxfd, &fds, NULL, NULL, NULL) "
                               "failed"OSS_NEWLINE,
                               errStrBuf ) ;
                  goto error ;
               }
            }
         }
         else if( JUMPTYPE_GLOBAL == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            input.snapshotModeChooser = GLOBAL ;
            input.forcedToRefresh_Global = REFRESH ;
         }
         else if( JUMPTYPE_GROUP == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the group name:" ;
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 ) ;
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
            move( row - 1, 0 ) ;
            clrtobot() ;
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               input.groupName = sdbtopBuffer ;
               trim( input.groupName ) ;
               input.snapshotModeChooser = GROUP ;
               input.forcedToRefresh_Global = REFRESH ;
            }
            noecho() ;
            curs_set( 0 ) ;
         }
         else if( JUMPTYPE_NODE == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the HostName:svcname : " ;
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 ) ;
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
            move( row - 1, 0 ) ;
            clrtobot() ;
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               input.nodeName = sdbtopBuffer ;
               trim( root.input.nodeName ) ;
               input.snapshotModeChooser = NODE ;
               input.forcedToRefresh_Global = REFRESH ;
            }
            noecho() ;
            curs_set( 0 ) ;
         }
         else if( JUMPTYPE_ASC== hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the column name to sort by asc : " ;
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 ) ;
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
            move( row - 1, 0 ) ;
            clrtobot() ;
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               input.sortingWay = SORTINGWAY_ASC ;
               displayName = sdbtopBuffer ;
               trim( displayName ) ;
               matchSourceFieldByDisplayName( displayName ) ;
               input.forcedToRefresh_Global = REFRESH ;
            }
            noecho() ;
            curs_set( 0 ) ;
         }
         else if( JUMPTYPE_DESC == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the column name to sort by desc : ";
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 );
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE );
            move( row - 1, 0 );
            clrtobot();
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               input.sortingWay = SORTINGWAY_DESC ;
               displayName = sdbtopBuffer ;
               trim( displayName ) ;
               matchSourceFieldByDisplayName( displayName ) ;
               input.forcedToRefresh_Global = REFRESH ;
            }
            noecho() ;
            curs_set( 0 ) ;
         }
         else if( JUMPTYPE_FILTER_CONDITION == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the filter condition(eg: TID:12345) : ";
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 ) ;
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
            move( row - 1, 0 ) ;
            clrtobot() ;
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               input.filterCondition = sdbtopBuffer ;
               trim( input.filterCondition ) ;
               input.forcedToRefresh_Global = REFRESH ;
            }
            noecho() ;
            curs_set( 0 ) ;
         }
         else if( JUMPTYPE_NO_FILTER_CONDITION == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            input.filterCondition = NULLSTRING;
            input.forcedToRefresh_Global = REFRESH ;
         }
         else if( JUMPTYPE_FILTER_NUMBER == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the filter number(the number is additive) : ";
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 ) ;
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
            move( row - 1, 0 ) ;
            clrtobot() ;
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               displayName = sdbtopBuffer ;
               trim( displayName ) ;
               rc = strToNum( displayName.c_str(), filterNum ) ;
               if( rc )
               {
                 filterNum = 0 ;
                 rc = SDB_OK ;
               }
               input.filterNumber += filterNum ;
               if( 0 > input.filterNumber )
               {
                  input.filterNumber = 0 ;
               }
               input.forcedToRefresh_Global = REFRESH ;
            }
            noecho() ;
            curs_set( 0 ) ;
         }
         else if( JUMPTYPE_NO_FILTER_NUMBER == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            if( 0 == root.input.filterNumber )
               goto done ;
            input.filterNumber = 0;
            input.forcedToRefresh_Global = REFRESH ;
         }
         else if( JUMPTYPE_REFRESHINTERVAL == hotKey->jumpType )
         {
            if( BODYTYPE_NORMAL != activatedPanel->bodyPanelType )
            {
               rc = eventManagement( BUTTON_H_LOWER, FALSE ) ;
               goto done ;
            }
            note = "please input the refresh interval(eg: 5) : ";
            getmaxyx( stdscr, row, col ) ;
            curs_set( 2 ) ;
            ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
            move( row - 1, 0 ) ;
            clrtobot() ;
            echo() ;
            mvprintw( row - 1 , ( col - note.length() ) / 2, note.c_str() ) ;
            if( BUTTON_ESC == getnstr( sdbtopBuffer, BUFFERSIZE ) )
            {
               input.forcedToRefresh_Local = REFRESH ;
            }
            else
            {
               displayName = sdbtopBuffer ;
               trim( displayName ) ;
               rc = strToNum( displayName.c_str(), refreshInterval ) ;
               if( rc || 0 > refreshInterval )
               {
                 rc = SDB_OK ;
               }
               else
               {
                  input.refreshInterval = refreshInterval ;
                  input.forcedToRefresh_Global = REFRESH ;
               }
            }
            noecho() ;
            curs_set( 0 ) ;
         }
      }
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 Event::refreshAll( HeadTailMap *header, BodyMap *body,
                         HeadTailMap *footer, BOOLEAN refreshAferClean )
{
   INT32 rc = SDB_OK ;
   clear() ;
   if( refreshAferClean )
   {
      refresh() ;
   }
   rc = refreshHT( header ) ;
   if( rc )
   {
      goto error ;
   }
   rc = refreshBD( body ) ;
   if( rc )
   {
      goto error ;
   }
   rc = refreshHT( footer ) ;
   if( rc )
   {
      goto error ;
   }
   refresh() ;
done:
   return rc ;
error:
   goto done ;
}

struct timeval waitTime ;
struct timeval startTime ;
struct timeval endTime ;
INT32 Event::runSDBTOP( BOOLEAN useSSL )
{
   INT32 rc = SDB_OK ;
   HeadTailMap* header = NULL ;
   HeadTailMap* footer = NULL ;
   INT64 key = 0 ; // the operation need to do
   fd_set fds ;

   INT32 maxfd  = STDIN + 1 ;

   root.input.forcedToRefresh_Global = NOTREFRESH ;
   root.input.forcedToRefresh_Local= NOTREFRESH ;

   rc = storeRootWindow( root ) ;
   if( rc )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s readConfiguration failed"OSS_NEWLINE,
                   errStrBuf ) ;
      goto error ;
   }
   rc = addFixedHotKey() ;
   if( rc )
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength, "%s addFixedHotKey failed"OSS_NEWLINE,
                   errStrBuf ) ;
      goto error ;
   }
   rc = assignActivatedPanel( &root.input.activatedPanel, BODYTYPE_MAIN) ;
   if( rc )
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s assignActivatedPanel failed"OSS_NEWLINE, errStrBuf ) ;
      goto error ;
   }
   try
   {
      coord = new(std::nothrow) sdb( useSSL ) ;
      if ( NULL == coord )
      {
         rc = SDB_OOM ;
         goto error;
      }
      rc = coord->connect( hostname.c_str(), serviceName.c_str(),
                          usrName.c_str(), password.c_str() ) ;
   }
   catch( std::exception &e )
   {
      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength, "%s can't connect to the coord:"
                   "e.what() =%d"OSS_NEWLINE, errStrBuf, e.what() ) ;
      rc = SDB_ERROR ;
      goto error ;

   }
   if( rc )
   {

      ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s can't connect to the coord: "
                   "%s, %s, %s, %s, rc =%d"OSS_NEWLINE,
                   errStrBuf, hostname.c_str(), serviceName.c_str(),
                   usrName.c_str(), password.c_str(), rc ) ;
      goto error ;
   }
   root.input.displayModeChooser = 0 ;
   initAllColourPairs() ;
   while( 1 )
   {
      gettimeofday( &startTime, 0 ) ;
      rc = getActivatedHeadTailMap( root.input.activatedPanel, &header, &footer) ;
      if( rc )
      {

         ossSnprintf( errStrBuf, errStrLength,"%s", errStr ) ;
         ossSnprintf( errStr, errStrLength,
                   "%s getActivatedHeadTailMap failed"OSS_NEWLINE,
                   errStrBuf ) ;
         goto error ;
      }
      if( BODYTYPE_NORMAL == root.input.activatedPanel->bodyPanelType )
      {
         rc = getCurSnapshot() ;
         if( rc )
         {
            goto error ;
         }
      }
      rc = refreshAll( header, root.input.activatedPanel, footer, TRUE ) ;
      if( rc )
      {
         goto error ;
      }
      waitTime.tv_sec = root.input.refreshInterval ;
      waitTime.tv_usec = 0 ;

      gettimeofday( &endTime, 0 ) ;

      waitTime.tv_sec -= ( endTime.tv_sec - startTime.tv_sec ) ;
      waitTime.tv_usec -= ( endTime.tv_usec - startTime.tv_usec ) ;
      if( 0 > waitTime.tv_sec )
      {
         waitTime.tv_sec = 0 ;
         waitTime.tv_usec = 0 ;
      }
      else if( 0 > waitTime.tv_usec )
      {
         if( 1 > waitTime.tv_sec )
         {
            waitTime.tv_usec += waitTime.tv_sec * 1000000 ;
            if( 0 > waitTime.tv_usec )
            {
               waitTime.tv_usec = 0 ;
            }
            waitTime.tv_sec = 0 ;
         }
         else
         {
            waitTime.tv_sec -= 1 ;
            waitTime.tv_usec += 1000000 ;
         }
      }

      while( 1 )
      {
         FD_ZERO ( &fds ) ;
         FD_SET ( STDIN, &fds ) ;
         rc = select ( maxfd, &fds, NULL, NULL, &waitTime ) ;
         if( rc < 0 )
         {
            rc = refreshAll( header, root.input.activatedPanel, footer, FALSE ) ;
            if( rc )
            {
               goto error ;
            }
            break ;
         }
         else if( rc > 0 )
         {
            if ( FD_ISSET ( STDIN, &fds ) )
            {
               ossMemset( sdbtopBuffer, 0, BUFFERSIZE ) ;
               read( STDIN, sdbtopBuffer, BUFFERSIZE ) ;
               rc = getSdbTopKey( sdbtopBuffer, key) ;
               if( rc )
               {
                  goto error ;
               }
               rc = eventManagement( key, TRUE ) ;
               if( rc )
               {
                  if( SDB_SDBTOP_DONE == rc )
                  {
                     goto done ;
                  }
                  goto error ;
               }
            }
         }
         else //time out
         {
            break ;
         }
         if( REFRESH == root.input.forcedToRefresh_Global )
         {
            root.input.forcedToRefresh_Global = NOTREFRESH ;
            break ;
         }
         if( REFRESH == root.input.forcedToRefresh_Local )
         {
            rc = refreshAll( header, root.input.activatedPanel, footer, FALSE ) ;
            if( rc )
            {
               goto error ;
            }
            root.input.forcedToRefresh_Local = NOTREFRESH ;
         }
      }
   }
done :
   return rc ;
error :
   goto done ;
}


void init ( po::options_description &desc )
{
   ADD_PARAM_OPTIONS_BEGIN ( desc )
      COMMANDS_OPTIONS
#ifdef SDB_SSL
      ( OPTION_SSL, "use SSL connection" )
#endif
   ADD_PARAM_OPTIONS_END
}

void displayArg ( po::options_description &desc )
{
   std::cout << desc << std::endl ;
}

void displayVersion()
{
   INT32 version = 0 ;
   INT32 subVersion = 0 ;
   INT32 fixedVersion = 0 ;
   INT32 release = 0 ;
   const CHAR *pBuild = NULL ;
   CHAR strVersion[BUFFERSIZE] = { 0 } ;

   ossGetVersion( &version, &subVersion, &fixedVersion, &release, &pBuild ) ;

   ossSnprintf( strVersion, BUFFERSIZE, "SequoiaDB version %d.%d.%d"OSS_NEWLINE
                "Release: %d"OSS_NEWLINE"%s", version, subVersion,
                fixedVersion, release, pBuild ) ;

   std::cout << strVersion << std::endl ;
}

INT32 resolveArgument ( po::options_description &desc,
                        INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   INT32 pathLen = 0 ;
   po::variables_map vm ;
   try
   {
      po::store ( po::parse_command_line ( argc, argv, desc ), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      std::cerr <<  "Unknown argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      std::cerr <<  "Invalid argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch( po::error &e )
   {
      std::cerr << e.what () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( vm.count ( OPTION_HELP ) )
   {
      displayArg ( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( vm.count( OPTION_VERSION ) )
   {
      displayVersion();
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if( vm.count( OPTION_CONFPATH ) )
   {
      confPath = vm[OPTION_CONFPATH].as<string>();
   }
   else
   {
      pathLen = ossStrlen( progPath ) + ossStrlen( SDBTOP_DEFAULT_CONFPATH ) ;
      if ( OSS_MAX_PATHSIZE < pathLen )
      {
         ossPrintf( "The program's path is too long"OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncat( progPath, SDBTOP_DEFAULT_CONFPATH, ossStrlen(SDBTOP_DEFAULT_CONFPATH) ) ;
      progPath[pathLen] = 0 ;
      confPath = std::string(progPath) ;
   }

   if( vm.count( OPTION_HOSTNAME ) )
   {
      hostname= vm[OPTION_HOSTNAME].as<string>();
   }
   if( vm.count( OPTION_SERVICENAME) )
   {
      serviceName = vm[OPTION_SERVICENAME].as<string>();
   }
   if( vm.count( OPTION_USRNAME) )
   {
      usrName = vm[OPTION_USRNAME].as<string>();
   }
   else
   {
      usrName = NULLSTRING ;
   }
   if( vm.count( OPTION_PASSWORD) )
   {
      password= vm[OPTION_PASSWORD].as<string>();
   }
   else
   {
      password = NULLSTRING ;
   }

#ifdef SDB_SSL
   if( vm.count ( OPTION_SSL ) )
   {
      useSSL = TRUE ;
   }
#endif

done :
   return rc ;
error :
   goto done ;
}


INT32 main( INT32 argc, CHAR **argv)
{
   INT32 rc = 0 ;
   Event sdbtop ;
   po::options_description desc ( "Command options" ) ;
   init ( desc ) ;
   rc = getProgramPath( progPath, OSS_MAX_PATHSIZE ) ;
   if ( rc )
   {
      ossPrintf( "Failed to get program's path"OSS_NEWLINE ) ;
      goto error ;
   }
   rc = resolveArgument ( desc, argc, argv ) ;
   if ( rc )
   {
      if ( SDB_PMD_HELP_ONLY != rc )
      {
         std::cerr<< "Error: Invalid arguments"OSS_NEWLINE ;
         displayArg ( desc ) ;
      }
      goto done ;
   }
   initscr() ;
   if( !has_colors() )
   {
      ossSnprintf( errStrBuf, errStrLength, "%s", errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s Your terminal can't support color"OSS_NEWLINE,
                   errStrBuf ) ;
      rc = SDB_ERROR ;
      goto error ;
   }
   start_color() ;
   cbreak() ;
   keypad( stdscr, FALSE ) ;
   noecho() ;
   curs_set( 1 ) ;
   rc = sdbtop.runSDBTOP( useSSL ) ;
   if( rc && SDB_SDBTOP_DONE != rc )
   {
      ossSnprintf( errStrBuf, errStrLength, "%s",  errStr ) ;
      ossSnprintf( errStr, errStrLength,
                   "%s can't runSDBTOP"OSS_NEWLINE,
                   errStrBuf ) ;
      goto error ;
   }
done :
   curs_set( 1 ) ;
   endwin() ;
   std::cerr << errStr << std::endl;
   return rc ;
error :
   goto done ;
}
