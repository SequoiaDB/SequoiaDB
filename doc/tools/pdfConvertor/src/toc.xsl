<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="2.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:outline="http://wkhtmltopdf.org/outline"
                xmlns="http://www.w3.org/1999/xhtml">
  <xsl:output doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN"
              doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
              indent="yes" />
  <xsl:template match="outline:outline">
    <html>
      <head>
        <title>Table of Contents</title>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
      </head>
      <body style="padding: 10px">
        <p class="h_toc">目录</p>
        <ul class="ul_toc" id="ul_toc_top" name="ul_toc_top"><xsl:apply-templates select="outline:item/outline:item"/></ul>
      </body>
    </html>
  </xsl:template>
  <xsl:template match="outline:item">
    <li class="li_toc">
      <xsl:if test="@title!=''">
        <div class="div_toc">
          <a class="toc_item">
            <xsl:if test="@link">
              <xsl:attribute name="href"><xsl:value-of select="@link"/></xsl:attribute>
            </xsl:if>
            <xsl:if test="@backLink">
              <xsl:attribute name="name"><xsl:value-of select="@backLink"/></xsl:attribute>
            </xsl:if>
            <xsl:value-of select="@title" /> 
          </a>
            <span class="span_toc">
              <a class="toc_page">
                <xsl:if test="@link">
                  <xsl:attribute name="href"><xsl:value-of select="@link"/></xsl:attribute>
                </xsl:if>
                <xsl:if test="@backLink">
                  <xsl:attribute name="name"><xsl:value-of select="@backLink"/></xsl:attribute>
                </xsl:if>
                <xsl:value-of select="@page" />
              </a>
            </span>
        </div>
      </xsl:if>
      <ul class="ul_toc">
        <xsl:comment>added to prevent self-closing tags in QtXmlPatterns</xsl:comment>
        <xsl:apply-templates select="outline:item"/>
      </ul>
    </li>
  </xsl:template>
</xsl:stylesheet>
