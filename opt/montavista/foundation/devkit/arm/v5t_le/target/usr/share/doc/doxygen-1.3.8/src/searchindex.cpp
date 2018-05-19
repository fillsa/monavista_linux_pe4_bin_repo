/******************************************************************************
 *
 * $Id: searchindex.cpp,v 1.7 2001/03/19 19:27:41 root Exp $
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
#include "searchindex.h"
#include "config.h"
#include <qfile.h>


// file format:
//   4 byte header
//   256*256*4 byte index
//   for each index entry: a zero terminated list of words 
//   for each word: a 0 terminated string + 4 bytes stats index
//   padding bytes to align at 4 byte boundary
//   for each word: a counter + for each url containing the word 8 bytes statistics
//   for each url: a 0 terminated string

const int numIndexEntries = 256*256;

//--------------------------------------------------------------------

IndexWord::IndexWord(const char *word) : m_word(word), m_urls(17)
{
  m_urls.setAutoDelete(TRUE);
}

void IndexWord::addUrlIndex(int idx)
{
  URLInfo *ui = m_urls.find(idx);
  if (ui==0)
  {
    ui=new URLInfo(idx,0);
    m_urls.insert(idx,ui);
  }
  ui->freq++;
}

//--------------------------------------------------------------------

SearchIndex::SearchIndex() : m_words(328829), m_index(numIndexEntries), m_urlIndex(-1)
{
  int i;
  m_words.setAutoDelete(TRUE);
  m_urls.setAutoDelete(TRUE);
  for (i=0;i<numIndexEntries;i++) m_index.insert(i,new QList<IndexWord>);
}

void SearchIndex::setCurrentDoc(const char *name,const char *baseName,const char *anchor)
{
  QCString url=baseName+Config_getString("HTML_FILE_EXTENSION");
  if (anchor) url+=(QCString)"#"+anchor;  
  m_urlIndex++;
  m_urls.insert(m_urlIndex,new URL(name,url));
}


static int charsToIndex(const char *word)
{
  if (word==0) return -1;
  uint c1=word[0];
  if (c1==0) return -1;
  uint c2=word[1];
  if (c2==0) return -1;
  return c1*256+c2;
}

void SearchIndex::addWord(const char *word)
{
  IndexWord *w = m_words[word];
  if (w==0)
  {
    int idx=charsToIndex(word);
    if (idx<0) return;
    w = new IndexWord(word);
    //fprintf(stderr,"addWord(%s) at index %d\n",word,idx);
    m_index[idx]->append(w);
    m_words.insert(word,w);
  }
  w->addUrlIndex(m_urlIndex);
}


static void writeInt(QFile &f,int index)
{
  f.putch(((uint)index)>>24);
  f.putch((((uint)index)>>16)&0xff);
  f.putch((((uint)index)>>8)&0xff);
  f.putch(((uint)index)&0xff);
}

static void writeString(QFile &f,const char *s)
{
  const char *p = s;
  while (*p) f.putch(*p++);
  f.putch(0);
}

void SearchIndex::write(const char *fileName)
{
  int i;
  int size=4; // for the header
  size+=4*numIndexEntries; // for the index
  int wordsOffset = size;
  // first pass: compute the size of the wordlist
  for (i=0;i<numIndexEntries;i++)
  {
    QList<IndexWord> *wlist = m_index[i];
    if (!wlist->isEmpty())
    {
      QListIterator<IndexWord> iwi(*wlist);
      IndexWord *iw;
      for (iwi.toFirst();(iw=iwi.current());++iwi)
      {
        int ws = iw->word().length()+1; 
        size+=ws+4; // word + url info list offset
      }
      size+=1; // zero list terminator
    }
  }

  // second pass: compute the offsets in the index
  int indexOffsets[numIndexEntries];
  int offset=wordsOffset;
  for (i=0;i<numIndexEntries;i++)
  {
    QList<IndexWord> *wlist = m_index[i];
    if (!wlist->isEmpty())
    {
      indexOffsets[i]=offset;
      QListIterator<IndexWord> iwi(*wlist);
      IndexWord *iw;
      for (iwi.toFirst();(iw=iwi.current());++iwi)
      {
        offset+= iw->word().length()+1; 
        offset+=4; // word + offset to url info array 
      }
      offset+=1; // zero list terminator
    }
    else
    {
      indexOffsets[i]=0;
    }
  }
  int padding = size;
  size = (size+3)&~3; // round up to 4 byte boundary
  padding = size - padding;

  //int statsOffset = size;
  QDictIterator<IndexWord> wdi(m_words);
  //IndexWord *iw;
  int *wordStatOffsets = new int[m_words.count()];
  
  int count=0;

  // third pass: compute offset to stats info for each word
  for (i=0;i<numIndexEntries;i++)
  {
    QList<IndexWord> *wlist = m_index[i];
    if (!wlist->isEmpty())
    {
      QListIterator<IndexWord> iwi(*wlist);
      IndexWord *iw;
      for (iwi.toFirst();(iw=iwi.current());++iwi)
      {
        //printf("wordStatOffsets[%d]=%d\n",count,size);
        wordStatOffsets[count++] = size;
        size+=4+iw->urls().count()*8; // count + (url_index,freq) per url
      }
    }
  }
  int *urlOffsets = new int[m_urls.count()];
  //int urlsOffset = size;
  QIntDictIterator<URL> udi(m_urls);
  URL *url;
  for (udi.toFirst();(url=udi.current());++udi)
  {
    urlOffsets[udi.currentKey()]=size;
    size+=url->name.length()+1+
          url->url.length()+1;
  }
  //printf("Total size %x bytes (word=%x stats=%x urls=%x)\n",size,wordsOffset,statsOffset,urlsOffset);
  QFile f(fileName);
  if (f.open(IO_WriteOnly))
  {
    // write header
    f.putch('D'); f.putch('O'); f.putch('X'); f.putch('S');
    // write index
    for (i=0;i<numIndexEntries;i++)
    {
      writeInt(f,indexOffsets[i]);
    }
    // write word lists
    count=0;
    for (i=0;i<numIndexEntries;i++)
    {
      QList<IndexWord> *wlist = m_index[i];
      if (!wlist->isEmpty())
      {
        QListIterator<IndexWord> iwi(*wlist);
        IndexWord *iw;
        for (iwi.toFirst();(iw=iwi.current());++iwi)
        {
          writeString(f,iw->word());
          writeInt(f,wordStatOffsets[count++]);
        }
        f.putch(0);
      }
    }
    // write extra padding bytes
    for (i=0;i<padding;i++) f.putch(0);
    // write word statistics
    for (i=0;i<numIndexEntries;i++)
    {
      QList<IndexWord> *wlist = m_index[i];
      if (!wlist->isEmpty())
      {
        QListIterator<IndexWord> iwi(*wlist);
        IndexWord *iw;
        for (iwi.toFirst();(iw=iwi.current());++iwi)
        {
          int numUrls = iw->urls().count();
          writeInt(f,numUrls);
          QIntDictIterator<URLInfo> uli(iw->urls());
          URLInfo *ui;
          for (uli.toFirst();(ui=uli.current());++uli)
          {
            writeInt(f,urlOffsets[ui->urlIdx]);
            writeInt(f,ui->freq);
          }
        }
      }
    }
    // write urls
    QIntDictIterator<URL> udi(m_urls);
    URL *url;
    for (udi.toFirst();(url=udi.current());++udi)
    {
      writeString(f,url->name);
      writeString(f,url->url);
    }
  }

  //for (wdi.toFirst();(iw=wdi.current());++wdi)
  //{
  //  printf("Word %s:\n",wdi.currentKey().data());
  //  QIntDictIterator<URLInfo> udi(iw->urls());
  //  URLInfo *ui;
  //  for (udi.toFirst();(ui=udi.current());++udi)
  //  {
  //    printf("  url[%d]=(name=%s,url=%s),freq=%d\n",
  //        ui->urlIdx,
  //        m_urls[ui->urlIdx]->name.data(),
  //        m_urls[ui->urlIdx]->url.data(),
  //        ui->freq);
  //  }
  //}

  delete urlOffsets;
  delete wordStatOffsets;
}

