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

#include "GLFillRectOp.h"

#include "gpu/Quad.h"
#include "gpu/QuadPerEdgeAAGeometryProcessor.h"

namespace tgfx {
std::unique_ptr<GeometryProcessor> GLFillRectOp::getGeometryProcessor(const DrawArgs& args) {
  return QuadPerEdgeAAGeometryProcessor::Make(args.renderTarget->width(),
                                              args.renderTarget->height(), args.aa);
}

std::vector<float> GetVertices(const Quad& insetQuad, const Quad& normalInsetQuad,
                               const Quad& outsetQuad, const Quad& normalOutsetQuad) {
  const auto& insetLT = insetQuad.point(0);
  const auto& insetLB = insetQuad.point(1);
  const auto& insetRT = insetQuad.point(2);
  const auto& insetRB = insetQuad.point(3);
  const auto& outsetLT = outsetQuad.point(0);
  const auto& outsetLB = outsetQuad.point(1);
  const auto& outsetRT = outsetQuad.point(2);
  const auto& outsetRB = outsetQuad.point(3);
  return {
      insetLT.x,  insetLT.y,  1.0f, normalInsetQuad.point(0).x,  normalInsetQuad.point(0).y,
      insetLB.x,  insetLB.y,  1.0f, normalInsetQuad.point(1).x,  normalInsetQuad.point(1).y,
      insetRT.x,  insetRT.y,  1.0f, normalInsetQuad.point(2).x,  normalInsetQuad.point(2).y,
      insetRB.x,  insetRB.y,  1.0f, normalInsetQuad.point(3).x,  normalInsetQuad.point(3).y,
      outsetLT.x, outsetLT.y, 0.0f, normalOutsetQuad.point(0).x, normalOutsetQuad.point(0).y,
      outsetLB.x, outsetLB.y, 0.0f, normalOutsetQuad.point(1).x, normalOutsetQuad.point(1).y,
      outsetRT.x, outsetRT.y, 0.0f, normalOutsetQuad.point(2).x, normalOutsetQuad.point(2).y,
      outsetRB.x, outsetRB.y, 0.0f, normalOutsetQuad.point(3).x, normalOutsetQuad.point(3).y,
  };
}

std::vector<float> GLFillRectOp::vertices(const DrawArgs& args) {
  auto normalBounds = Rect::MakeLTRB(0, 0, 1, 1);
  std::vector<float> vertexes;
  // Vertex coordinates are arranged in a 2D pixel coordinate system, and textures are arranged
  // according to a texture coordinate system (0 - 1).
  if (args.aa != AAType::Coverage) {
    for (size_t i = 0; i < rects.size(); ++i) {
      auto quad = Quad::MakeFromRect(rects[i], viewMatrices[i]);
      auto localQuad = Quad::MakeFromRect(normalBounds, localMatrices[i]);
      std::vector<float> vert = {
          quad.point(3).x, quad.point(3).y, localQuad.point(3).x, localQuad.point(3).y,
          quad.point(2).x, quad.point(2).y, localQuad.point(2).x, localQuad.point(2).y,
          quad.point(1).x, quad.point(1).y, localQuad.point(1).x, localQuad.point(1).y,
          quad.point(0).x, quad.point(0).y, localQuad.point(0).x, localQuad.point(0).y,
      };
      vertexes.insert(vertexes.end(), vert.begin(), vert.end());
    }
  } else {
    for (size_t i = 0; i < rects.size(); ++i) {
      auto scale = sqrtf(viewMatrices[i].getScaleX() * viewMatrices[i].getScaleX() +
                         viewMatrices[i].getSkewY() * viewMatrices[i].getSkewY());
      // we want the new edge to be .5px away from the old line.
      auto padding = 0.5f / scale;
      auto insetBounds = rects[i].makeInset(padding, padding);
      auto insetQuad = Quad::MakeFromRect(insetBounds, viewMatrices[i]);
      auto outsetBounds = rects[i].makeOutset(padding, padding);
      auto outsetQuad = Quad::MakeFromRect(outsetBounds, viewMatrices[i]);

      auto normalPadding = Point::Make(padding / rects[i].width(), padding / rects[i].height());
      auto normalInset = normalBounds.makeInset(normalPadding.x, normalPadding.y);
      auto normalInsetQuad = Quad::MakeFromRect(normalInset, localMatrices[i]);
      auto normalOutset = normalBounds.makeOutset(normalPadding.x, normalPadding.y);
      auto normalOutsetQuad = Quad::MakeFromRect(normalOutset, localMatrices[i]);

      auto vert = GetVertices(insetQuad, normalInsetQuad, outsetQuad, normalOutsetQuad);
      vertexes.insert(vertexes.end(), vert.begin(), vert.end());
    }
  }
  return vertexes;
}

std::unique_ptr<GLFillRectOp> GLFillRectOp::Make(const Rect& rect, const Matrix& viewMatrix) {
  return std::unique_ptr<GLFillRectOp>(new GLFillRectOp({rect}, {viewMatrix}, {Matrix::I()}));
}

std::unique_ptr<GLFillRectOp> GLFillRectOp::Make(const std::vector<Rect>& rects,
                                                 const std::vector<Matrix>& viewMatrices,
                                                 const std::vector<Matrix>& localMatrices) {
  return std::unique_ptr<GLFillRectOp>(new GLFillRectOp(rects, viewMatrices, localMatrices));
}

GLFillRectOp::GLFillRectOp(std::vector<Rect> rects, std::vector<Matrix> viewMatrices,
                           std::vector<Matrix> localMatrices)
    : rects(std::move(rects)),
      viewMatrices(std::move(viewMatrices)),
      localMatrices(std::move(localMatrices)) {
  auto bounds = Rect::MakeEmpty();
  for (size_t i = 0; i < rects.size(); ++i) {
    bounds.join(viewMatrices[i].mapRect(rects[i]));
  }
  setBounds(bounds);
}

static constexpr size_t kIndicesPerAAFillRect = 30;

// clang-format off
static constexpr uint16_t gFillAARectIdx[] = {
  0, 1, 2, 1, 3, 2,
  0, 4, 1, 4, 5, 1,
  0, 6, 4, 0, 2, 6,
  2, 3, 6, 3, 7, 6,
  1, 5, 3, 3, 5, 7,
};
// clang-format on

std::shared_ptr<GLBuffer> GLFillRectOp::getIndexBuffer(const DrawArgs& args) {
  if (rects.size() > 1) {
    std::vector<uint16_t> indexes;
    for (size_t i = 0; i < rects.size(); ++i) {
      if (args.aa != AAType::Coverage) {
        indexes.push_back(i * 4);
        indexes.push_back(i * 4 + 1);
        indexes.push_back(i * 4 + 2);
        indexes.push_back(i * 4 + 1);
        indexes.push_back(i * 4 + 2);
        indexes.push_back(i * 4 + 3);
      } else {
        for (auto j : gFillAARectIdx) {
          indexes.push_back(i * 8 + j);
        }
      }
    }
    return GLBuffer::Make(args.context, &indexes[0], indexes.size());
  }
  if (args.aa == AAType::Coverage) {
    return GLBuffer::Make(args.context, gFillAARectIdx, kIndicesPerAAFillRect);
  }
  return nullptr;
}

}  // namespace tgfx
