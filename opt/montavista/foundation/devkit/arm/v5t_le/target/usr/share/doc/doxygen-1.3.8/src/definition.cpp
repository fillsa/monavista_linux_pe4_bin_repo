/******************************************************************************
 *
 * $Id: index.cpp,v 1.30 1999/03/06 22:15:39 root Exp $
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

#include "qtbc.h"
#include <ctype.h>
#include <qregexp.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "definition.h"
#include "doxygen.h"
#include "language.h"
#include "message.h"
#include "outputlist.h"
#include "code.h"
#include "util.h"
#include "groupdef.h"
#include "section.h"

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define popen _popen
#define pclose _pclose
#endif

static void addToMap(const char *name,Definition *d)
{
  QCString symbolName = name;
  int index=computeQualifiedIndex(symbolName);
  if (index!=-1) symbolName=symbolName.mid(index+2);
  if (!symbolName.isEmpty()) 
  {
    DefinitionList *dl=Doxygen::symbolMap->find(symbolName);
    if (dl==0)
    {
      dl = new DefinitionList;
      Doxygen::symbolMap->append(symbolName,dl);
    }
    //printf("******* adding symbol `%s'\n",symbolName.data());
    dl->append(d);
    d->setSymbolName(symbolName);
  }
}

static void removeFromMap(Definition *d)
{
  QCString symbolName = d->symbolName();
  int index=computeQualifiedIndex(symbolName);
  if (index!=-1) symbolName=symbolName.mid(index+2);
  if (!symbolName.isEmpty()) 
  {
    //printf("******* removing symbol `%s'\n",symbolName.data());
    DefinitionList *dl=Doxygen::symbolMap->find(symbolName);
    if (dl)
    {
      ASSERT(dl!=0);
      bool b = dl->removeRef(d);
      ASSERT(b==TRUE);
    }
  }
}

Definition::Definition(const char *df,int dl,
                       const char *name,const char *b,
                       const char *d,bool isSymbol)
{
  //QCString ns;
  m_defFileName = df;
  m_defLine = dl;
  m_name=name; 
  if (m_name!="<globalScope>") 
  {
    //extractNamespaceName(m_name,m_localName,ns);
    m_localName=stripScope(name);
  }
  else
  {
    m_localName=name;
  }
  //printf("m_localName=%s\n",m_localName.data());
  m_brief=b; 
  m_doc=d; 
  m_sectionDict=0, 
  m_startBodyLine=m_endBodyLine=-1, 
  m_bodyDef=0;
  m_sourceRefByDict=0;
  m_sourceRefsDict=0;
  m_todoId=0;
  m_testId=0;
  m_bugId=0;
  m_deprecatedId=0;
  m_outerScope=Doxygen::globalScope;
  m_partOfGroups=0;
  m_xrefListItems=0;
  m_briefLine=1;
  m_briefFile=(QCString)"<"+name+">";
  m_docLine=1;
  m_docFile=(QCString)"<"+name+">";
  m_isSymbol = isSymbol;
  if (m_isSymbol) addToMap(name,this);
}

Definition::~Definition()
{
  if (m_isSymbol) removeFromMap(this);
  delete m_sectionDict;
  delete m_sourceRefByDict;
  delete m_sourceRefsDict;
  delete m_partOfGroups;
  delete m_xrefListItems;
}

void Definition::addSectionsToDefinition(QList<SectionInfo> *anchorList)
{
  if (!anchorList) return;
  //printf("%s: addSectionsToDefinition(%d)\n",name().data(),anchorList->count());
  SectionInfo *si=anchorList->first();
  while (si)
  {
    //printf("Add section `%s' to definition `%s'\n",
    //    si->label.data(),name().data());
    SectionInfo *gsi=Doxygen::sectionDict.find(si->label);
    if (gsi==0)
    {
      gsi = new SectionInfo(*si);
      Doxygen::sectionDict.insert(si->label,gsi);
    }
    if (m_sectionDict==0) 
    {
      m_sectionDict = new SectionDict(17);
    }
    if (m_sectionDict->find(gsi->label)==0)
    {
      m_sectionDict->insert(gsi->label,gsi);
      gsi->definition = this;
    }
    si=anchorList->next();
  }
}

void Definition::writeDocAnchorsToTagFile()
{
  if (!Config_getString("GENERATE_TAGFILE").isEmpty() && m_sectionDict)
  {
    //printf("%s: writeDocAnchorsToTagFile(%d)\n",name().data(),m_sectionDict->count());
    QDictIterator<SectionInfo> sdi(*m_sectionDict);
    SectionInfo *si;
    for (;(si=sdi.current());++sdi)
    {
      if (!si->generated)
      {
        //printf("write an entry!\n");
        if (definitionType()==TypeMember) Doxygen::tagFile << "  ";
        Doxygen::tagFile << "    <docanchor>" << si->label << "</docanchor>" << endl;
      }
    }
  }
}

void Definition::setDocumentation(const char *d,const char *docFile,int docLine,bool stripWhiteSpace) 
{ 
  if (d==0) return;
  //printf("Definition::setDocumentation(%s,%s,%d,%d)\n",d,docFile,docLine,stripWhiteSpace);
  QCString doc;
  if (stripWhiteSpace)
  {
    // strip leading empty lines in front of the text, but not the 
    // leading spaces in front of the first line, so list items are 
    // parsed with the correct indent
    const char *p=d;
    char c;
    int s=0,so=0;
    while ((c=*p) && (c==' ' || c=='\r' || c=='\n'))
    {
      if (c=='\n') so=s; 
      p++;
      s++;
    }
    if (c=='\0') return;
    doc=d+so;
    // zero any trailing whitespace
    int e=doc.length()-1;
    while (e>=0 && (c=doc.at(e)) && (c==' ' || c=='\r' || c=='\n'))
    {
      doc.at(e)='\0';
      e--;
    }
  }
  else // don't strip whitespace
  {
    doc=d;
  }
  //printf("setting docs for %s: `%s'\n",name().data(),m_doc.data());
  m_doc=doc;
  m_docFile = docFile;
  m_docLine = docLine;
}

#define uni_isupper(c) (QChar(c).category()==QChar::Letter_Uppercase)

void Definition::setBriefDescription(const char *b,const char *briefFile,int briefLine) 
{ 
  if (b==0) return;
  //printf("Definition::setBriefDescription(%s,%s,%d)\n",b,briefFile,briefLine);
  m_brief=QCString(b).stripWhiteSpace();
  int bl=m_brief.length(); 
  if (bl>0) // add puntuation if needed
  {
    switch(m_brief.at(bl-1))
    {
      case '.': case '!': case '?': break;
      default: 
        if (uni_isupper(m_brief.at(0))) m_brief+='.'; 
        break;
    }
  }
  m_briefFile = briefFile;
  m_briefLine = briefLine;
}

/*! Reads a fragment of code from file \a fileName starting at 
 * line \a startLine and ending at line \a endLine (inclusive). The fragment is
 * stored in \a result. If FALSE is returned the code fragment could not be
 * found.
 *
 * The file is scanned for a opening bracket ('{') from \a startLine onward.
 * The line actually containing the bracket is returned via startLine.
 * The file is scanned for a closing bracket ('}') from \a endLine backward.
 * The line actually containing the bracket is returned via endLine.
 */
static bool readCodeFragment(const char *fileName,
                      int &startLine,int &endLine,QCString &result)
{
  //printf("readCodeFragment(%s,%d,%d)\n",fileName,startLine,endLine);
  if (fileName==0 || fileName[0]==0) return FALSE; // not a valid file name
  QCString cmd=getFileFilter(fileName)+" \""+fileName+"\"";
  FILE *f = Config_getBool("FILTER_SOURCE_FILES") ? popen(cmd,"r") : fopen(fileName,"r");
  bool found=FALSE;
  if (f)
  {
    int c=0;
    int col=0;
    int lineNr=1;
    // skip until the startLine has reached
    while (lineNr<startLine && !feof(f))
    {
      while ((c=fgetc(f))!='\n' && c!=EOF) /* skip */;
      lineNr++; 
    }
    if (!feof(f))
    {
      // skip until the opening bracket or lonely : is found
      char cn=0;
      while (lineNr<=endLine && !feof(f) && !found)
      {
        while ((c=fgetc(f))!='{' && c!=':' && c!=EOF) 
        {
          //printf("parsing char `%c'\n",c);
          if (c=='\n') 
          {
            lineNr++,col=0; 
          }
          else if (c=='\t') 
          {
            col+=Config_getInt("TAB_SIZE") - (col%Config_getInt("TAB_SIZE"));
          }
          else
          {
            col++;
          }
        }
        if (c==':')
        {
          cn=fgetc(f);
          if (cn!=':') found=TRUE;
        }
        else if (c=='{')
        {
          found=TRUE;
        }
      }
      //printf(" -> readCodeFragment(%s,%d,%d) lineNr=%d\n",fileName,startLine,endLine,lineNr);
      if (found) 
      {
        // For code with more than one line,
        // fill the line with spaces until we are at the right column
        // so that the opening brace lines up with the closing brace
        if (endLine!=startLine)
        {
          QCString spaces;
          spaces.fill(' ',col);
          result+=spaces;
        }
        // copy until end of line
        result+=c;
        if (c==':') result+=cn;
        startLine=lineNr;
        const int maxLineLength=4096;
        char lineStr[maxLineLength];
        do 
        {
          //printf("reading line %d in range %d-%d\n",lineNr,startLine,endLine);
          int size_read;
          do 
          {
            // read up to maxLineLength-1 bytes, the last byte being zero
            char *p = fgets(lineStr, maxLineLength,f);
            //printf("  read %s",p);
            if (p) 
            {
              size_read=qstrlen(p); 
            }
            else 
            {
              size_read=-1;
            }
            result+=lineStr;
          } while (size_read == (maxLineLength-1));

          lineNr++; 
        } while (lineNr<=endLine && !feof(f));

        // strip stuff after closing bracket
        int newLineIndex = result.findRev('\n');
        int braceIndex   = result.findRev('}');
        if (braceIndex > newLineIndex) 
        {
          result.truncate(braceIndex+1);
        }
        endLine=lineNr-1;
      }
    }
    if (Config_getBool("FILTER_SOURCE_FILES")) pclose(f); else fclose(f);
  }
  return found;
}

/*! Write a reference to the source code defining this definition */
void Definition::writeSourceDef(OutputList &ol,const char *)
{
  ol.pushGeneratorState();
  //printf("Definition::writeSourceRef %d %p\n",bodyLine,bodyDef);
  if (Config_getBool("SOURCE_BROWSER") && m_startBodyLine!=-1 && m_bodyDef)
  {
    QCString refText = theTranslator->trDefinedAtLineInSourceFile();
    int lineMarkerPos = refText.find("@0");
    int fileMarkerPos = refText.find("@1");
    if (lineMarkerPos!=-1 && fileMarkerPos!=-1) // should always pass this.
    {
      QCString lineStr,anchorStr;
      lineStr.sprintf("%d",m_startBodyLine);
      anchorStr.sprintf("l%05d",m_startBodyLine);
      if (lineMarkerPos<fileMarkerPos) // line marker before file marker
      {
        // write text left from linePos marker
        ol.parseText(refText.left(lineMarkerPos)); 
        ol.disableAllBut(OutputGenerator::Html); 
        // write line link (HTML only)
        ol.writeObjectLink(0,m_bodyDef->getSourceFileBase(),
            anchorStr,lineStr);
        ol.enableAll();
        ol.disable(OutputGenerator::Html);
        // write normal text (Latex/Man only)
        ol.docify(lineStr);
        ol.enableAll();
        
        // write text between markers
        ol.parseText(refText.mid(lineMarkerPos+2,
              fileMarkerPos-lineMarkerPos-2));

        ol.disableAllBut(OutputGenerator::Html); 
        // write file link (HTML only)
        ol.writeObjectLink(0,m_bodyDef->getSourceFileBase(),
            0,m_bodyDef->name());
        ol.enableAll();
        ol.disable(OutputGenerator::Html);
        // write normal text (Latex/Man only)
        ol.docify(m_bodyDef->name());
        ol.enableAll();
        
        // write text right from file marker
        ol.parseText(refText.right(
              refText.length()-fileMarkerPos-2)); 
      }
      else // file marker before line marker
      {
        // write text left from file marker
        ol.parseText(refText.left(fileMarkerPos)); 
        ol.disableAllBut(OutputGenerator::Html); 
        // write file link (HTML only)
        ol.writeObjectLink(0,m_bodyDef->getSourceFileBase(),
            0,m_bodyDef->name());
        ol.enableAll();
        ol.disable(OutputGenerator::Html);
        // write normal text (Latex/Man only)
        ol.docify(m_bodyDef->name());
        ol.enableAll();
        
        // write text between markers
        ol.parseText(refText.mid(fileMarkerPos+2,
              lineMarkerPos-fileMarkerPos-2)); 

        ol.disableAllBut(OutputGenerator::Html); 
        // write line link (HTML only)
        ol.writeObjectLink(0,m_bodyDef->getSourceFileBase(),
            anchorStr,lineStr);
        ol.enableAll();
        ol.disable(OutputGenerator::Html);
        // write normal text (Latex/Man only)
        ol.docify(lineStr);
        ol.enableAll();

        // write text right from linePos marker
        ol.parseText(refText.right(
              refText.length()-lineMarkerPos-2)); 
      }
    }
    else
    {
      err("Error: translation error: invalid markers in trDefinedInSourceFile()\n");
    }

    ol.disableAllBut(OutputGenerator::RTF);
    ol.newParagraph();
    ol.enableAll();
  }
  ol.popGeneratorState();
}

/*! Write code of this definition into the documentation */
void Definition::writeInlineCode(OutputList &ol,const char *scopeName)
{
  ol.pushGeneratorState();
  //printf("Source Fragment %s: %d-%d bodyDef=%p\n",name().data(),
  //        m_startBodyLine,m_endBodyLine,m_bodyDef);
  if (Config_getBool("INLINE_SOURCES") && m_startBodyLine!=-1 && 
      m_endBodyLine>=m_startBodyLine && m_bodyDef)
  {
    QCString codeFragment;
    int actualStart=m_startBodyLine,actualEnd=m_endBodyLine;
    if (readCodeFragment(m_bodyDef->absFilePath(),
          actualStart,actualEnd,codeFragment)
       )
    {
      initParseCodeContext();
      //printf("Read:\n`%s'\n\n",codeFragment.data());
      if (definitionType()==TypeMember) setParameterList((MemberDef *)this);
      ol.startCodeFragment();
      parseCode(ol,scopeName,codeFragment,FALSE,0,
          m_bodyDef,actualStart,actualEnd,TRUE);
      ol.endCodeFragment();
      ol.newParagraph();
    }
  }
  ol.popGeneratorState();
}

/*! Write a reference to the source code fragments in which this 
 *  definition is used.
 */
void Definition::writeSourceRefList(OutputList &ol,const char *scopeName,
    const QCString &text,MemberSDict *members,bool /*funcOnly*/)
{
  ol.pushGeneratorState();
  if (Config_getBool("SOURCE_BROWSER") && members)
  {
    ol.newParagraph();
    ol.parseText(text);
    ol.docify(" ");

    QCString ldefLine=theTranslator->trWriteList(members->count());

    QRegExp marker("@[0-9]+");
    int index=0,newIndex,matchLen;
    // now replace all markers in inheritLine with links to the classes
    while ((newIndex=marker.match(ldefLine,index,&matchLen))!=-1)
    {
      bool ok;
      ol.parseText(ldefLine.mid(index,newIndex-index));
      uint entryIndex = ldefLine.mid(newIndex+1,matchLen-1).toUInt(&ok);
      MemberDef *md=members->at(entryIndex);
      if (ok && md)
      {
        QCString scope=md->getScopeString();
        QCString name=md->name();
        //printf("class=%p scope=%s scopeName=%s\n",md->getClassDef(),scope.data(),scopeName);
        if (!scope.isEmpty() && scope!=scopeName)
        {
          if (Config_getBool("OPTIMIZE_OUTPUT_JAVA"))
          {
            name.prepend(scope+".");
          }
          else
          {
            name.prepend(scope+"::");
          }
        }
        if (!md->isObjCMethod() &&
            (md->isFunction() || md->isSlot() || 
             md->isPrototype() || md->isSignal()
            )
           ) name+="()";
        //Definition *d = md->getOutputFileBase();
        //if (d==Doxygen::globalScope) d=md->getBodyDef();
        if (md->getStartBodyLine()!=-1 && md->getBodyDef()) 
        {
          //printf("md->getBodyDef()=%p global=%p\n",md->getBodyDef(),Doxygen::globalScope); 
          // for HTML write a real link
          ol.pushGeneratorState();
          ol.disableAllBut(OutputGenerator::Html);
          QCString lineStr,anchorStr;
          anchorStr.sprintf("l%05d",md->getStartBodyLine());
          //printf("Write object link to %s\n",md->getBodyDef()->getSourceFileBase().data());
          ol.writeObjectLink(0,md->getBodyDef()->getSourceFileBase(),anchorStr,name);
          ol.popGeneratorState();

          // for the other output formats just mention the name
          ol.pushGeneratorState();
          ol.disable(OutputGenerator::Html);
          ol.docify(name);
          ol.popGeneratorState();
        }
        else if (md->isLinkable() /*&& d && d->isLinkable()*/)
        {
          // for HTML write a real link
          ol.pushGeneratorState();
          ol.disableAllBut(OutputGenerator::Html);
          ol.writeObjectLink(md->getReference(),
                             md->getOutputFileBase(),
                             md->anchor(),name);
          ol.popGeneratorState();

          // for the other output formats just mention the name
          ol.pushGeneratorState();
          ol.disable(OutputGenerator::Html);
          ol.docify(name);
          ol.popGeneratorState();
        }
        else
        {
          ol.docify(name);
        }
      }
      index=newIndex+matchLen;
    } 
    ol.parseText(ldefLine.right(ldefLine.length()-index));
    ol.writeString(".");
  }
  ol.popGeneratorState();
}

void Definition::writeSourceReffedBy(OutputList &ol,const char *scopeName)
{
  writeSourceRefList(ol,scopeName,theTranslator->trReferencedBy(),m_sourceRefByDict,FALSE);
}

void Definition::writeSourceRefs(OutputList &ol,const char *scopeName)
{
  writeSourceRefList(ol,scopeName,theTranslator->trReferences(),m_sourceRefsDict,TRUE);
}

bool Definition::hasDocumentation() const
{ 
  static bool extractAll = Config_getBool("EXTRACT_ALL"); 
  return !m_doc.isEmpty() ||             // has detailed docs
         !m_brief.isEmpty() ||           // has brief description
         extractAll;                     // extract everything
}

void Definition::addSourceReferencedBy(MemberDef *md)
{
  if (md)
  {
    QCString name  = md->name();
    QCString scope = md->getScopeString();

    if (!scope.isEmpty())
    {
      name.prepend(scope+"::");
    }

    if (m_sourceRefByDict==0)
    {
      m_sourceRefByDict = new MemberSDict;
    }
    if (m_sourceRefByDict->find(name)==0)
    {
      m_sourceRefByDict->inSort(name,md);
    }
  }
}

void Definition::addSourceReferences(MemberDef *md)
{
  if (md)
  {
    QCString name  = md->name();
    QCString scope = md->getScopeString();

    if (!scope.isEmpty())
    {
      name.prepend(scope+"::");
    }

    if (m_sourceRefsDict==0)
    {
      m_sourceRefsDict = new MemberSDict;
    }
    if (m_sourceRefsDict->find(name)==0)
    {
      //printf("Adding reference %s->%s\n",md->name().data(),name.data());
      m_sourceRefsDict->inSort(name,md);
    }
  }
}

Definition *Definition::findInnerCompound(const char *)
{
  return 0;
}

void Definition::addInnerCompound(Definition *)
{
  err("Error: Definition::addInnerCompound() called\n");
}

QCString Definition::qualifiedName() 
{
  if (!m_qualifiedName.isEmpty()) return m_qualifiedName;
  
  //printf("start Definition::qualifiedName() localName=%s\n",m_localName.data());
  if (m_outerScope==0) 
  {
    if (m_localName=="<globalScope>") return "";
    else return m_localName; 
  }

  if (m_outerScope->name()=="<globalScope>")
  {
    m_qualifiedName = m_localName.copy();
  }
  else
  {
    m_qualifiedName = m_outerScope->qualifiedName()+"::"+m_localName;
  }
  //printf("end Definition::qualifiedName()=%s\n",qualifiedName.data());
  return m_qualifiedName;
};

QCString Definition::localName() const
{
  return m_localName;
}

void Definition::setBodySegment(int bls,int ble) 
{
  //printf("setBodySegment(%d,%d) for %s\n",bls,ble,name().data());
  m_startBodyLine=bls; 
  m_endBodyLine=ble; 
}

void Definition::makePartOfGroup(GroupDef *gd)
{
  if (m_partOfGroups==0) m_partOfGroups = new GroupList;
  m_partOfGroups->append(gd);
}

void Definition::setRefItems(const QList<ListItemInfo> *sli)
{
  if (sli)
  {
    // deep copy the list
    if (m_xrefListItems==0) 
    {
      m_xrefListItems=new QList<ListItemInfo>;
      m_xrefListItems->setAutoDelete(TRUE);
    }
    QListIterator<ListItemInfo> slii(*sli);
    ListItemInfo *lii;
    for (slii.toFirst();(lii=slii.current());++slii)
    {
      m_xrefListItems->append(new ListItemInfo(*lii));
    } 
  }
}

void Definition::mergeRefItems(Definition *d)
{
  if (d->xrefListItems())
  {
    // deep copy the list
    if (m_xrefListItems==0) 
    {
      m_xrefListItems=new QList<ListItemInfo>;
      m_xrefListItems->setAutoDelete(TRUE);
    }
    QListIterator<ListItemInfo> slii(*d->xrefListItems());
    ListItemInfo *lii;
    for (slii.toFirst();(lii=slii.current());++slii)
    {
      if (getXRefListId(lii->type)==-1)
      {
        m_xrefListItems->append(new ListItemInfo(*lii));
      }
    } 
  }
}

int Definition::getXRefListId(const char *listName) const
{
  if (m_xrefListItems)
  {
    QListIterator<ListItemInfo> slii(*m_xrefListItems);
    ListItemInfo *lii;
    for (slii.toFirst();(lii=slii.current());++slii)
    {
      if (strcmp(lii->type,listName)==0)
      {
        return lii->itemId;
      }
    }
  }
  return -1;
}

const QList<ListItemInfo> *Definition::xrefListItems() const
{
  return m_xrefListItems;
}


QCString Definition::convertNameToFile(const char *name,bool allowDots) const
{
  if (!m_ref.isEmpty())
  {
    return name;
  }
  else
  {
    return ::convertNameToFile(name,allowDots);
  }
}

