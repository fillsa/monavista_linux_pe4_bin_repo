/******************************************************************************
 *
 * $Id: outputgen.cpp,v 1.15 2001/03/19 19:27:41 root Exp $
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

#include <stdlib.h>

#include "qtbc.h"
#include "outputgen.h"
#include "message.h"

OutputGenerator::OutputGenerator()
{
  //printf("OutputGenerator::OutputGenerator()\n");
  file=0;
  b.setBuffer(a);
  b.open( IO_WriteOnly );
  t.setDevice(&b);
#if QT_VERSION >= 200
  t.setEncoding(QTextStream::Latin1);
#endif
  active=TRUE;
  genStack = new QStack<bool>;
  genStack->setAutoDelete(TRUE);
}

OutputGenerator::~OutputGenerator()
{
  //printf("OutputGenerator::~OutputGenerator()\n");
  delete file;
  delete genStack;
}

void OutputGenerator::startPlainFile(const char *name)
{
  //printf("startPlainFile(%s)\n",name);
  QCString fileName=dir+"/"+name;
  file = new QFile(fileName);
  if (!file)
  {
    err("Could not create file object for %s\n",fileName.data());
    exit(1);
  }
  if (!file->open(IO_WriteOnly))
  {
    err("Could not open file %s for writing\n",fileName.data());
    exit(1);
  }
  fs.setDevice(file);
}

void OutputGenerator::endPlainFile()
{
  //printf("endPlainFile(%s)\n",file->name());
  fs.writeRawBytes(a.data(),a.size()) ;  // write string buffer to file
  delete file; 
  file=0;
  b.close();
  a.resize(0);
  b.setBuffer(a);
  b.open(IO_WriteOnly);
  t.setDevice(&b);
}

QCString OutputGenerator::getContents() const
{
  QCString s;
  s.resize(a.size()+1);
  memcpy(s.data(),a.data(),a.size());
  s.at(a.size())='\0';
  return s;
}

void OutputGenerator::pushGeneratorState()
{
  genStack->push(new bool(isEnabled()));
}

void OutputGenerator::popGeneratorState()
{
  bool *lb = genStack->pop();
  ASSERT(lb!=0);
  if (lb==0) return; // for some robustness against superfluous \endhtmlonly commands.
  if (*lb) enable(); else disable();
  delete lb;
}

