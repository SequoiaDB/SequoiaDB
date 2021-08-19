/******************************************************************************


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

   Source File Name = sptSPDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_SPDEF_HPP_
#define SPT_SPDEF_HPP_

#include "core.hpp"
#include "jsapi.h"

namespace engine
{
   #define SAFE_JS_FREE( cx, p ) \
      do { if ( p ) { JS_free( ( cx ), ( p ) ) ; ( p ) = NULL ; } } while ( 0 )

   #define SPT_EVAL_FLAG_NONE              0
   #define SPT_EVAL_FLAG_PRINT             0x00000001
   #define SPT_EVAL_FLAG_IGNORE_ERR_PREFIX 0x00000002

   #define SPT_ERR               "ErrMsg"

   #define SPT_GLOBAL_STR        "Global"

   /*
      SPT_PROP_ATTR define
   */
   /// visible to enumerate. for...in, for...each, JS_Enumerate
   #define SPT_PROP_ENUMERATE                JSPROP_ENUMERATE
   /// can't be set
   #define SPT_PROP_READONLY                 JSPROP_READONLY
   /// can't be deleted
   #define SPT_PROP_PERMANENT                JSPROP_PERMANENT

   #define SPT_PROP_DEFAULT                  (SPT_PROP_ENUMERATE|SPT_PROP_READONLY|SPT_PROP_PERMANENT)
   #define SPT_FUNC_DEFAULT                  (SPT_PROP_ENUMERATE)

   /*
      SPT_JS_OP_CODE define
   */
   enum SPT_JS_OP_CODE
   {
      SPT_JSOP_GETPROP        = 53,
      SPT_JSOP_SETPROP        = 54,
      SPT_JSOP_GETELEMENT     = 55,
      SPT_JSOP_CALL           = 58,
      SPT_JSOP_FUNAPPLY       = 78,
      SPT_JSOP_SETNAME        = 111,
      SPT_JSOP_SETGNAME       = 157,
      SPT_JSOP_CALLPROP       = 187,
      SPT_JSOP_GETXPROP       = 198,
      SPT_JSOP_GETTHISPROP    = 211,
      SPT_JSOP_GETARGPROP     = 212,
      SPT_JSOP_GETLOCALPROP   = 213,
      SPT_JSOP_LENGTH         = 223,
      SPT_JSOP_SETMETHOD      = 235,
      SPT_JSOP_FUNCALL        = 242
   } ;

   /*
      SPT_JSOP_GETPROP, SPT_JSOP_GETXPROP, SPT_JSOP_GETLOCALPROP,
      SPT_JSOP_GETARGPROP, SPT_JSOP_GETTHISPROP, SPT_JSOP_LENGTH

      with sptIsOpGetProperty() in sptCommon.hpp
   */

   /*
      SPT_JSOP_SETPROP, SPT_JSOP_SETGNAME, SPT_JSOP_SETNAME,
      SPT_JSOP_SETMETHOD

      with sptIsOpSetProperty() in sptCommon.hpp
   */

   /*
      SPT_JSOP_CALL, SPT_JSOP_FUNAPPLY, SPT_JSOP_FUNCALL

      SPT_JSOP_CALLPROP

      with sptIsOpCallProperty() in sptCommon.hpp
   */

   /*
      Bellow is the op define and value in js code

      1   JSOP_PUSH
      2   JSOP_POPV
      3   JSOP_ENTERWITH
      4   JSOP_LEAVEWITH
      5   JSOP_RETURN
      6   JSOP_GOTO
      7   JSOP_IFEQ
      8   JSOP_IFNE
      9   JSOP_ARGUMENTS
      10  JSOP_FORARG
      11  JSOP_FORLOCAL
      12  JSOP_DUP
      13  JSOP_DUP2
      14  JSOP_SETCONST
      15  JSOP_BITOR
      16  JSOP_BITXOR
      17  JSOP_BITAND
      18  JSOP_EQ
      19  JSOP_NE
      20  JSOP_LT
      21  JSOP_LE
      22  JSOP_GT
      23  JSOP_GE
      24  JSOP_LSH
      25  JSOP_RSH
      26  JSOP_URSH
      27  JSOP_ADD
      28  JSOP_SUB
      29  JSOP_MUL
      30  JSOP_DIV
      31  JSOP_MOD
      32  JSOP_NOT
      33  JSOP_BITNOT
      34  JSOP_NEG
      35  JSOP_POS
      36  JSOP_DELNAME
      37  JSOP_DELPROP
      38  JSOP_DELELEM
      39  JSOP_TYPEOF
      40  JSOP_VOID
      41  JSOP_INCNAME
      42  JSOP_INCPROP
      43  JSOP_INCELEM
      44  JSOP_DECNAME
      45  JSOP_DECPROP
      46  JSOP_DECELEM
      47  JSOP_NAMEINC
      48  JSOP_PROPINC
      49  JSOP_ELEMINC
      50  JSOP_NAMEDEC
      51  JSOP_PROPDEC
      52  JSOP_ELEMDEC
      53  JSOP_GETPROP    --
      54  JSOP_SETPROP    --
      55  JSOP_GETELEM
      56  JSOP_SETELEM
      57  JSOP_CALLNAME
      58  JSOP_CALL       --
      59  JSOP_NAME
      60  JSOP_DOUBLE
      61  JSOP_STRING
      62  JSOP_ZERO
      63  JSOP_ONE
      64  JSOP_NULL
      65  JSOP_THIS
      66  JSOP_FALSE
      67  JSOP_TRUE
      68  JSOP_OR
      69  JSOP_AND
      70  JSOP_TABLESWITCH
      71  JSOP_LOOKUPSWITCH
      72  JSOP_STRICTEQ
      73  JSOP_STRICTNE
      74  JSOP_SETCALL
      75  JSOP_ITER
      76  JSOP_MOREITER
      77  JSOP_ENDITER
      78  JSOP_FUNAPPLY
      79  JSOP_SWAP
      80  JSOP_OBJECT
      81  JSOP_POP
      82  JSOP_NEW
      83  JSOP_TRAP
      84  JSOP_GETARG
      85  JSOP_SETARG
      86  JSOP_GETLOCAL
      87  JSOP_SETLOCAL
      88  JSOP_UINT16
      89  JSOP_NEWINIT
      90  JSOP_NEWARRAY
      91  JSOP_NEWOBJECT
      92  JSOP_ENDINIT
      93  JSOP_INITPROP
      94  JSOP_INITELEM
      95  JSOP_DEFSHARP
      96  JSOP_USESHARP
      97  JSOP_INCARG
      98  JSOP_DECARG
      99  JSOP_ARGINC
      100 JSOP_ARGDEC
      101 JSOP_INCLOCAL
      102 JSOP_DECLOCAL
      103 JSOP_LOCALINC
      104 JSOP_LOCALDEC
      105 JSOP_IMACOP
      106 JSOP_FORNAME
      107 JSOP_FORPROP
      108 JSOP_FORELEM
      109 JSOP_POPN
      110 JSOP_BINDNAME
      111 JSOP_SETNAME
      112 JSOP_THROW
      113 JSOP_IN
      114 JSOP_INSTANCEOF
      115 JSOP_DEBUGGER
      116 JSOP_GOSUB
      117 JSOP_RETSUB
      118 JSOP_EXCEPTION
      119 JSOP_LINENO
      121 JSOP_CASE
      122 JSOP_DEFAULT
      123 JSOP_EVAL
      124 JSOP_ENUMELEM
      125 JSOP_GETTER
      126 JSOP_SETTER
      127 JSOP_DEFFUN
      128 JSOP_DEFCONST
      129 JSOP_DEFVAR
      130 JSOP_LAMBDA
      131 JSOP_CALLEE
      132 JSOP_SETLOCALPOP
      133 JSOP_PICK
      135 JSOP_FINALLY
      136 JSOP_GETFCSLOT
      137 JSOP_CALLFCSLOT
      138 JSOP_ARGSUB
      139 JSOP_ARGCNT
      140 JSOP_DEFLOCALFUN
      141 JSOP_GOTOX
      142 JSOP_IFEQX
      143 JSOP_IFNEX
      144 JSOP_ORX
      145 JSOP_ANDX
      146 JSOP_GOSUBX
      147 JSOP_CASEX
      148 JSOP_DEFAULTX
      149 JSOP_TABLESWITCHX
      150 JSOP_LOOKUPSWITCHX
      153 JSOP_THROWING
      154 JSOP_SETRVAL
      156 JSOP_GETGNAME
      157 JSOP_SETGNAME
      158 JSOP_INCGNAME
      159 JSOP_DECGNAME
      160 JSOP_GNAMEINC
      161 JSOP_GNAMEDEC
      162 JSOP_REGEXP
      163 JSOP_DEFXMLNS
      164 JSOP_ANYNAME
      165 JSOP_QNAMEPART
      166 JSOP_QNAMECONST
      167 JSOP_QNAME
      168 JSOP_TOATTRNAME
      169 JSOP_TOATTRVAL
      170 JSOP_ADDATTRNAME
      171 JSOP_ADDATTRVAL
      172 JSOP_BINDXMLNAME
      173 JSOP_SETXMLNAME
      174 JSOP_XMLNAME
      175 JSOP_DESCENDANTS
      176 JSOP_FILTER
      177 JSOP_ENDFILTER
      178 JSOP_TOXML
      179 JSOP_TOXMLLIST
      180 JSOP_XMLTAGEXPR
      181 JSOP_XMLELTEXPR
      182 JSOP_NOTRACE
      183 JSOP_XMLCDATA
      184 JSOP_XMLCOMMENT
      185 JSOP_XMLPI
      186 JSOP_DELDESC
      187 JSOP_CALLPROP   --
      188 JSOP_BLOCKCHAIN
      190 JSOP_UINT24
      191 JSOP_INDEXBASE
      192 JSOP_RESETBASE
      193 JSOP_RESETBASE0
      196 JSOP_CALLELEM
      197 JSOP_STOP
      198 JSOP_GETXPROP
      199 JSOP_CALLXMLNAME
      200 JSOP_TYPEOFEXPR
      201 JSOP_ENTERBLOCK
      202 JSOP_LEAVEBLOCK
      203 JSOP_IFPRIMTOP
      204 JSOP_PRIMTOP
      205 JSOP_GENERATOR
      206 JSOP_YIELD
      207 JSOP_ARRAYPUSH
      208 JSOP_GETFUNNS
      209 JSOP_ENUMCONSTELEM
      210 JSOP_LEAVEBLOCKEXPR
      211 JSOP_GETTHISPROP
      212 JSOP_GETARGPROP
      213 JSOP_GETLOCALPROP  --
      214 JSOP_INDEXBASE1
      215 JSOP_INDEXBASE2
      216 JSOP_INDEXBASE3
      217 JSOP_CALLGNAME
      218 JSOP_CALLLOCAL
      219 JSOP_CALLARG
      220 JSOP_BINDGNAME
      221 JSOP_INT8
      222 JSOP_INT32
      223 JSOP_LENGTH
      224 JSOP_HOLE
      225 JSOP_DEFFUN_FC
      226 JSOP_DEFLOCALFUN_FC
      227 JSOP_LAMBDA_FC
      228 JSOP_OBJTOP
      229 JSOP_TRACE
      230 JSOP_GETUPVAR_DBG
      231 JSOP_CALLUPVAR_DBG
      232 JSOP_DEFFUN_DBGFC
      233 JSOP_DEFLOCALFUN_DBGFC
      234 JSOP_LAMBDA_DBGFC
      235 JSOP_SETMETHOD
      236 JSOP_INITMETHOD
      237 JSOP_UNBRAND
      238 JSOP_UNBRANDTHIS
      239 JSOP_SHARPINIT
      240 JSOP_GETGLOBAL
      241 JSOP_CALLGLOBAL
      242 JSOP_FUNCALL
      243 JSOP_FORGNAME
   */

}

#endif // SPT_SPDEF_HPP_

