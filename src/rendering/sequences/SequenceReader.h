/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "base/utils/Task.h"
#include "gpu/Texture.h"
#include "rendering/Performance.h"

namespace pag {
class RenderCache;

class SequenceReader {
 public:
  explicit SequenceReader(Frame totalFrames) : totalFrames(totalFrames) {
  }

  virtual ~SequenceReader();

  /**
   * Returns true if the TextureReader contains only one frame.
   */
  bool staticContent() const {
    return totalFrames == 1;
  }

  /**
   * Decodes the specified target frame asynchronously.
   */
  void prepare(Frame targetFrame);

  /**
   * Returns the texture of specified target frame.
   */
  std::shared_ptr<tgfx::Texture> readTexture(Frame targetFrame, RenderCache* cache);

 protected:
  /**
   * Decodes the closest frame to the specified targetTime.
   */
  virtual bool decodeFrame(Frame targetFrame) = 0;

  /**
   * Returns the texture of current decoded frame.
   */
  virtual std::shared_ptr<tgfx::Texture> makeTexture(tgfx::Context* context) = 0;

  /**
   * Reports the decoding performance data.
   */
  virtual void recordPerformance(Performance* performance, int64_t decodingTime) = 0;

 protected:
  std::shared_ptr<Task> lastTask = nullptr;

 private:
  Frame totalFrames = 0;
  Frame pendingFirstFrame = -1;
  Frame lastFrame = -1;
  std::shared_ptr<tgfx::Texture> lastTexture = nullptr;

  friend class SequenceTask;

  friend class RenderCache;
};
}  // namespace pag
