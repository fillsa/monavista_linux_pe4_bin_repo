/******************************************************************************
 *
 * $Id: filedef.h,v 1.32 2001/03/19 19:27:40 root Exp $
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

#ifndef FILEDEF_H
#define FILEDEF_H

#include "index.h"
#include <qlist.h>
#include <qintdict.h>
#include <qdict.h>
#include "config.h"
#include "definition.h"
#include "memberlist.h"
#include "util.h"

class FileDef;
class FileList;
class ClassSDict;
class ClassDef;
class ClassList;
class MemberDef;
class OutputList;
class NamespaceDef;
class NamespaceSDict;
class MemberGroupSDict;
class PackageDef;

struct IncludeInfo
{
  IncludeInfo() { fileDef=0; local=FALSE; }
  ~IncludeInfo() {}
  FileDef *fileDef;
  QCString includeName;
  bool local;
};

/*! \class FileDef filedef.h
    \brief A File definition.
    
    An object of this class contains all file information that is gathered.
    This includes the members and compounds defined in the file.
    
    The member writeDocumentation() can be used to generate the page of
    documentation to HTML and LaTeX.
*/

class FileDef : public Definition
{
  friend class FileName;

  public:
    //enum FileType { Source, Header, Unknown };

    FileDef(const char *p,const char *n,const char *ref=0,const char *dn=0);
   ~FileDef();
    DefType definitionType() { return TypeFile; }

    /*! Returns the unique file name (this may include part of the path). */
    QCString name() const 
    { 
      if (Config_getBool("FULL_PATH_NAMES")) 
        return filename; 
      else 
        return Definition::name(); 
    } 
    
    QCString getOutputFileBase() const 
    { return convertNameToFile(diskname); }

    QCString getFileBase() const
    { return diskname; }

    QCString getSourceFileBase() const
    { return convertNameToFile(diskname+"-source"); }
    
    /*! Returns the name of the verbatim copy of this file (if any). */
    QCString includeName() const 
    { return convertNameToFile(diskname+"-source"); }
    
    /*! Returns the absolute path including the file name. */
    QCString absFilePath() const { return filepath; }
    
    
    /*! Returns the name as it is used in the documentation */
    QCString docName() const { return docname; }
    
    void addSourceRef(int line,Definition *d,MemberDef *md);
    Definition *getSourceDefinition(int lineNr);
    MemberDef *getSourceMember(int lineNr);

    /* Sets the name of the include file to \a n. */
    //void setIncludeName(const char *n_) { incName=n_; }
    
    /*! Returns the absolute path of this file. */ 
    QCString getPath() const { return path; }
    
    bool isLinkableInProject() const
    {
      return hasDocumentation() && !isReference();
    }

    bool isLinkable() const
    {
      return isLinkableInProject() || isReference();
    }
    bool isIncluded(const QCString &name) const;

    void writeDetailedDocumentation(OutputList &ol);
    void writeDocumentation(OutputList &ol);
    void writeSource(OutputList &ol);
    void parseSource();
    friend void generatedFileNames();
    void insertMember(MemberDef *md);
    void insertClass(ClassDef *cd);
    void insertNamespace(NamespaceDef *nd);
    void computeAnchors();

    void setPackageDef(PackageDef *pd) { package=pd; }
    PackageDef *packageDef() const { return package; }
    
    void addUsingDirective(NamespaceDef *nd);
    NamespaceSDict *getUsedNamespaces() const { return usingDirList; }
    void addUsingDeclaration(Definition *def);
    SDict<Definition> *getUsedClasses() const { return usingDeclList; }
    void combineUsingRelations();

    bool generateSourceFile() const;

    void addIncludeDependency(FileDef *fd,const char *incName,bool local);
    void addIncludedByDependency(FileDef *fd,const char *incName,bool local);
    QList<IncludeInfo> *includeFileList() const { return includeList; }
    QList<IncludeInfo> *includedByFileList() const { return includedByList; }

    void addMembersToMemberGroup();
    void distributeMemberGroupDocumentation();
    void findSectionsInDocumentation();
    void addIncludedUsingDirectives();

    void addListReferences();

    //void generateXML(QTextStream &t);
    //void generateXMLSection(QTextStream &t,MemberList *ml,const char *type);

    MemberList allMemberList;

    // members in the declaration part of the documentation
    MemberList decDefineMembers;
    MemberList decProtoMembers;
    MemberList decTypedefMembers;
    MemberList decEnumMembers;
    MemberList decFuncMembers;
    MemberList decVarMembers;

    // members in the documentation part of the documentation
    MemberList docDefineMembers;
    MemberList docProtoMembers;
    MemberList docTypedefMembers;
    MemberList docEnumMembers;
    MemberList docFuncMembers;
    MemberList docVarMembers;

    /* user defined member groups */
    MemberGroupSDict *memberGroupSDict;

    NamespaceSDict *namespaceSDict;
    ClassSDict *classSDict;
    
    bool visited;

  private: 
    
    QDict<IncludeInfo> *includeDict;
    QList<IncludeInfo> *includeList;
    QDict<IncludeInfo> *includedByDict;
    QList<IncludeInfo> *includedByList;
    NamespaceSDict *usingDirList;
    SDict<Definition> *usingDeclList;
    //DefineList *defineList;
    QCString path;
    QCString filepath;
    QCString diskname;
    QCString filename;
    QCString docname;
    //QCString incName;
    QIntDict<Definition> *srcDefDict;
    QIntDict<MemberDef> *srcMemberDict;
    bool isSource;


    PackageDef *package;
};


class FileList : public QList<FileDef>
{
  public:
    FileList() : m_pathName("tmp") {}
    FileList(const char *path) : QList<FileDef>(), m_pathName(path) {}
   ~FileList() {}
    QCString path() const { return m_pathName; }
    int compareItems(GCI item1,GCI item2)
    {
      FileDef *md1 = (FileDef *)item1;
      FileDef *md2 = (FileDef *)item2;
      return stricmp(md1->name(),md2->name());
    }
  private:
    QCString m_pathName;
};

class OutputNameList : public QList<FileList>
{
  public:
    OutputNameList() : QList<FileList>() {}
   ~OutputNameList() {}
    int compareItems(GCI item1,GCI item2)
    {
      FileList *fl1 = (FileList *)item1;
      FileList *fl2 = (FileList *)item2;
      return stricmp(fl1->path(),fl2->path());
    }
};

class OutputNameDict : public QDict<FileList>
{
  public:
    OutputNameDict(int size) : QDict<FileList>(size) {}
   ~OutputNameDict() {}
};

class Directory;

class DirEntry
{
  public:
    enum EntryKind { Dir, File };
    DirEntry(DirEntry *parent,FileDef *fd)  
       : m_parent(parent), m_kind(File), m_fd(fd), m_isLast(FALSE) { }
    DirEntry(DirEntry *parent)              
       : m_parent(parent), m_kind(Dir), m_fd(0), m_isLast(FALSE) { }
    virtual ~DirEntry() { }
    EntryKind kind() const { return m_kind; }
    FileDef *file()  const { return m_fd; }
    bool isLast() const    { return m_isLast; }
    void setLast(bool b)   { m_isLast=b; }
    DirEntry *parent() const { return m_parent; }

  protected:
    DirEntry *m_parent;
  private:
    EntryKind m_kind;
    FileDef   *m_fd;
    int num;
    bool m_isLast;
};

class Directory : public DirEntry
{
  public:
    Directory(Directory *parent,const QCString &name) 
       : DirEntry(parent), m_name(name)
    { m_children.setAutoDelete(TRUE); }
    virtual ~Directory()              {}
    void addChild(DirEntry *d)        { m_children.append(d); d->setLast(TRUE); }
    QList<DirEntry> &children()       { return m_children; }
    void rename(const QCString &name) { m_name=name; }
    void reParent(Directory *parent)  { m_parent=parent; }
    QCString name() const             { return m_name; }

  private:
    QCString m_name;
    QList<DirEntry> m_children;
};

void generateFileTree(QTextStream &t);

#endif

