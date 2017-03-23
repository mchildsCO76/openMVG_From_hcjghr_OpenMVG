
// Copyright (c) 2016 Klemen ISTENIC.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace openMVG  {
namespace VSSLAM  {

using namespace openMVG;
using namespace openMVG::geometry;

  struct MotionModel
  {
    Mat4 velocity_;
    bool bValid_ = false;

    bool isValid()
    {
      return bValid_;
    }
    void updateMotionModel(Frame * prev_frame, Frame * cur_frame)
    {
      // TODO: Check how to calculate if we have relative cameras
      // TODO: Compensate for time difference between frames
      velocity_ = cur_frame->getTransformationMatrix()*prev_frame->getTransformationMatrixInverse();

      bValid_ = true;
    }

    Mat4 predictLocation(Frame * prev_frame)
    {
      // TODO: Check how to predict if we have relative cameras model
      return velocity_ * prev_frame->getTransformationMatrix();
    }

  };

}
}
