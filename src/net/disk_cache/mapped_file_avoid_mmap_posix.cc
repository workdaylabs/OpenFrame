// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/mapped_file.h"

#include <stdlib.h>

#include "base/files/file_path.h"
#include "base/logging.h"

namespace disk_cache {

void* MappedFile::Init(const base::FilePath& name, size_t size) {
  DCHECK(!init_);
  if (init_ || !File::Init(name))
    return NULL;

  if (!size)
    size = GetLength();

  buffer_ = malloc(size);
  snapshot_ = malloc(size);
  if (buffer_ && snapshot_ && Read(buffer_, size, 0)) {
    memcpy(snapshot_, buffer_, size);
  } else {
    free(buffer_);
    free(snapshot_);
    buffer_ = snapshot_ = 0;
  }

  init_ = true;
  view_size_ = size;
  return buffer_;
}

bool MappedFile::Load(const FileBlock* block) {
  size_t offset = block->offset() + view_size_;
  return Read(block->buffer(), block->size(), offset);
}

bool MappedFile::Store(const FileBlock* block) {
  size_t offset = block->offset() + view_size_;
  return Write(block->buffer(), block->size(), offset);
}

void MappedFile::Flush() {
  DCHECK(buffer_);
  DCHECK(snapshot_);
  const char* buffer_ptr = static_cast<const char*>(buffer_);
  char* snapshot_ptr = static_cast<char*>(snapshot_);
  const size_t block_size = 4096;
  for (size_t offset = 0; offset < view_size_; offset += block_size) {
    size_t size = std::min(view_size_ - offset, block_size);
    if (memcmp(snapshot_ptr + offset, buffer_ptr + offset, size)) {
      memcpy(snapshot_ptr + offset, buffer_ptr + offset, size);
      Write(snapshot_ptr + offset, size, offset);
    }
  }
}

MappedFile::~MappedFile() {
  if (!init_)
    return;

  if (buffer_ && snapshot_) {
    Flush();
  }
  free(buffer_);
  free(snapshot_);
}

}  // namespace disk_cache
