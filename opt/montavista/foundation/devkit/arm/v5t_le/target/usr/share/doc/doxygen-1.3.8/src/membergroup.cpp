/******************************************************************************
 *
 * $Id: membergroup.cpp,v 1.19 2001/03/19 19:27:41 root Exp $
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
#include "membergroup.h"
#include "memberlist.h"
#include "outputlist.h"
#include "util.h"
#include "classdef.h"
#include "namespacedef.h"
#include "filedef.h"
#include "language.h"
#include "scanner.h"
#include "groupdef.h"
#include "doxygen.h"
#include "docparser.h"

//static QCString idToName(int id)
//{
//  QCString result;
//  result.sprintf("mgroup_%d",id);
//  return result;
//}

MemberGroup::MemberGroup(Definition *parent,
      int id,const char *hdr,const char *d,const char *docFile) 
{
  //printf("New member group id=%d header=%s desc=%s\n",id,hdr,d);
  memberList      = new MemberList;
  grpId           = id;
  grpHeader       = hdr;
  doc             = d;
  scope           = 0;
  inSameSection   = TRUE;
  inDeclSection   = 0;
  m_numDecMembers = -1;
  m_parent        = parent;
  m_docFile       = docFile;
  //printf("Member group docs=`%s'\n",doc.data());
}

MemberGroup::~MemberGroup()
{
  delete memberList;
}

void MemberGroup::insertMember(MemberDef *md)
{
  //printf("MemberGroup::insertMember memberList=%p count=%d"
  //       " member section list: %p\n",
  //       memberList->first() ? memberList->first()->getSectionList() : 0,
  //       memberList->count(),
  //       md->getSectionList());
  MemberDef *firstMd = memberList->first();
  if (inSameSection && memberList->count()>0 && 
      firstMd->getSectionList(m_parent)!=md->getSectionList(m_parent))
  {
    inSameSection=FALSE;
  }
  else if (inDeclSection==0)
  {
    inDeclSection = md->getSectionList(m_parent);
  }
  memberList->append(md);

  // copy the group of the first member in the memberGroup
  GroupDef *gd;
  if (firstMd && (gd=firstMd->getGroupDef()))
  {
    md->setGroupDef(gd, firstMd->getGroupPri(), firstMd->getGroupFileName(), firstMd->getGroupStartLine(), firstMd->getGroupHasDocs());
    gd->insertMember(md);
  }
}


void MemberGroup::setAnchors(ClassDef *context)
{
  ::setAnchors(context,'z',memberList,grpId);
}

void MemberGroup::writeDeclarations(OutputList &ol,
               ClassDef *cd,NamespaceDef *nd,FileDef *fd,GroupDef *gd)
{
  //printf("MemberGroup::writeDeclarations() memberList->count()=%d\n",memberList->count());
  memberList->writeDeclarations(ol,cd,nd,fd,gd,grpHeader,doc/*,TRUE,TRUE*/);
}

void MemberGroup::writePlainDeclarations(OutputList &ol,
               ClassDef *cd,NamespaceDef *nd,FileDef *fd,GroupDef *gd
              )
{
  //printf("MemberGroup::writePlainDeclarations() memberList->count()=%d\n",memberList->count());
  memberList->writePlainDeclarations(ol,cd,nd,fd,gd);
}

void MemberGroup::writeDocumentation(OutputList &ol,const char *scopeName,
               Definition *container)
{
  memberList->writeDocumentation(ol,scopeName,container,0);
}

/*! Add this group as a subsection of the declaration section, instead
 *  of rendering it in its own section
 */
void MemberGroup::addToDeclarationSection()
{
  if (inDeclSection)
  {
    inDeclSection->addMemberGroup(this);
  }
}

int MemberGroup::countDecMembers(/*bool sectionPerType*/)
{
  if (m_numDecMembers==-1) /* number of member not cached */
  {
    memberList->countDecMembers(/*TRUE,TRUE,sectionPerType*/);
    m_numDecMembers = memberList->numDecMembers();
  }
  return m_numDecMembers;
}

void MemberGroup::distributeMemberGroupDocumentation()
{
  //printf("MemberGroup::distributeMemberGroupDocumentation() %s\n",grpHeader.data());
  MemberDef *md=memberList->first();
  if (md && !(md->documentation().isEmpty() && 
              md->briefDescription().isEmpty() && 
              md->inbodyDocumentation().isEmpty()
             )
     )
  {
    //printf("First member %s has documentation!\n",md->name().data());
    MemberDef *omd=memberList->next();
    while (omd)
    {
      if (omd->documentation().isEmpty() && omd->briefDescription().isEmpty() && omd->inbodyDocumentation().isEmpty())
      {
        //printf("Copying documentation to member %s\n",omd->name().data());
        omd->setBriefDescription(md->briefDescription(),md->briefFile(),md->briefLine());
        omd->setDocumentation(md->documentation(),md->docFile(),md->docLine());
        omd->setInbodyDocumentation(md->inbodyDocumentation(),md->inbodyFile(),md->inbodyLine());
      }
      omd=memberList->next();
    }
  }
}

int MemberGroup::varCount() const       
{ 
  return memberList->varCount(); 
}

int MemberGroup::funcCount() const      
{ 
  return memberList->funcCount(); 
}

int MemberGroup::enumCount() const      
{ 
  return memberList->enumCount(); 
}

int MemberGroup::enumValueCount() const 
{ 
  return memberList->enumValueCount(); 
}

int MemberGroup::typedefCount() const   
{ 
  return memberList->typedefCount(); 
}

int MemberGroup::protoCount() const     
{ 
  return memberList->protoCount(); 
}

int MemberGroup::defineCount() const    
{ 
  return memberList->defineCount(); 
}

int MemberGroup::friendCount() const    
{ 
  return memberList->friendCount(); 
}

int MemberGroup::numDecMembers() const  
{ 
  return memberList->numDecMembers(); 
}

int MemberGroup::numDocMembers() const  
{ 
  return memberList->numDocMembers(); 
}

void MemberGroup::setInGroup(bool b)
{
  memberList->setInGroup(b);
}

void MemberGroup::addListReferences(Definition *def)
{
  memberList->addListReferences(def);
}

void MemberGroup::findSectionsInDocumentation()
{
  docFindSections(doc,0,this,m_docFile);
  memberList->findSectionsInDocumentation();
}

