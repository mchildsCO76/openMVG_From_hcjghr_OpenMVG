
// Copyright (c) 2016 Klemen ISTENIC.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <openMVG/vsslam/mapping/MapLandmark.hpp>

namespace openMVG  {
namespace VSSLAM  {


  void MapLandmark::updateNormal()
  {
    // Set normal to zero
    normal_.setZero();
    // Compute average of normal
    for (auto & o_i : obs_)
    {
      Vec3 O_frame_i = o_i.second.frame_ptr->getCameraCenter();
      if (ref_frame_ == nullptr)
      {
        Vec3 normal_i = X_ - O_frame_i;
        normal_ = normal_ + normal_i/normal_i.norm();
      }
    }
    normal_ = normal_/obs_.size();
  }

  void MapLandmark::addObservation(Frame * frame, const IndexT & feat_id)
  {
    obs_[frame->getFrameId()] = MapObservation(feat_id,frame);
    updateNormal();
  }

  // Method which defines what is enough quality for the points to be added to the system
  bool MapLandmark::isValidByConnectivityDegree(const size_t & min_degree_landmark) const
  {
    if (n_all_obs_ < min_degree_landmark)
      return false;
    return true;
  }

  bool MapLandmark::hasFrameObservation(const IndexT & frame_id)
  {
    if (obs_.find(frame_id) != obs_.end())
      return true;
    return false;
  }
}
}
