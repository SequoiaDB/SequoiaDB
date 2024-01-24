<?xml version="1.0" encoding="ISO-8859-1"?>

<!--
    This is an XSL document that translates BEAM XML complaints
    into BEAM textual complaints. This document is simply an
    example of how to translate the BEAM XML into a format
    of your choice.
    
    The BEAM XML file that this translates was tested when
    the BEAM XML output conformed to this DTD:
    
      https://w3.eda.ibm.com/beam/beam_results_1.1.dtd
      
    To test this XSL, install the "xsltproc" command from
    http://xmlsoft.org, and run this command:
    
      xsltproc this_xsl_file.xsl beams_xml_output.xml

    The result should be displayed on standard out, and it
    should resemble BEAM's default textual output.
-->

<!--
    Set up some useful shortcuts, like "&nl;" to output a literal newline.
-->

<!DOCTYPE xsl:stylesheet [
<!ENTITY qt '<xsl:text xmlns:xsl="http://www.w3.org/1999/XSL/Transform">&quot;</xsl:text>'>
<!ENTITY nl '<xsl:text xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
</xsl:text>'>
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                              xmlns:beam="http://w3.eda.ibm.com/beam/">

 <xsl:output method="text" media-type="text/plain" omit-xml-declaration="yes"/>


 <!--
     Match the root document and recurse.
 -->

 <xsl:template match="/">
  <xsl:apply-templates/>
 </xsl:template>


 <!--
     Match the "beam:results" node and output the BEAM header
     
       BEAM_VERSION=
       BEAM_ROOT=
       BEAM_BUILD_ROOT=
       BEAM_DIRECTORY_WRITE_INNOCENTS=
       BEAM_DIRECTORY_WRITE_ERRORS=

    Then, process the nested elements.
 -->

 <xsl:template match="beam:results">
  <xsl:text>BEAM_VERSION=</xsl:text>
  <xsl:value-of select="@version"/>
  &nl;
  
  <xsl:text>BEAM_ROOT=</xsl:text>
  <xsl:value-of select="beam:root-dir"/>
  &nl;
  
  <xsl:text>BEAM_BUILD_ROOT=</xsl:text>
  <xsl:value-of select="beam:build-root"/>
  &nl;
  
  <xsl:text>BEAM_DIRECTORY_WRITE_INNOCENTS=</xsl:text>
  <xsl:value-of select="beam:innocent-dir"/>
  &nl;
  
  <xsl:text>BEAM_DIRECTORY_WRITE_ERRORS=</xsl:text>
  <xsl:value-of select="beam:error-dir"/>
  &nl;
  
  <xsl:apply-templates/>
 </xsl:template>


 <!--
     Match the "beam:complaint" node and output
     
       (dash dash) ERRORX    /* comment */    >>>Innocent_Code
       
     Then, process the nested elements.
 -->

 <xsl:template match="beam:complaint">
  &nl;
  <xsl:text>-- </xsl:text>
  <xsl:value-of select="@type"/>
  
  <xsl:text>     /*</xsl:text>
  <xsl:value-of select="@comment"/>
  <xsl:text>*/     </xsl:text>
  
  <xsl:text>&gt;&gt;&gt;</xsl:text>
  <xsl:value-of select="@innocent-code"/>
  &nl;
  
  <xsl:apply-templates/>
 </xsl:template>


 <!--
     Match "beam:msg" nodes of type "user" and output either
     
       "file", line X: Some Text
       
     or just

       Some Text
     
     depending on whether the "beam:msg" node has a file and
     line or not.
       
     Indentation is controlled with the xsl parameter "indent", which is a
     string to be printed as the indentation before a line.
     
     Extra newlines can be printed if the xsl parameter "extra_newlines"
     is set to 1.
 -->

 <xsl:template match="beam:msg[@type='user']">
  <xsl:param name="indent" select="''"/>
  <xsl:param name="extra_newlines" select="0"/>

  <xsl:value-of select="$indent"/>

  <xsl:if test="@file and @line">
   &qt;
   <xsl:value-of select="@file"/>
   &qt;
  
   <xsl:text>, line </xsl:text>
   <xsl:value-of select="@line"/>
   <xsl:text>: </xsl:text>
  </xsl:if>
  
  <xsl:value-of select="text()"/>
  &nl;
  
  <xsl:if test="$extra_newlines and following-sibling::beam:msg">
   &nl;
  </xsl:if>
 </xsl:template>
 
 
 <!--
     Match a "beam:msg" node that has a compiler interpretation in it.
     
     Output
     
       You Typed: ...
       After Macro Expansion: ...
       Compiler Interpretation: ...
       
     Indentation is controlled with the xsl parameter "indent", which is a
     string to be printed as the indentation before a line.

     Extra newlines can be printed if the xsl parameter "extra_newlines"
     is set to 1.
 -->

 <xsl:template match="beam:msg[beam:typed]">
  <xsl:param name="indent" select="''"/>
  <xsl:param name="extra_newlines" select="0"/>

  <xsl:if test="beam:typed">
   <xsl:value-of select="$indent"/>
   <xsl:text>You Typed:               </xsl:text>
   <xsl:value-of select="beam:typed/text()"/>
   &nl;
  </xsl:if>
  
  <xsl:if test="beam:expanded">
   <xsl:value-of select="$indent"/>
   <xsl:text>After Macro Expansion:   </xsl:text>
   <xsl:value-of select="beam:expanded/text()"/>
   &nl;
  </xsl:if>

  <xsl:if test="beam:interpretation">
   <xsl:value-of select="$indent"/>
   <xsl:text>Compiler Interpretation: </xsl:text>
   <xsl:value-of select="beam:interpretation/text()"/>
   &nl;
  </xsl:if>
 
  <xsl:if test="$extra_newlines and following-sibling::beam:msg">
   &nl;
  </xsl:if>
 </xsl:template>

 <!--
     Match the "beam:path" node and output
     
       ONE POSSIBLE PATH LEADING TO THE ERROR:

     Then, process the nested elements, indenting them with a space.
 -->
 
 <xsl:template match="beam:path">
   <xsl:text>ONE POSSIBLE PATH LEADING TO THE ERROR:</xsl:text>&nl;
   <xsl:apply-templates>
    <xsl:with-param name="indent" select="' '"/>
   </xsl:apply-templates>
 </xsl:template>
 
 
 <!--
     Match the "beam:explanation" node and output nested elements,
     indenting them with a space, and separating them with an
     extra newline.
 -->
 
 <xsl:template match="beam:explanation">
  &nl;
  <xsl:apply-templates>
   <xsl:with-param name="indent" select="' '"/>
   <xsl:with-param name="extra_newlines" select="1"/>
  </xsl:apply-templates>
 </xsl:template>
 
 
 <!--
     Match the "beam:source" node and output
     
       Offending source line:
         the_source_line;
 -->
 
 <xsl:template match="beam:source">
  &nl;
  <xsl:text>Offending source line:</xsl:text>
  &nl;

  <xsl:value-of select="text()"/>
  &nl;
 </xsl:template>
 
 
 <!--
     Ignore all other nodes.
     The additional "text()" match is for older xsltproc
     versions, which don't match "node()" to text nodes.
 -->
 <xsl:template match="node()">
   <xsl:apply-templates/>
 </xsl:template>
 <xsl:template match="text()">
   <xsl:apply-templates/>
 </xsl:template>
</xsl:stylesheet>
