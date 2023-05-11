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

#ifndef FC_COIN_VERTEXARRAYINDEXER_H
#define FC_COIN_VERTEXARRAYINDEXER_H

#include <vector>
#include <set>
#include <map>

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>
#include "SoFCVertexAttribute.h"
#include "COWData.h"

class SoState;
class SbBox3f;
class SbVec3f;
class SbMatrix;
class SoFCVertexCache;

class SoFCVertexArrayIndexer {
public:
  typedef SoFCVertexAttribute<GLint, GLushort> IndexArray;

  SoFCVertexArrayIndexer();
  SoFCVertexArrayIndexer(const IndexArray &initarray);

  SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                         const std::map<int, int> & partindices,
                         int maxindex,
                         bool exclude = false);

  SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                         const std::set<int> & partindices,
                         int maxindex,
                         bool exclude = false);

  SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                         const SbFCVector<int> & partindices,
                         int maxindex,
                         bool exclude = false);

  ~SoFCVertexArrayIndexer();

  static void initClass();
  static void cleanup();

  void addTriangle(const int32_t v0,
                   const int32_t v1,
                   const int32_t v2);

  void addLine(const int32_t v0,
               const int32_t v1,
               int lineindex);

  void addPoint(const int32_t v0);

  void close(const int *parts=nullptr, int count=0);
  void close(SbFCVector<int> && parts);

  void render(SoState * state,
              const cc_glglue * glue,
              bool renderasvbo,
              uint32_t vbocontextid,
              const intptr_t * offsets = NULL,
              const int32_t * counts = NULL,
              int32_t draw_count = 0);

  const IndexArray & getIndexArray() const { return indexarray; }

  const SbFCVector<int> & getPartialIndices() const { return partialindices; }

  int getNumIndices(void) const;
  const GLint * getIndices(void) const;
  GLint * getWriteableIndices(void);

  int getNumParts(void) const;
  const int * getPartOffsets(void) const;

  GLenum getTarget(void) const {return this->target;}

  SbBool useShorts() const {return this->use_shorts;}

  void getBoundingBox(const SbMatrix * matrix, SbBox3f &bbox, const SbVec3f * vertices) const;

  void append(SoFCVertexArrayIndexer *other, int offset);

private:
  template<class IndicesT, class GetT>
  void init(const SoFCVertexArrayIndexer & other,
            const IndicesT & partindices,
            GetT && getT,
            int maxindex,
            bool exclude);

private:
  void addIndex(int32_t i);
  void sort_triangles(void);
  void sort_lines(void);

  friend class SoFCVertexCache;

  GLenum target;

  COWVector<int> partarray;
  COWVector<intptr_t> linestripoffsets;
  COWVector<int32_t> linestripcounts;

  SbFCVector<int> partialindices;
  SbFCVector<intptr_t> partialoffsets;
  SbFCVector<int32_t> partialcounts;

  IndexArray indexarray;
  int indexarraylength;
  int lastlineindex;
  bool vbo_used = false;
  SbBool use_shorts;
};

#endif // FC_COIN_VERTEXARRAYINDEXER_H
