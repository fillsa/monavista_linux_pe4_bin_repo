/******************************************************************************
 *
 * $Id: latexgen.h,v 1.50 2001/03/19 19:27:41 root Exp $
 *
 * Copyright (C) 1997-2004 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby 
 * granted. No representations are made about the suitability of this software 
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#ifndef LATEXGEN_H
#define LATEXGEN_H

#include "outputgen.h"

class QFile;

class LatexGenerator : public OutputGenerator
{
  public:
    LatexGenerator();
   ~LatexGenerator();
    static void init();
    static void writeStyleSheetFile(QFile &f);
    static void writeHeaderFile(QFile &f);

    //OutputGenerator *copy();
    //OutputGenerator *clone() { return new LatexGenerator(*this); }
    //void append(const OutputGenerator *o);
    void enable() { active=TRUE; }
    void disable() { active=FALSE; }
    void enableIf(OutputType o)  { if (o==Latex) active=TRUE;  }
    void disableIf(OutputType o) { if (o==Latex) active=FALSE; }
    void disableIfNot(OutputType o) { if (o!=Latex) active=FALSE; }
    bool isEnabled(OutputType o) { return (o==Latex && active); } 
    OutputGenerator *get(OutputType o) { return (o==Latex) ? this : 0; }

    void printDoc(DocNode *);

    void startFile(const char *name,const char *manName,const char *title);
    void writeFooter() {}
    void endFile();
    void clearBuffer();
    
    //void writeIndex();
    void startIndexSection(IndexSections);
    void endIndexSection(IndexSections);
    void startProjectNumber();
    void endProjectNumber() {}
    void writeStyleInfo(int part);
    void startTitleHead(const char *);
    void startTitle();
    void endTitleHead(const char *,const char *name);
    void endTitle()   { t << "}"; }

    void newParagraph();
    void writeString(const char *text);
    void startIndexList() { t << "\\begin{CompactList}"    << endl; }
    void endIndexList()   { t << "\\end{CompactList}"      << endl; }
    void startIndexKey();
    void endIndexKey();
    void startIndexValue(bool);
    void endIndexValue(const char *,bool);
    void startItemList()  { t << "\\begin{CompactItemize}" << endl; }
    void endItemList()    { t << "\\end{CompactItemize}"   << endl; }
    //void startEnumList()  { t << "\\begin{enumerate}"      << endl; }
    //void endEnumList()    { t << "\\end{enumerate}"        << endl; }
    //void startAlphabeticalIndexList() {}
    //void endAlphabeticalIndexList() {} 
    //void writeIndexHeading(const char *) {}
    void writeIndexItem(const char *ref,const char *file,const char *name);
    void docify(const char *text);
    void codify(const char *text);
    void writeObjectLink(const char *ref,const char *file,
                         const char *anchor,const char *name);
    void writeCodeLink(const char *ref, const char *file,
                       const char *anchor,const char *name);
    void startTextLink(const char *,const char *);
    void endTextLink();
    void startHtmlLink(const char *url);
    void endHtmlLink();
    //void writeMailLink(const char *url);
    void startTypewriter() { t << "{\\tt "; }
    void endTypewriter()   { t << "}";      }
    void startGroupHeader();
    void endGroupHeader();
    void writeListItem() { t << "\\item " << endl; }

    void startMemberSections() {}
    void endMemberSections() {} 
    void startMemberHeader();
    void endMemberHeader();
    void startMemberSubtitle() {}
    void endMemberSubtitle() {}
    void startMemberList();
    void endMemberList();
    void startAnonTypeScope(int);
    void endAnonTypeScope(int);
    void startMemberItem(int);
    void endMemberItem();

    void startMemberGroupHeader(bool);
    void endMemberGroupHeader();
    void startMemberGroupDocs();
    void endMemberGroupDocs();
    void startMemberGroup();
    void endMemberGroup(bool);
    
    void insertMemberAlign() {}

    void writeRuler() { t << endl << endl; /*t << "\\vspace{0.4cm}\\hrule\\vspace{0.2cm}" << endl; */ }
    void writeAnchor(const char *fileName,const char *name);
    void startCodeFragment() { t << endl << endl << "\\footnotesize\\begin{verbatim}"; }
    void endCodeFragment()   { t << "\\end{verbatim}\\normalsize " << endl; }
    //void startPreFragment()  { t << "\\small\\begin{alltt}"; 
    //                           insidePre=TRUE; 
    //                         }
    //void endPreFragment()    { t << "\\end{alltt}\\normalsize " << endl; 
    //                           insidePre=FALSE; 
    //                         }
    //void startVerbatimFragment() { t << endl << endl << "\\footnotesize\\begin{verbatim}"; }
    //void endVerbatimFragment()   { t << "\\end{verbatim}\\normalsize " << endl; }
    void writeLineNumber(const char *,const char *,const char *,int l) { t << l << " "; }
    void startCodeLine() { col=0; }
    void endCodeLine() { codify("\n"); }
    //void writeBoldString(const char *text) 
    //                     { t << "{\\bf "; docify(text); t << "}"; }
    void startEmphasis() { t << "{\\em ";  }
    void endEmphasis()   { t << "}"; }
    void startBold()     { t << "{\\bf "; }
    void endBold()       { t << "}"; }
    void startDescription();
    void endDescription();
    void startDescItem();
    void endDescItem();
    void lineBreak() { t << "\\par\n"; }
    void startMemberDoc(const char *,const char *,const char *,const char *);
    void endMemberDoc(bool);
    void startDoxyAnchor(const char *,const char *,const char *,const char *);
    void endDoxyAnchor(const char *,const char *);
    void startCodeAnchor(const char *) {}
    void endCodeAnchor() {}
    void writeChar(char c);
    void writeLatexSpacing() { t << "\\hspace{0.3cm}"; }
    //void writeLatexLabel(const char *scope,const char *anchor);
    void writeStartAnnoItem(const char *type,const char *file, 
                            const char *path,const char *name);
    void writeEndAnnoItem(const char *name);
    void startSubsection() { t << "\\subsection*{"; }
    void endSubsection() { t << "}" << endl; }
    void startSubsubsection() { t << "\\subsubsection*{"; }
    void endSubsubsection() { t << "}" << endl; }
    void startCenter()      { t << "\\begin{center}" << endl; }
    void endCenter()        { t << "\\end{center}" << endl; }
    void startSmall()       { t << "\\footnotesize "; }
    void endSmall()         { t << "\\normalsize "; }
    //void startSubscript()   { t << "$_{\\mbox{"; }
    //void endSubscript()     { t << "}}$"; }
    //void startSuperscript() { t << "$^{\\mbox{"; }
    //void endSuperscript()   { t << "}}$"; }
    //void startTable(bool hasCaption,int c)  
    //                        { 
    //                          if (hasCaption) t << "\\begin{table}[h]";
    //                          t << "\\begin{TabularC}{" << c << "}\n\\hline\n"; 
    //                        }
    //void endTable(bool hasCaption)         
    //                        { 
    //                          if (hasCaption)
    //                          {
    //                            t << "\\end{table}\n";
    //                          }
    //                          else
    //                          {
    //                            t << "\\\\\\hline\n\\end{TabularC}\n"; 
    //                          }
    //                        }
    //void startCaption()     { t << "\\\\\\hline\n\\end{TabularC}\n"
    //                               "\\centering\n\\caption{"; 
    //                        }
    //void endCaption()       { t << "}\n"; }
    //void nextTableRow()     {}
    //void endTableRow()      { t << "\\\\\\hline\n"; }
    //void nextTableColumn()  { t << "&"; }
    //void endTableColumn()   {}
    //void writeCopyright()    { t << "\\copyright"; }
    //void writeQuote()        { t << "''"; }
    //void writeUmlaut(char c) { if (c=='i') t << "\\\"{\\i}"; else 
    //                                       t << "\\\"{" << c << "}"; 
    //                         }
    //void writeAcute(char c)  { if (c=='i') t << "\\'{\\i}"; else
    //                                       t << "\\'{"  << c << "}"; 
    //                         }
    //void writeGrave(char c)  { if (c=='i') t << "\\`{\\i}"; else
    //                                       t << "\\`{"  << c << "}"; 
    //                         }
    //void writeCirc(char c)   { if (c=='i') t << "\\^{\\i}"; else
    //                                       t << "\\^{"  << c << "}"; 
    //                         }
    //void writeTilde(char c)  { t << "\\~{"  << c << "}"; }
    //void writeRing(char c)   { t << "\\" << c << c; }
    //void writeSharpS()       { t << "\"s"; }
    //void writeCCedil(char c) { t << "\\c{" << c << "}"; }
    void startMemberDescription() { t << "\\begin{CompactList}\\small\\item\\em "; }
    void endMemberDescription() { t << "\\item\\end{CompactList}"; }
    void startDescList(SectionTypes)     { t << "\\begin{Desc}\n\\item["; }
    void endDescList()       { t << "\\end{Desc}" << endl; }
    void startSimpleSect(SectionTypes,const char *,const char *,const char *);
    void endSimpleSect();
    void startParamList(ParamListTypes,const char *title);
    void endParamList();
    //void endDescTitle()      { t << "]"; }
    void writeDescItem()     { t << "\\par" << endl; }
    void startSection(const char *,const char *,SectionInfo::SectionType);
    void endSection(const char *,SectionInfo::SectionType);
    //void writeSectionRef(const char *,const char *,const char *,const char *);
    //void writeSectionRefItem(const char *,const char *,const char *);
    //void writeSectionRefAnchor(const char *,const char *,const char *);
    void addIndexItem(const char *,const char *);
    void startIndent()       {}
    void endIndent()         {}
    void writeSynopsis()     {}
    //void generateExternalIndex() {}
    void startClassDiagram();
    void endClassDiagram(ClassDiagram &,const char *,const char *);
    //void startColorFont(uchar,uchar,uchar) {}
    //void endColorFont()   {}
    void startPageRef();
    void endPageRef(const char *,const char *);
    //void startQuickIndexItem(const char *,const char *) {}
    //void endQuickIndexItem() {}
    void writeQuickLinks(bool,HighlightedItem) {}
    //void writeFormula(const char *,const char *);
    void writeNonBreakableSpace(int);
    //void startImage(const char *,const char *,bool);
    //void endImage(bool);
    //void startDotFile(const char *,bool);
    //void endDotFile(bool);
    
    void startDescTable()
    { t << "\\begin{description}" << endl; }
    void endDescTable()
    { t << "\\end{description}" << endl; }
    void startDescTableTitle()
    { t << "\\item[{\\em " << endl; }
    void endDescTableTitle()
    { t << "}]"; }
    void startDescTableData() {}
    void endDescTableData() {}
    void lastIndexPage() {}

    //static void docifyStatic(QTextStream &t,const char *str);

    void startDotGraph();
    void endDotGraph(DotClassGraph &);
    void startInclDepGraph();
    void endInclDepGraph(DotInclDepGraph &);
    void startCallGraph();
    void endCallGraph(DotCallGraph &);
    void writeGraphicalHierarchy(DotGfxHierarchyTable &) {}

    void startTextBlock(bool) {}
    void endTextBlock(bool) {}

    void startMemberDocPrefixItem() {}
    void endMemberDocPrefixItem() {}
    void startMemberDocName(bool) {}
    void endMemberDocName() {}
    void startParameterType(bool,const char *) {}
    void endParameterType() {}
    void startParameterName(bool) {}
    void endParameterName(bool,bool,bool) {}
    void startParameterList(bool) {}
    void endParameterList() {}

    void startFontClass(const char *) {}
    void endFontClass() {}

    //void startHtmlOnly()  {}
    //void endHtmlOnly()    {}
    //void startLatexOnly() {}
    //void endLatexOnly()   {}
    
    //void startSectionRefList();
    //void endSectionRefList();

    void writeCodeAnchor(const char *) {}

  private:
    LatexGenerator(const LatexGenerator &);
    LatexGenerator &operator=(const LatexGenerator &);
    int col;
    bool insideTabbing;
    bool firstDescItem;
    bool insidePre;
    QCString relPath;
};

#endif
