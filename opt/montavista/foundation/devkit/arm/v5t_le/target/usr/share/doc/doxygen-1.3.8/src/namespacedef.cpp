/******************************************************************************
 *
 * $Id: namespacedef.cpp,v 1.27 2001/03/19 19:27:41 root Exp $
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
#include "namespacedef.h"
#include "outputlist.h"
#include "util.h"
#include "language.h"
#include "classdef.h"
#include "classlist.h"
#include "memberlist.h"
#include "doxygen.h"
#include "message.h"
#include "docparser.h"
#include "searchindex.h"

NamespaceDef::NamespaceDef(const char *df,int dl,
                           const char *name,const char *lref) : 
   Definition(df,dl,name)
{
  fileName="namespace";
  fileName+=name;
  classSDict = new ClassSDict(17);
  namespaceSDict = new NamespaceSDict(17);
  m_innerCompounds = new SDict<Definition>(257);
  usingDirList = 0;
  usingDeclList = 0;
  setReference(lref);
  memberGroupSDict = new MemberGroupSDict;
  memberGroupSDict->setAutoDelete(TRUE);
}

NamespaceDef::~NamespaceDef()
{
  delete classSDict;
  delete namespaceSDict;
  delete m_innerCompounds;
  delete usingDirList;
  delete usingDeclList;
  delete memberGroupSDict;
}

void NamespaceDef::distributeMemberGroupDocumentation()
{
  MemberGroupSDict::Iterator mgli(*memberGroupSDict);
  MemberGroup *mg;
  for (;(mg=mgli.current());++mgli)
  {
    mg->distributeMemberGroupDocumentation();
  }
}

void NamespaceDef::findSectionsInDocumentation()
{
  docFindSections(documentation(),this,0,docFile());
  MemberGroupSDict::Iterator mgli(*memberGroupSDict);
  MemberGroup *mg;
  for (;(mg=mgli.current());++mgli)
  {
    mg->findSectionsInDocumentation();
  }
  decDefineMembers.findSectionsInDocumentation();
  decProtoMembers.findSectionsInDocumentation();
  decTypedefMembers.findSectionsInDocumentation();
  decEnumMembers.findSectionsInDocumentation();
  decFuncMembers.findSectionsInDocumentation();
  decVarMembers.findSectionsInDocumentation();
}

void NamespaceDef::insertUsedFile(const char *f)
{
  if (files.find(f)==-1) 
  {
    if (Config_getBool("SORT_MEMBER_DOCS"))
      files.inSort(f);
    else
      files.append(f);
  }
}

void NamespaceDef::addInnerCompound(Definition *d)
{
  m_innerCompounds->append(d->localName(),d);
  if (d->definitionType()==Definition::TypeNamespace)
  {
    insertNamespace((NamespaceDef *)d);
  }
  else if (d->definitionType()==Definition::TypeClass)
  {
    insertClass((ClassDef *)d);
  }
}

void NamespaceDef::insertClass(ClassDef *cd)
{
  if (classSDict->find(cd->name())==0)
  {
    if (Config_getBool("SORT_BRIEF_DOCS"))
      classSDict->inSort(cd->name(),cd);
    else
      classSDict->append(cd->name(),cd);
  }
}

void NamespaceDef::insertNamespace(NamespaceDef *nd)
{
  if (namespaceSDict->find(nd->name())==0)
  {
    if (Config_getBool("SORT_MEMBER_DOCS"))
      namespaceSDict->inSort(nd->name(),nd);
    else
      namespaceSDict->append(nd->name(),nd);
  }
}


void NamespaceDef::addMembersToMemberGroup()
{
  ::addMembersToMemberGroup(&decDefineMembers,memberGroupSDict,this);
  ::addMembersToMemberGroup(&decProtoMembers,memberGroupSDict,this);
  ::addMembersToMemberGroup(&decTypedefMembers,memberGroupSDict,this);
  ::addMembersToMemberGroup(&decEnumMembers,memberGroupSDict,this);
  ::addMembersToMemberGroup(&decFuncMembers,memberGroupSDict,this);
  ::addMembersToMemberGroup(&decVarMembers,memberGroupSDict,this);
}

void NamespaceDef::insertMember(MemberDef *md)
{
  //memList->append(md);
  allMemberList.append(md); 
  switch(md->memberType())
  {
    case MemberDef::Variable:     
      if (Config_getBool("SORT_BRIEF_DOCS"))
        decVarMembers.inSort(md);
      else
        decVarMembers.append(md);
      if (Config_getBool("SORT_MEMBER_DOCS"))
        docVarMembers.inSort(md); 
      else
        docVarMembers.append(md);
      break;
    case MemberDef::Function: 
      if (Config_getBool("SORT_BRIEF_DOCS"))
        decFuncMembers.inSort(md);
      else
        decFuncMembers.append(md);
      if (Config_getBool("SORT_MEMBER_DOCS"))    
        docFuncMembers.inSort(md); 
      else
        docFuncMembers.append(md);
      break;
    case MemberDef::Typedef:      
      if (Config_getBool("SORT_BRIEF_DOCS"))
        decTypedefMembers.inSort(md);
      else
        decTypedefMembers.append(md);
      if (Config_getBool("SORT_MEMBER_DOCS"))
        docTypedefMembers.inSort(md); 
      else
        docTypedefMembers.append(md);
      break;
    case MemberDef::Enumeration:  
      if (Config_getBool("SORT_BRIEF_DOCS"))
        decEnumMembers.inSort(md);
      else
        decEnumMembers.append(md);
      if (Config_getBool("SORT_MEMBER_DOCS"))
        docEnumMembers.inSort(md); 
      else
        docEnumMembers.append(md);
      break;
    case MemberDef::EnumValue:    
      break;
    case MemberDef::Prototype:    
      if (Config_getBool("SORT_BRIEF_DOCS"))
        decProtoMembers.inSort(md);
      else
        decProtoMembers.append(md);
      if (Config_getBool("SORT_MEMBER_DOCS"))
        docProtoMembers.inSort(md); 
      else
        docProtoMembers.append(md);
      break;
    case MemberDef::Define:       
      if (Config_getBool("SORT_BRIEF_DOCS"))
        decDefineMembers.inSort(md);
      else
        decDefineMembers.append(md);
      if (Config_getBool("SORT_MEMBER_DOCS"))
        docDefineMembers.inSort(md); 
      else
        docDefineMembers.append(md);
      break;
    default:
      err("NamespaceDef::insertMembers(): "
           "member `%s' with class scope `%s' inserted in namespace scope `%s'!\n",
           md->name().data(),
           md->getClassDef() ? md->getClassDef()->name().data() : "",
           name().data());
  }
  //addMemberToGroup(md,groupId);
}

void NamespaceDef::computeAnchors()
{
  setAnchors(0,'a',&allMemberList);
}

void NamespaceDef::writeDetailedDocumentation(OutputList &ol)
{
  if ((!briefDescription().isEmpty() && Config_getBool("REPEAT_BRIEF")) || 
      !documentation().isEmpty())
  {
    ol.writeRuler();
    ol.pushGeneratorState();
    ol.disableAllBut(OutputGenerator::Html);
    //bool latexOn = ol.isEnabled(OutputGenerator::Latex);
    //if (latexOn) ol.disable(OutputGenerator::Latex);
    ol.writeAnchor(0,"_details"); 
    //if (latexOn) ol.enable(OutputGenerator::Latex);
    ol.popGeneratorState();
    ol.startGroupHeader();
    ol.parseText(theTranslator->trDetailedDescription());
    ol.endGroupHeader();
    ol.startTextBlock();
    if (!briefDescription().isEmpty() && Config_getBool("REPEAT_BRIEF"))
    {
      ol.parseDoc(briefFile(),briefLine(),this,0,briefDescription(),FALSE,FALSE);
    }
    if (!briefDescription().isEmpty() && Config_getBool("REPEAT_BRIEF") &&
        !documentation().isEmpty())
    {
      ol.pushGeneratorState();
        ol.disable(OutputGenerator::Man);
        ol.newParagraph();
        ol.enableAll();
        ol.disableAllBut(OutputGenerator::Man);
        ol.writeString("\n\n");
      ol.popGeneratorState();
    }
    if (!documentation().isEmpty())
    {
      ol.parseDoc(docFile(),docLine(),this,0,documentation()+"\n",TRUE,FALSE);
      ol.newParagraph();
    }
    ol.endTextBlock();
  }
}
  
void NamespaceDef::writeDocumentation(OutputList &ol)
{
  QCString pageTitle;
  if (Config_getBool("OPTIMIZE_OUTPUT_JAVA"))
  {
    pageTitle = theTranslator->trPackage(displayName());
  }
  else
  {
    pageTitle = theTranslator->trNamespaceReference(displayName());
  }
  startFile(ol,getOutputFileBase(),name(),pageTitle);
  startTitle(ol,getOutputFileBase());
  //ol.docify(pageTitle);
  //ol.parseText(pageTitle);
  addGroupListToTitle(ol,this);
  endTitle(ol,getOutputFileBase(),displayName());
  
  if (Config_getBool("SEARCHENGINE"))
  {
    Doxygen::searchIndex->setCurrentDoc(pageTitle,getOutputFileBase());
    Doxygen::searchIndex->addWord(localName().lower());
  }

  if (!Config_getString("GENERATE_TAGFILE").isEmpty())
  {
    Doxygen::tagFile << "  <compound kind=\"namespace\">" << endl;
    Doxygen::tagFile << "    <name>" << convertToXML(name()) << "</name>" << endl;
    Doxygen::tagFile << "    <filename>" << convertToXML(getOutputFileBase()) << Doxygen::htmlFileExtension << "</filename>" << endl;
  }
  
  ol.startTextBlock();
    
  if (Config_getBool("DETAILS_AT_TOP"))
  {
    writeDetailedDocumentation(ol);
    ol.newParagraph();
  }
  else if (!briefDescription().isEmpty()) 
  {
    ol.parseDoc(briefFile(),briefLine(),this,0,briefDescription(),TRUE,FALSE);
    ol.writeString(" \n");
    ol.pushGeneratorState();
    ol.disableAllBut(OutputGenerator::Html);
    ol.startTextLink(0,"_details");
    ol.parseText(theTranslator->trMore());
    ol.endTextLink();
    ol.enableAll();
    ol.disableAllBut(OutputGenerator::Man);
    ol.newParagraph();
    ol.popGeneratorState();
  }
  ol.disable(OutputGenerator::Man);
  ol.newParagraph();
  ol.enable(OutputGenerator::Man);
  ol.writeSynopsis();

  ol.endTextBlock();
  
  ol.startMemberSections();
  classSDict->writeDeclaration(ol);

  /* write user defined member groups */
  MemberGroupSDict::Iterator mgli(*memberGroupSDict);
  MemberGroup *mg;
  for (;(mg=mgli.current());++mgli)
  {
    mg->writeDeclarations(ol,0,this,0,0);
  }

  //allMemberList.writeDeclarations(ol,0,this,0,0,0,0);
  decDefineMembers.writeDeclarations(ol,0,this,0,0,theTranslator->trDefines(),0);
  decProtoMembers.writeDeclarations(ol,0,this,0,0,theTranslator->trFuncProtos(),0);
  decTypedefMembers.writeDeclarations(ol,0,this,0,0,theTranslator->trTypedefs(),0);
  decEnumMembers.writeDeclarations(ol,0,this,0,0,theTranslator->trEnumerations(),0);
  decFuncMembers.writeDeclarations(ol,0,this,0,0,theTranslator->trFunctions(),0);
  decVarMembers.writeDeclarations(ol,0,this,0,0,theTranslator->trVariables(),0);
  ol.endMemberSections();
  
  if (!Config_getBool("DETAILS_AT_TOP"))
  {
    writeDetailedDocumentation(ol);
  }

  docDefineMembers.writeDocumentation(ol,name(),this,
                          theTranslator->trDefineDocumentation());
  
  docProtoMembers.writeDocumentation(ol,name(),this,
                          theTranslator->trFunctionPrototypeDocumentation());

  docTypedefMembers.writeDocumentation(ol,name(),this,
                          theTranslator->trTypedefDocumentation());
  
  docEnumMembers.writeDocumentation(ol,name(),this,
                          theTranslator->trEnumerationTypeDocumentation());

  docFuncMembers.writeDocumentation(ol,name(),this,
                          theTranslator->trFunctionDocumentation());
  
  docVarMembers.writeDocumentation(ol,name(),this,
                          theTranslator->trVariableDocumentation());

  // write Author section (Man only)
  ol.pushGeneratorState();
  ol.disableAllBut(OutputGenerator::Man);
  ol.startGroupHeader();
  ol.parseText(theTranslator->trAuthor(TRUE,TRUE));
  ol.endGroupHeader();
  ol.parseText(theTranslator->trGeneratedAutomatically(Config_getString("PROJECT_NAME")));

  if (!Config_getString("GENERATE_TAGFILE").isEmpty()) 
  {
    writeDocAnchorsToTagFile();
    Doxygen::tagFile << "  </compound>" << endl;
  }

  ol.popGeneratorState();
  endFile(ol);
}

int NamespaceDef::countMembers()
{
  allMemberList.countDocMembers();
  return allMemberList.numDocMembers()+classSDict->count();
}

void NamespaceDef::addUsingDirective(NamespaceDef *nd)
{
  if (usingDirList==0)
  {
    usingDirList = new NamespaceSDict;
  }
  if (usingDirList->find(nd->qualifiedName())==0)
  {
    usingDirList->append(nd->qualifiedName(),nd);
  }
}

void NamespaceDef::addUsingDeclaration(Definition *d)
{
  if (usingDeclList==0)
  {
    usingDeclList = new SDict<Definition>(17);
  }
  if (usingDeclList->find(d->qualifiedName())==0)
  {
    usingDeclList->append(d->qualifiedName(),d);
  }
}

QCString NamespaceDef::getOutputFileBase() const 
{ 
  return convertNameToFile(fileName); 
}

Definition *NamespaceDef::findInnerCompound(const char *n)
{
  if (n==0) return 0;
  return m_innerCompounds->find(n);
}

void NamespaceDef::addListReferences()
{
  addRefItem(xrefListItems(),
             theTranslator->trNamespace(TRUE,TRUE),
             getOutputFileBase(),displayName()
            );
  MemberGroupSDict::Iterator mgli(*memberGroupSDict);
  MemberGroup *mg;
  for (;(mg=mgli.current());++mgli)
  {
    mg->addListReferences(this);
  }
  docDefineMembers.addListReferences(this);
  docProtoMembers.addListReferences(this);
  docTypedefMembers.addListReferences(this);
  docEnumMembers.addListReferences(this);
  docFuncMembers.addListReferences(this);
  docVarMembers.addListReferences(this);
}

QCString NamespaceDef::displayName() const
{
  QCString result=name();
  if (Config_getBool("OPTIMIZE_OUTPUT_JAVA"))
  {
    result = substitute(result,"::",".");
  }
  return result; 
}

void NamespaceDef::combineUsingRelations()
{
  if (visited) return; // already done
  visited=TRUE;
  if (usingDirList)
  {
    NamespaceSDict::Iterator nli(*usingDirList);
    NamespaceDef *nd;
    for (nli.toFirst();(nd=nli.current());++nli)
    {
      nd->combineUsingRelations();
    }
    for (nli.toFirst();(nd=nli.current());++nli)
    {
      // add used namespaces of namespace nd to this namespace
      if (nd->getUsedNamespaces())
      {
        NamespaceSDict::Iterator unli(*nd->getUsedNamespaces());
        NamespaceDef *und;
        for (unli.toFirst();(und=unli.current());++unli)
        {
          //printf("Adding namespace %s to the using list of %s\n",und->qualifiedName().data(),qualifiedName().data());
          addUsingDirective(und);
        }
      }
      // add used classes of namespace nd to this namespace
      if (nd->getUsedClasses())
      {
        SDict<Definition>::Iterator cli(*nd->getUsedClasses());
        Definition *ucd;
        for (cli.toFirst();(ucd=cli.current());++cli)
        {
          //printf("Adding class %s to the using list of %s\n",cd->qualifiedName().data(),qualifiedName().data());
          addUsingDeclaration(ucd);
        }
      }
    }
  }
}

