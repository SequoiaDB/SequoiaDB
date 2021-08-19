/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sqlGrammar.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SQLGRAMMAR_HPP_
#define SQLGRAMMAR_HPP_

#include "core.hpp"
#include "oss.hpp"

/// safe to share grammar across threads.
#define BOOST_SPIRIT_THREADSAFE
/// set the start limit
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT 20

#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_core.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS ;
using namespace boost::spirit ;

namespace engine
{
typedef const CHAR *  iteratorT ;
typedef tree_parse_info<iteratorT> SQL_AST ;
typedef tree_match<iteratorT>::container_t SQL_CONTAINER ;
typedef SQL_CONTAINER::const_iterator SQL_CON_ITR ;

#define SQL_PARSE( sql, grammar ) \
        ast_parse( sql, grammar )

#define SQL_RULE( ID ) \
        rule<ScannerT, parser_context<>, parser_tag<ID> >

#define SQL_BLANK no_node_d[+blank_p]
#define SQL_BLANKORNO no_node_d[*blank_p]

   struct _sqlGrammar : public grammar<_sqlGrammar>, SDBObject
   {
         /// sql type start
      const static INT32 SQL = 0 ;
      const static INT32 SELECT = 1 ;
      const static INT32 INSERT = 2 ;
      const static INT32 UPDATE = 3 ;
      const static INT32 DELETE_ = 4 ;
      const static INT32 CRTCS = 5 ;
      const static INT32 DROPCS = 6 ;
      const static INT32 CRTCL = 7 ;
      const static INT32 DROPCL = 8 ;
      const static INT32 CRTINDEX = 9 ;
      const static INT32 DROPINDEX = 10 ;    //--10
      const static INT32 LISTCS = 11 ;
      const static INT32 LISTCL = 12 ;
         /// sql type end

         /// term start
      const static INT32   WHERE = 13 ;
      const static INT32   SET = 14 ;
      const static INT32   AND = 15 ;
      const static INT32   OR = 16 ;
      const static INT32   AS = 17 ;
      const static INT32   VALUES = 18 ;
      const static INT32   ON = 19 ;
      const static INT32   UNIQUE = 20 ;       //--20
      const static INT32   DESC = 21 ;
      const static INT32   ASC = 22 ;
      const static INT32   EG = 23 ;
      const static INT32   NE = 24 ;
      const static INT32   LT = 25 ;
      const static INT32   GT = 26 ;
      const static INT32   GTE = 27 ;
      const static INT32   LTE = 28 ;
      const static INT32   LBRACKETS = 29 ;
      const static INT32   RBRACKETS = 30 ;       //--30
      const static INT32   COMMA =31 ;
      const static INT32   WILDCARD = 32 ;
      const static INT32   INNERJOIN = 33 ;
      const static INT32   L_OUTERJOIN = 34 ;
      const static INT32   R_OUTERJOIN = 35 ;
      const static INT32   F_OUTERJOIN = 36 ;
      const static INT32   FROM = 37 ;
      const static INT32   ORDERBY = 38 ;
      const static INT32   HINT = 39 ;
      const static INT32   LIMIT = 40 ;             //--40
      const static INT32   OFFSET = 41 ;
      const static INT32   FUNC = 42 ;
      const static INT32   GROUPBY = 43 ;
      const static INT32   BEGINTRAN = 44 ;
      const static INT32   ROLLBACK = 45 ;
      const static INT32   COMMIT = 46 ;
      const static INT32   DATE = 47 ;
      const static INT32   LIKE = 49 ;
      const static INT32   SPLITBY = 50 ;

      /// do not use IN coz windows
      const static INT32   INN = 51;
      const static INT32   NOT = 52;
      const static INT32   NULLL = 53;
      const static INT32   IS = 54;
      const static INT32   ISNOT = 55;
      const static INT32   OID = 56;
      const static INT32   TIMESTAMP = 57;
      const static INT32   DECIMAL = 58 ;
         /// trem end

      /// math start
      const static INT32 ADD = 80 ;
      const static INT32 SUB = 81 ;
      const static INT32 MULTIPLY = 82 ;
      const static INT32 DIVIDE = 83 ;
      const static INT32 MOD = 84 ;
      /// math end

         /// factor start
      const static INT32   DBATTR = 1000 ;
         /// factor end

         /// base type start
      const static INT32   DIGITAL = 1001 ;
      const static INT32   STR = 1002 ;
      const static INT32   BOOL_TRUE = 1003 ;
      const static INT32   BOOL_FALSE = 1004 ;
         /// base type end

      const static INT32   SQLMAX = 10000 ;

      template <typename ScannerT>
      struct definition
      {
         /// we define all the rules here.
         /// it can be defined in different files if necessary.

         SQL_RULE(SQL) sql ;
         SQL_RULE(SELECT) select ;
         SQL_RULE(INSERT) insert ;
         SQL_RULE(UPDATE) update ;
         SQL_RULE(DELETE_) del ;
         SQL_RULE(CRTCS) crtcs ;
         SQL_RULE(DROPCS) dropcs ;
         SQL_RULE(CRTCL) crtcl ;
         SQL_RULE(DROPCL) dropcl ;
         SQL_RULE(CRTINDEX) crtindex ;
         SQL_RULE(DROPINDEX) dropindex ;
         SQL_RULE(LISTCS) listcs ;
         SQL_RULE(LISTCL) listcl ;

         SQL_RULE(AS) as ;
         SQL_RULE(UNIQUE) unique ;
         SQL_RULE(ON) on ;
         SQL_RULE(LIMIT) limit ;
         SQL_RULE(OFFSET) offset ;
         SQL_RULE(ASC) asc ;
         SQL_RULE(DESC) desc ;
         SQL_RULE(WHERE) where ;
         SQL_RULE(FROM) from ;
         SQL_RULE(HINT) hint ;
         SQL_RULE(EG) eg ;
         SQL_RULE(NE) ne ;
         SQL_RULE(GT) gt ;
         SQL_RULE(LT) lt ;
         SQL_RULE(GTE) gte ;
         SQL_RULE(LTE) lte ;
         SQL_RULE(OR) orr ;
         SQL_RULE(AND) andd ;
         SQL_RULE(LBRACKETS) lbrackets ;
         SQL_RULE(RBRACKETS) rbrackets ;
         SQL_RULE(SET) set ;
         SQL_RULE(COMMA) comma ;
         SQL_RULE(WILDCARD) wildcard ;
         SQL_RULE(INNERJOIN) innerj ;
         SQL_RULE(L_OUTERJOIN) louterj ;
         SQL_RULE(R_OUTERJOIN) routerj ;
         SQL_RULE(F_OUTERJOIN) fouterj ;
         SQL_RULE(ORDERBY) orderby ;
         SQL_RULE(GROUPBY) groupby ;
         SQL_RULE(FUNC) func ;
         SQL_RULE(BEGINTRAN) begintran ;
         SQL_RULE(ROLLBACK) rollback ;
         SQL_RULE(COMMIT) commit ;
         SQL_RULE(DATE) date ;
         SQL_RULE(LIKE) like ;
         SQL_RULE(INN) in;
         SQL_RULE(NOT) nott;
         SQL_RULE(SPLITBY) splitby ;
         SQL_RULE(NULLL) nulll ;
         SQL_RULE(IS) is;
         SQL_RULE(ISNOT) isnot ;
         SQL_RULE(OID) oid ;
         SQL_RULE(TIMESTAMP) timestamp ;
         SQL_RULE(DECIMAL) decimal ;

         SQL_RULE(DBATTR) dbattr ;

         SQL_RULE(DIGITAL) digital ;
         SQL_RULE(STR) str ;
         SQL_RULE(BOOL_TRUE) bool_true ;
         SQL_RULE(BOOL_FALSE) bool_false ;

         SQL_RULE(ADD) add ;
         SQL_RULE(SUB) sub ;
         SQL_RULE(MULTIPLY) multiply ;
         SQL_RULE(DIVIDE) divide ;
         SQL_RULE(MOD) mod ;

         /// logical rule for parsing.
         rule<ScannerT> graph ;
         rule<ScannerT> wCondition ;
         rule<ScannerT> oCondition ;
         rule<ScannerT> wFactor ;
         rule<ScannerT> oFactor ;
         rule<ScannerT> assign ;
         rule<ScannerT> selectors ;
         rule<ScannerT> selector ;
         rule<ScannerT> fDomain ;
         rule<ScannerT> fSrc ;
         rule<ScannerT> iFields ;
         rule<ScannerT> iValues ;
         rule<ScannerT> iSrc ;
         rule<ScannerT> oField ;
         rule<ScannerT> oFields ;
         rule<ScannerT> partition ;
         rule<ScannerT> dbattrchar ;
         rule<ScannerT> inFactor ;
         rule<ScannerT> expr_factor ;
         rule<ScannerT> expr_group ;
         rule<ScannerT> expr;
         rule<ScannerT> commValue ;

         const SQL_RULE(SQL) &start() const
         {
            return sql ;
         }

         definition( _sqlGrammar const &self )
         {
            digital = leaf_node_d[
                          lexeme_d[!(ch_p('+')|ch_p('-'))>>+digit_p >>
                          !('.'>>+digit_p)>>!(((ch_p('e')|ch_p('E')) >>
                          !(ch_p('+')|ch_p('-')))>>+digit_p)]
                          ];

            bool_true = as_lower_d[ str_p("true") ] ;

            bool_false = as_lower_d[ str_p("false") ] ;

            graph = (anychar_p - ( ch_p('"')|ch_p('\0')|ch_p('\'')))
                    | str_p("\\\\")
                    | str_p("\\\"")
                    | str_p("\\'") ;

            str =  ( inner_node_d[ch_p('"')
                               >> leaf_node_d[+graph]
                               >> ch_p('"')])
                   | ( no_node_d[ch_p('"')] >> no_node_d[ch_p('"')])
                   | ( inner_node_d[ch_p('\'')
                               >> leaf_node_d[+graph]
                               >> ch_p('\'')])
                   | ( no_node_d[ch_p('\'')] >> no_node_d[ch_p('\'')]);

            dbattrchar = anychar_p - ( ch_p('=')
                                       |ch_p('<')
                                       |ch_p('>')
                                       |ch_p('(')
                                       |ch_p(')')
                                       |ch_p('*')
                                       |ch_p(';')
                                       |ch_p(',')
                                       |ch_p('"')
                                       |ch_p('\'')
                                       |space_p
                                       |ch_p('\0')
                                       |ch_p('-')
                                       |ch_p('+')
                                       |ch_p('/')
                                       |ch_p('%')) ;

            dbattr = leaf_node_d[ lexeme_d[+(dbattrchar)] ] ;

            as = as_lower_d[str_p("as")] ;

            unique = as_lower_d[str_p("unique")] ;

            asc = as_lower_d[str_p("asc")] ;

            desc = as_lower_d[str_p("desc")] ;

            orr = as_lower_d[str_p("or")] ;

            andd = as_lower_d[str_p("and")] ;

            nott = as_lower_d[str_p("not")] ;

            eg = ch_p('=') ;

            ne = str_p("<>") ;

            lt = ch_p('<') ;

            gt = ch_p('>') ;

            lte = str_p("<=") ;

            gte = str_p(">=") ;

            like = as_lower_d[str_p("like")] ;

            in = as_lower_d[str_p("in")];

            nulll = as_lower_d[str_p("null")] ;

            is = as_lower_d[str_p("is")] ;

            isnot = leaf_node_d[ is >> SQL_BLANKORNO >> nott ] ;

            lbrackets = ch_p('(') ;

            rbrackets = ch_p(')') ;

            comma = ch_p(',') ;

            wildcard = ch_p('*') ;

            begintran = no_node_d[as_lower_d[str_p("begin")]]
                        >> SQL_BLANK
                        >> no_node_d[as_lower_d[str_p("transaction")]] ;

            rollback = no_node_d[as_lower_d[str_p("rollback")]] ;

            commit = no_node_d[as_lower_d[str_p("commit")]] ;

            add = ch_p('+') ;

            sub = ch_p('-') ;

            multiply = ch_p('*') ;

            divide = ch_p('/') ;

            mod = ch_p('%') ;

            commValue = digital | str | nulll | oid |
                        date | timestamp | decimal |
                        bool_true | bool_false ;

            expr_factor = func | digital | str | dbattr
                          | (inner_node_d[lbrackets
                                      >> SQL_BLANKORNO
                                      >> expr
                                      >> SQL_BLANKORNO
                                      >> rbrackets]) ;

            expr_group = expr_factor % ( SQL_BLANKORNO
                                        >> root_node_d[( multiply | divide | mod )]
                                        >> SQL_BLANKORNO ) ;

            expr = expr_group % ( SQL_BLANKORNO
                                  >> root_node_d[( add | sub )]
                                  >> SQL_BLANKORNO  ) ;

            func = leaf_node_d[dbattr
                               >> SQL_BLANKORNO
                               >> lbrackets
                               >> SQL_BLANKORNO
                               >> *( dbattr % ( SQL_BLANKORNO >> comma >> SQL_BLANKORNO ) )
                               >> SQL_BLANKORNO
                               >> rbrackets] ;

            date = root_node_d[as_lower_d[str_p("date")]]
                               >> SQL_BLANKORNO
                               >> no_node_d[lbrackets]
                               >> SQL_BLANKORNO
                               >> str
                               >> SQL_BLANKORNO
                               >> no_node_d[rbrackets] ;

            oid = root_node_d[as_lower_d[str_p("oid")]]
                               >> SQL_BLANKORNO
                               >> no_node_d[lbrackets]
                               >> SQL_BLANKORNO
                               >> str
                               >> SQL_BLANKORNO
                               >> no_node_d[rbrackets] ;

            timestamp = root_node_d[as_lower_d[str_p("timestamp")]]
                               >> SQL_BLANKORNO
                               >> no_node_d[lbrackets]
                               >> SQL_BLANKORNO
                               >> str
                               >> SQL_BLANKORNO
                               >> no_node_d[rbrackets] ;

            decimal = root_node_d[as_lower_d[str_p("decimal")]]
                               >> SQL_BLANKORNO
                               >> no_node_d[lbrackets]
                               >> SQL_BLANKORNO
                               >> digital
                               >> SQL_BLANKORNO
                               >> no_node_d[rbrackets] ;

            partition = no_node_d[as_lower_d[str_p("partition")]]
                        >> SQL_BLANK
                        >> no_node_d[as_lower_d[str_p("by")]]
                        >> SQL_BLANKORNO
                        >> no_node_d[lbrackets]
                        >> SQL_BLANKORNO
                        >> oFields
                        >> SQL_BLANKORNO
                        >> no_node_d[rbrackets] ;


            innerj = leaf_node_d[as_lower_d[str_p("inner")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("join")]] ;

            louterj = leaf_node_d[as_lower_d[str_p("left")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("outer")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("join")]] ;

            routerj = leaf_node_d[as_lower_d[str_p("right")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("outer")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("join")]] ;

            fouterj = leaf_node_d[as_lower_d[str_p("full")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("outer")]
                                 >> SQL_BLANK
                                 >> as_lower_d[str_p("join")]] ;

            oField = dbattr >> !( SQL_BLANK >> root_node_d[(asc|desc)]) ;

            oFields = oField % ( SQL_BLANKORNO
                                 >> root_node_d[comma]
                                 >> SQL_BLANKORNO ) ;

            orderby = root_node_d[leaf_node_d[as_lower_d[str_p("order")]
                                  >> SQL_BLANK
                                  >> as_lower_d[str_p("by")]]]
                      >> SQL_BLANK
                      >> oFields ;

            groupby = root_node_d[leaf_node_d[as_lower_d[str_p("group")]
                                  >> SQL_BLANK
                                  >> as_lower_d[str_p("by")]]]
                      >> SQL_BLANK
                      >> oFields ;

            splitby = root_node_d[leaf_node_d[as_lower_d[str_p("split")]
                                  >> SQL_BLANK
                                  >> as_lower_d[str_p("by")]]]
                      >> SQL_BLANK
                      >> oFields ;

            limit = root_node_d[as_lower_d[str_p("limit")]]
                    >> SQL_BLANK
                    >> digital ;

            offset = root_node_d[as_lower_d[str_p("offset")]]
                    >> SQL_BLANK
                    >> digital ;

            hint = root_node_d[str_p("/*+")]
                   >> SQL_BLANKORNO
                   >> func %(SQL_BLANK )
                   >> SQL_BLANKORNO
                   >> no_node_d[str_p("*/")] ;

            inFactor = no_node_d[lbrackets]
                       >> SQL_BLANKORNO
                       >> ( commValue )
                           % ( SQL_BLANKORNO
                               >> root_node_d[comma]
                               >> SQL_BLANKORNO )
                       >> SQL_BLANKORNO
                       >> no_node_d[rbrackets];

            /// lt and gt must be behind the others.
            wFactor = ( dbattr
                        >> SQL_BLANKORNO
                        >> root_node_d[(eg|lte|gte|ne|lt|gt)]
                        >> SQL_BLANKORNO
                        >> ( commValue | dbattr ) )
                      |( dbattr
                        >> SQL_BLANK
                        >> root_node_d[(isnot|is)]
                        >> SQL_BLANK
                        >> ( commValue | dbattr ) )
                      |( dbattr
                         >> SQL_BLANK
                         >> root_node_d[in]
                         >> SQL_BLANKORNO
                         >> inFactor )
                      |( dbattr
                         >>SQL_BLANK
                         >> root_node_d[like]
                         >>SQL_BLANK
                         >> str )
                      | (inner_node_d[lbrackets
                                      >> SQL_BLANKORNO
                                      >> wCondition
                                      >> SQL_BLANKORNO
                                      >> rbrackets])
                      | root_node_d[nott] >> SQL_BLANK >> wFactor ;

            oFactor = ( dbattr
                        >> SQL_BLANKORNO
                        >> root_node_d[(eg|lte|gte|ne|lt|gt)]
                        >> SQL_BLANKORNO
                        >> dbattr )
                      | (inner_node_d[lbrackets
                                      >> SQL_BLANKORNO
                                      >> oCondition
                                      >> SQL_BLANKORNO
                                      >> rbrackets]) ;

            wCondition =  wFactor % ( SQL_BLANK
                                     >> (root_node_d[(andd | orr)])
                                     >> SQL_BLANK ) ;

            oCondition = oFactor % ( SQL_BLANK
                                     >> (root_node_d[andd])
                                     >> SQL_BLANK ) ;

            fSrc = ( ( no_node_d[lbrackets]
                       >> SQL_BLANKORNO
                       >> select
                       >> SQL_BLANKORNO
                       >> no_node_d[rbrackets] )
                       >> !( SQL_BLANK
                             >> root_node_d[as]
                             >> SQL_BLANK
                             >> dbattr))
                   | ( dbattr
                       >> !( SQL_BLANK
                             >> root_node_d[as]
                             >> SQL_BLANK
                             >> dbattr ) );

            on = root_node_d[as_lower_d[str_p("on")]]
                 >> SQL_BLANK
                 >> oCondition ;

            fDomain = ( fSrc
                        >> SQL_BLANK
                        >> root_node_d[innerj |louterj|routerj |fouterj]
                        >> SQL_BLANK
                        >> fSrc
                        >> !( SQL_BLANK >> on ) )
                      | fSrc ;

            from = root_node_d[as_lower_d[str_p("from")]]
                   >> SQL_BLANK
                   >> fDomain ;

            selector =  expr
                        >> !( SQL_BLANK
                              >> root_node_d[as]
                              >> SQL_BLANK
                              >> dbattr ) ;

            selectors = wildcard
                        | ( selector % ( SQL_BLANKORNO
                            >> root_node_d[comma]
                            >> SQL_BLANKORNO )) ;

            where = root_node_d[as_lower_d[str_p("where")]]
                    >> SQL_BLANK
                    >> wCondition ;

            assign = dbattr >> SQL_BLANKORNO
                       >> root_node_d[eg] >> SQL_BLANKORNO
                       >> commValue ;

            set = no_node_d[as_lower_d[str_p("set")]]
                  >> SQL_BLANK
                  >> assign
                     % (SQL_BLANKORNO
                        >> root_node_d[comma]
                        >> SQL_BLANKORNO)  ;

            iFields = no_node_d[ lbrackets ]
                  >>  SQL_BLANKORNO
                  >>  dbattr % ( SQL_BLANKORNO
                              >> root_node_d[comma]
                              >> SQL_BLANKORNO )
                  >>  SQL_BLANKORNO
                  >>  no_node_d[ rbrackets ] ;

            iValues = no_node_d[ lbrackets ]
                   >> SQL_BLANKORNO
                   >> ( commValue ) % ( SQL_BLANKORNO
                                     >> root_node_d[comma]
                                     >> SQL_BLANKORNO )
                   >> SQL_BLANKORNO
                   >> no_node_d[ rbrackets ] ;

            iSrc = ( SQL_BLANKORNO
                     >> iFields
                     >> SQL_BLANK
                     >> no_node_d[ as_lower_d[str_p("values")] ]
                     >> SQL_BLANKORNO
                     >> iValues )
                   | ( SQL_BLANKORNO
                    >> no_node_d[ lbrackets ]
                    >> SQL_BLANKORNO
                    >> select
                    >> SQL_BLANKORNO
                    >> no_node_d[ rbrackets ] )
                   | ( SQL_BLANK >> select ) ;

            ////

            select = no_node_d[as_lower_d[str_p("select")]]
                     >> SQL_BLANK
                     >> selectors
                     >> SQL_BLANK
                     >> from
                     >> !( SQL_BLANK >> where )
                     >> !( SQL_BLANK >> groupby )
                     >> !( SQL_BLANK >> splitby )
                     >> !( SQL_BLANK >> orderby )
                     >> (
                          ( SQL_BLANK >> limit >> !( SQL_BLANK >> offset ) )
                         |( !( SQL_BLANK >> offset ) >> !( SQL_BLANK >> limit ) )
                        )
                     >> !( SQL_BLANK >> hint ) ;

            insert = no_node_d[as_lower_d[str_p("insert")]]
                     >> SQL_BLANK
                     >> no_node_d[as_lower_d[str_p("into")]]
                     >> SQL_BLANK
                     >> dbattr
                     >> ( ( SQL_BLANKORNO
                       >> iFields
                       >> SQL_BLANK
                       >> no_node_d[ as_lower_d[str_p("values")] ]
                       >> SQL_BLANKORNO
                       >> iValues )
                      | ( SQL_BLANKORNO
                       >> no_node_d[ lbrackets ]
                       >> SQL_BLANKORNO
                       >> select
                       >> SQL_BLANKORNO
                       >> no_node_d[ rbrackets ] )
                      | ( SQL_BLANK >> select ) ) ;

            update = no_node_d[as_lower_d[str_p("update")]]
                     >> SQL_BLANK
                     >> dbattr
                     >> SQL_BLANK
                     >> set
                     >> !( SQL_BLANK >> where )
                     >> !( SQL_BLANK >> hint ) ;

            del = no_node_d[as_lower_d[str_p("delete")]]
                  >> SQL_BLANK
                  >> no_node_d[as_lower_d[str_p("from")]]
                  >> SQL_BLANK
                  >> leaf_node_d[dbattr]
                  >> !( SQL_BLANK >> where ) ;

            crtindex = no_node_d[as_lower_d[str_p("create")]]
                       >> !(SQL_BLANK >> unique)
                       >> SQL_BLANK
                       >> no_node_d[as_lower_d[str_p("index")]]
                       >> SQL_BLANK
                       >> dbattr
                       >> SQL_BLANK
                       >> no_node_d[as_lower_d[str_p("on")]]
                       >> SQL_BLANK
                       >> dbattr
                       >> SQL_BLANKORNO
                       >> inner_node_d[ lbrackets
                                        >> (SQL_BLANKORNO
                                            >> oFields
                                            >> SQL_BLANKORNO )
                                        >> rbrackets ] ;

            dropindex = no_node_d[as_lower_d[str_p("drop")]
                                  >> SQL_BLANK
                                  >> as_lower_d[str_p("index")]]
                        >> SQL_BLANK
                        >> dbattr
                        >> SQL_BLANK
                        >> no_node_d[as_lower_d[str_p("on")]]
                        >> SQL_BLANK
                        >> dbattr ;

            crtcs = no_node_d[as_lower_d[str_p("create")]]
                    >> SQL_BLANK
                    >> no_node_d[as_lower_d[str_p("collectionspace")]]
                    >> SQL_BLANK
                    >> leaf_node_d[dbattr] ;

            /// hope to get: |--(id:6)
            ///                  |--(value:foo, id:44)
            /// but have no idea how.
            /// now we get:  |--(value:foo, id:6)
            /// the same to crtcs, crtcl, dropcl.
            dropcs = no_node_d[as_lower_d[str_p("drop")]]
                     >> SQL_BLANK
                     >>no_node_d[as_lower_d[str_p("collectionspace")]]
                     >> SQL_BLANK
                     >> leaf_node_d[dbattr] ;

            crtcl = no_node_d[as_lower_d[str_p("create")]]
                    >> SQL_BLANK
                    >> no_node_d[as_lower_d[str_p("collection")]]
                    >> SQL_BLANK
                    >> leaf_node_d[dbattr]
                    >> !(SQL_BLANK >> partition) ;

            dropcl = no_node_d[as_lower_d[str_p("drop")]]
                     >> SQL_BLANK
                     >> no_node_d[as_lower_d[str_p("collection")]]
                     >> SQL_BLANK
                     >> leaf_node_d[dbattr] ;

            listcs = no_node_d[
                     as_lower_d[str_p("list")]
                     >> SQL_BLANK >>
                     as_lower_d[str_p("collectionspaces")]
                     ] ;

            listcl = no_node_d[
                     as_lower_d[str_p("list")]
                     >> SQL_BLANK >>
                     as_lower_d[str_p("collections")]
                     ] ;

            sql = select | insert | update | del | crtindex | dropindex
                  | crtcl | dropcl | crtcs |dropcs | listcs| listcl
                  | begintran | rollback | commit ;
/*
#ifdef _DEBUG
            BOOST_SPIRIT_DEBUG_RULE(sql) ;
            BOOST_SPIRIT_DEBUG_RULE(insert) ;
            BOOST_SPIRIT_DEBUG_RULE(del) ;
            BOOST_SPIRIT_DEBUG_RULE(update) ;
            BOOST_SPIRIT_DEBUG_RULE(select) ;
            BOOST_SPIRIT_DEBUG_RULE(where) ;
            BOOST_SPIRIT_DEBUG_RULE(wCondition) ;
            BOOST_SPIRIT_DEBUG_RULE(wFactor) ;
            BOOST_SPIRIT_DEBUG_RULE(digital) ;
            BOOST_SPIRIT_DEBUG_RULE(str) ;
            BOOST_SPIRIT_DEBUG_RULE(selector) ;
            BOOST_SPIRIT_DEBUG_RULE(fDomain) ;
            BOOST_SPIRIT_DEBUG_RULE(on) ;
            BOOST_SPIRIT_DEBUG_RULE(crtindex) ;
            BOOST_SPIRIT_DEBUG_RULE(dropindex) ;
            BOOST_SPIRIT_DEBUG_RULE(dropcs) ;

#endif

            this->start_parserT( select, insert,
                                 update, del,
                                 crtindex, dropindex,
                                 crtcl, dropcl,
                                 crtcs, dropcs,
                                 listcs, listcl ) ;
*/

         }

      } ;
   } ;

   typedef struct _sqlGrammar SQL_GRAMMAR ;

}

#endif

