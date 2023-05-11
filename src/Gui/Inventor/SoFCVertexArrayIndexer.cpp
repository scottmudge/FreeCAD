/**************************************************************************\
 * Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include "PreCompiled.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SbName.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbMatrix.h>

#include "SoFCVBO.h"
#include "SoFCVertexArrayIndexer.h"

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer()
  : target(0)
  , indexarray(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW)
  , indexarraylength(0)
  , lastlineindex(0)
  , use_shorts(TRUE)
{
}

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer(const IndexArray &initarray)
  : target(0)
  , indexarray(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW)
  , indexarraylength(0)
  , lastlineindex(0)
  , use_shorts(TRUE)
{
  if (initarray) {
    assert(initarray.getTarget() == indexarray.getTarget());
    this->indexarray.init(initarray);
  }
}

template<class IndicesT, class GetT> inline void
SoFCVertexArrayIndexer::init(const SoFCVertexArrayIndexer & other,
                             const IndicesT & partindices,
                             GetT && getT,
                             int maxindex,
                             bool exclude)
{
  assert(other.indexarray);
  this->lastlineindex = 0;
  this->target = other.target;
  this->partarray = other.partarray;
  this->linestripoffsets = other.linestripoffsets;
  this->linestripcounts = other.linestripcounts;
  if (partindices.empty()) {
    this->use_shorts = other.use_shorts;
    this->indexarray = other.indexarray;
    this->indexarraylength = other.indexarraylength;
    this->partialindices = other.partialindices;
  }
  else if (this->partarray.size()) {
    this->use_shorts = other.use_shorts;
    this->indexarray = other.indexarray;
    this->indexarraylength = other.indexarraylength;
    maxindex = static_cast<int>(this->partarray.size());
    if (exclude) {
      if (this->partarray.size() > (int)partindices.size())
        this->partialindices.reserve(this->partarray.size() - (int)partindices.size());
      auto it = partindices.begin();
      auto &filter = other.partialindices;
      for (int i=0; i<maxindex; ++i) {
        if (filter.size() && !std::binary_search(filter.begin(), filter.end(), i))
          continue;
        for (; it != partindices.end(); ++it) {
          if (getT(it) >= i)
            break;
        }
        if (it != partindices.end() && getT(it) == i) {
          ++it;
          continue;
        }
        this->partialindices.push_back(i);
      }
    } else {
      this->partialindices.reserve(partindices.size());
      for (auto it = partindices.begin(); it != partindices.end(); ++it) {
        int i = getT(it);
        if (i >=0 && i < maxindex)
          this->partialindices.push_back(i);
      }
    }
  } else {
    this->use_shorts = TRUE;
    this->indexarraylength = 0;
    for (auto it = partindices.begin(); it != partindices.end(); ++it) {
      int i = getT(it);
      if (i >= 0 && i < other.indexarray.getLength())
        addIndex(other.indexarray[i]);
    }
  }
}

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                                               const std::map<int, int> & partindices,
                                               int maxindex,
                                               bool exclude)
  :indexarray(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW)
{
  init(other,
       partindices,
       [](const std::map<int, int>::const_iterator &it) {return it->first;},
       maxindex,
       exclude);
}

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                                               const std::set<int> & partindices,
                                               int maxindex,
                                               bool exclude)
  :indexarray(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW)
{
  init(other,
       partindices,
       [](const std::set<int>::const_iterator &it) {return *it;},
       maxindex,
       exclude);
}

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                                               const SbFCVector<int> & partindices,
                                               int maxindex,
                                               bool exclude)
  :indexarray(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW)
{
  init(other,
       partindices,
       [](const SbFCVector<int>::const_iterator &it) {return *it;},
       maxindex,
       exclude);
}

SoFCVertexArrayIndexer::~SoFCVertexArrayIndexer()
{
}

const int *
SoFCVertexArrayIndexer::getPartOffsets(void) const
{
  return &this->partarray[0];
}

int
SoFCVertexArrayIndexer::getNumParts(void) const
{
  return static_cast<int>(this->partarray.size());
}

inline void 
SoFCVertexArrayIndexer::addIndex(int32_t i) 
{
  if (i >= 65536) this->use_shorts = FALSE;
  // We might append more indices for GL_LINE_STRIP (see sort_lines()).
  // 'indexarraylength' here is used to remember the index count for GL_LINES.
  this->indexarray.set(this->indexarraylength++, static_cast<GLint> (i));
}

void
SoFCVertexArrayIndexer::addLine(const int32_t v0, const int32_t v1, int lineindex)
{
  if (this->target == 0) this->target = GL_LINES;
  if (this->target == GL_LINES) {
    for (;lineindex > this->lastlineindex; ++this->lastlineindex)
      this->partarray.push_back(this->indexarraylength);
    this->addIndex(v0);
    this->addIndex(v1);
  }
}

void
SoFCVertexArrayIndexer::addPoint(const int32_t v0)
{
  if (this->target == 0) this->target = GL_POINTS;
  if (this->target == GL_POINTS) {
    this->addIndex(v0);
  }
}

void
SoFCVertexArrayIndexer::addTriangle(const int32_t v0,
                                    const int32_t v1,
                                    const int32_t v2)
{
  if (this->target == 0) this->target = GL_TRIANGLES;
  if (this->target == GL_TRIANGLES) {
    this->addIndex(v0);
    this->addIndex(v1);
    this->addIndex(v2);
  }
}

void
SoFCVertexArrayIndexer::initClass()
{
}

void
SoFCVertexArrayIndexer::cleanup()
{
}

void
SoFCVertexArrayIndexer::close(const int *parts, int count)
{
  if (!this->indexarray) return;

  if (count) {
    int step;
    switch(this->target) {
    case GL_TRIANGLES:
      step = 3;
      break;
    case GL_LINES:
      step = 2;
      break;
    default:
      step = 1;
    }
    this->partarray.clear();
    this->partarray.reserve(count);
    int partindex = 0;
    for (int i=0; i<count; ++i) {
      partindex += parts[i]*step;
      if (partindex > this->indexarraylength)
        break;
      this->partarray.push_back(partindex);
    }
  }
  else if (this->target == GL_LINES
          && (this->partarray.empty()
              || this->partarray.back() < this->indexarraylength))
  {
    this->partarray.push_back(this->indexarraylength);
  }

  if (this->target == GL_TRIANGLES)
    this->sort_triangles();
  else if (this->target == GL_LINES)
    this->sort_lines();
}

void
SoFCVertexArrayIndexer::close(SbFCVector<int> && parts)
{
  if (!this->indexarray) return;
  this->partarray.move(std::move(parts));
}

void
SoFCVertexArrayIndexer::render(SoState * state,
                               const cc_glglue * glue,
                               bool renderasvbo,
                               uint32_t contextid,
                               const intptr_t * offsets,
                               const int32_t * counts,
                               int32_t drawcount)
{
  if (!this->indexarray) return;

  if (renderasvbo != vbo_used) {
    partialcounts.clear();
    partialoffsets.clear();
    vbo_used = renderasvbo;
  }

  if (renderasvbo) {
    this->use_shorts = this->indexarray.attach(this->use_shorts);
    assert(this->indexarray.getTarget() == GL_ELEMENT_ARRAY_BUFFER);
    this->indexarray.bindBuffer(state, contextid);
  }

  GLenum drawtarget = this->target;

  if (!drawcount && !this->linestripoffsets.empty() && glIsEnabled(GL_LINE_STIPPLE)) {
    if (this->partialindices.empty()) {
      drawtarget = GL_LINE_STRIP;
      drawcount = static_cast<int>(this->linestripoffsets.size());
      if (!renderasvbo) {
        int typeshift = this->use_shorts ? 1 : 2;
        for (int i=0; i<drawcount; ++i) {
          // If not render as vbo, then glMultiDrawElements() expects the offsets
          // to contain pointer instead of offsets to the bound buffer.
          cc_glglue_glDrawElements(glue,
                                   drawtarget,
                                   this->linestripcounts[i],
                                   GL_UNSIGNED_INT,
                                   this->getIndices() + (this->linestripoffsets[i] >> typeshift));
        }
        return;
      }
      offsets = &this->linestripoffsets[0];
      counts = &this->linestripcounts[0];
    }
  }

  if (!drawcount && !this->partialindices.empty()) {
    if (this->partialoffsets.empty()) {
      auto size = this->partialindices.size();
      if (this->linestripoffsets.size())
        size += size;
      this->partialoffsets.reserve(size);
      this->partialcounts.reserve(size);

      if (renderasvbo) {
        int typesize = this->use_shorts ? 2 : 4;
        for (int i : this->partialindices) {
          int prev = i ? this->partarray[i-1] : 0;
          this->partialoffsets.push_back(prev * typesize);
          this->partialcounts.push_back(this->partarray[i] - prev);
        }
      }
      else {
        for (int i : this->partialindices) {
          int prev = i ? this->partarray[i-1] : 0;
          this->partialoffsets.push_back((intptr_t)(this->indexarray.getArrayPtr() + prev));
          this->partialcounts.push_back(this->partarray[i] - prev);
        }
      }

      if (!this->linestripoffsets.empty()) {
        if (renderasvbo) {
          for (int i : this->partialindices) {
            this->partialoffsets.push_back(this->linestripoffsets[i]);
            this->partialcounts.push_back(this->linestripcounts[i]);
          }
        }
        else {
          int typeshift = this->use_shorts ? 1 : 2;
          for (int i : this->partialindices) {
            this->partialcounts.push_back(this->linestripcounts[i]);
            this->partialoffsets.push_back(
                (intptr_t)(this->indexarray.getArrayPtr()
                  + (this->linestripoffsets[i] >> typeshift)));
          }
        }
      }
    }

    drawcount = static_cast<int>(this->partialindices.size());
    if (!this->linestripoffsets.empty() && glIsEnabled(GL_LINE_STIPPLE)) {
      drawtarget = GL_LINE_STRIP;
      offsets = &this->partialoffsets[drawcount];
      counts = &this->partialcounts[drawcount];
    }
    else {
      offsets = &this->partialoffsets[0];
      counts = &this->partialcounts[0];
    }
  }

  if (!drawcount) {
    if (renderasvbo)
      cc_glglue_glDrawElements(glue,
                              drawtarget,
                              this->indexarraylength,
                              this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                              NULL);
    else {
      const GLint * idxptr = this->indexarray.getArrayPtr();
      cc_glglue_glDrawElements(glue,
                               drawtarget,
                               this->indexarraylength,
                               GL_UNSIGNED_INT,
                               idxptr);
    }
  }
  else if (cc_glglue_has_multidraw_vertex_arrays(glue)) {
    cc_glglue_glMultiDrawElements(glue,
                                  drawtarget,
                                  (GLsizei*) counts,
                                  renderasvbo && this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                  (const GLvoid **) offsets,
                                  drawcount);
  }
  else {
    for (int i=0; i<drawcount; ++i)
      cc_glglue_glDrawElements(glue,
                               drawtarget,
                               counts[i],
                               renderasvbo && this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                               (const GLvoid *) offsets[i]);
  }

  if (renderasvbo)
    cc_glglue_glBindBuffer(glue, GL_ELEMENT_ARRAY_BUFFER, 0);
}

// sort an array of three integers
static void sort3(int32_t * arr)
{
  // simple bubble-sort
  int32_t tmp;
  if (arr[1] < arr[0]) {
    tmp = arr[0];
    arr[0] = arr[1];
    arr[1] = tmp;
  }
  if (arr[2] < arr[1]) {
    tmp = arr[1];
    arr[1] = arr[2];
    arr[2] = tmp;
  }
  if (arr[1] < arr[0]) {
    tmp = arr[0];
    arr[0] = arr[1];
    arr[1] = tmp;
  }
}

// sort an array of two integers
static void sort2(int32_t * arr)
{
  int32_t tmp;
  if (arr[1] < arr[0]) {
    tmp = arr[0];
    arr[0] = arr[1];
    arr[1] = tmp;
  }
}

// qsort callback used for sorting triangles based on vertex indices
extern "C" {
static int
compare_triangle(const void * v0, const void * v1)
{
  int i;
  int32_t * t0 = (int32_t*) v0;
  int32_t * t1 = (int32_t*) v1;

  int32_t ti0[3];
  int32_t ti1[3];
  for (i = 0; i < 3; i++) {
    ti0[i] = t0[i];
    ti1[i] = t1[i];
  }
  sort3(ti0);
  sort3(ti1);

  for (i = 0; i < 3; i++) {
    int32_t diff = ti0[i] - ti1[i];
    if (diff != 0) return diff;
  }
  return 0;
}
}

// qsort callback used for sorting lines based on vertex indices
extern "C" {
static int
compare_line(const void * v0, const void * v1)
{
  int i;
  int32_t * t0 = (int32_t*) v0;
  int32_t * t1 = (int32_t*) v1;

  int32_t ti0[2];
  int32_t ti1[2];
  for (i = 0; i < 2; i++) {
    ti0[i] = t0[i];
    ti1[i] = t1[i];
  }
  sort2(ti0);
  sort2(ti1);

  for (i = 0; i < 2; i++) {
    int32_t diff = ti0[i] - ti1[i];
    if (diff != 0) return diff;
  }
  return 0;
}
}

//
// sort triangles to optimize rendering
//
void
SoFCVertexArrayIndexer::sort_triangles(void)
{
#if 0
  // Does this really helps?
#else
  if (!this->indexarray) return;
  // sort triangles based on vertex indices to get more hits in the
  // GPU vertex cache. Not the optimal solution, but should work
  // pretty well. Example: bunny.iv (~70000 triangles) went from 238
  // fps with no sorting to 380 fps with sorting.
  if (this->partarray.size() <= 1) {
    if (this->indexarraylength > 10 * 3)
      qsort((void*) this->indexarray.getWritableArrayPtr(),
            this->indexarraylength / 3,
            sizeof(int32_t) * 3,
            compare_triangle);
  }
  else {
    GLint *indices = this->indexarray.getWritableArrayPtr();
    int prev = 0;
    for (GLint idx : this->partarray) {
      if (idx - prev > 10*3)
        qsort(indices + prev,
              (idx - prev) / 3,
              sizeof(int32_t) * 3,
              compare_triangle);
      prev = idx;
    }
  }
#endif
}

//
// sort lines to optimize rendering
//
void
SoFCVertexArrayIndexer::sort_lines(void)
{
  if (!this->indexarray) return;

  // sort lines based on vertex indices to get more hits in the
  // GPU vertex cache.
  if (this->partarray.size() <= 1) {
    if (this->indexarraylength > 10 * 2)
      qsort((void*) this->indexarray.getWritableArrayPtr(),
            this->indexarraylength / 2,
            sizeof(int32_t) * 2,
            compare_line);
    return;
  }

  // If partarray is not empty, we'll append more index for rendering as
  // GL_LINE_STRIP for each part, which is necessary for GL_LINE_STIPPLE (line
  // pattern) to work on poly lines (because OpenGL line stipple restarts on
  // each line segment).

  bool haslinestrip = false;
  int prev = 0;
  for (int idx : this->partarray) {
    if (idx - prev > 2) {
      // Make sure we have at least one line part with more than two segments
      haslinestrip = true;
      break;
    }
    prev = idx;
  }
  this->linestripoffsets.clear();
  this->linestripcounts.clear();
  if (!haslinestrip)
    return;
  this->linestripoffsets.reserve(this->partarray.size());
  this->linestripcounts.reserve(this->partarray.size());
  this->use_shorts = false;
  prev = 0;
  int typesize = 4;
  for (int idx : this->partarray) {
    if (idx - prev <= 2) {
      this->linestripoffsets.push_back(prev * typesize);
      this->linestripcounts.push_back(idx-prev);
    } else {
      this->linestripoffsets.push_back(this->indexarray.getLength() * typesize);
      this->indexarray.append(this->indexarray[prev]);
      int count = 1;
      for (int i=prev+1; i<idx; i+=2) {
        this->indexarray.append(this->indexarray[i]);
        ++count;
      }
      this->linestripcounts.push_back(count);
    }
    prev = idx;
  }
}

/*!
  Returns the number of indices in the indexer.
*/
int
SoFCVertexArrayIndexer::getNumIndices(void) const
{
  return this->indexarraylength;

}

const GLint *
SoFCVertexArrayIndexer::getIndices(void) const
{
  return this->indexarray ? this->indexarray.getArrayPtr() : NULL;
}

/*!
  Returns a pointer to the index array. It's allowed to reorganize
  these indices to change the rendering order. Calling this function
  will invalidate any VBO caches used by the indexer.
*/
GLint *
SoFCVertexArrayIndexer::getWriteableIndices(void)
{
  if (!this->indexarray) return NULL;
  return this->indexarray.getWritableArrayPtr();
}

void
SoFCVertexArrayIndexer::getBoundingBox(const SbMatrix * matrix,
                                       SbBox3f &bbox,
                                       const SbVec3f * vertices) const
{
  if (!this->indexarray)
    return;

  const GLint * indices = this->indexarray.getArrayPtr();
  if (this->partialindices.empty()) {
    if (matrix) {
      for (int i=0, n=this->indexarraylength; i<n; ++i) {
        SbVec3f v;
        matrix->multVecMatrix(vertices[indices[i]], v);
        bbox.extendBy(v);
      }
    }
    else {
      for (int i=0, n=this->indexarraylength; i<n; ++i)
        bbox.extendBy(vertices[indices[i]]);
    }
    return;
  }

  if (matrix) {
    for (int i : this->partialindices) {
      int j = i ? this->partarray[i-1] : 0;
      for (int end = this->partarray[i]; j < end; ++j) {
        SbVec3f v;
        matrix->multVecMatrix(vertices[indices[j]], v);
        bbox.extendBy(v);
      }
    }
  }
  else {
    for (int i : this->partialindices) {
      int j = i ? this->partarray[i-1] : 0;
      for (int end = this->partarray[i]; j < end; ++j)
        bbox.extendBy(vertices[indices[j]]);
    }
  }
}

void
SoFCVertexArrayIndexer::append(SoFCVertexArrayIndexer *other, int offset)
{
  if (!other->indexarray)
    return;
  int idxoffset = this->indexarraylength;
  for (int i=0, c=other->indexarraylength; i<c; ++i)
    addIndex(other->indexarray[i] + offset);
  for (int i : other->partarray)
    this->partarray.append(i + idxoffset);
}

// vim: noai:ts=2:sw=2
