
// Copyright (c) 2017 Klemen ISTENIC.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "openMVG/cameras/cameras.hpp"
#include "openMVG/types.hpp"

using namespace openMVG;

namespace openMVG  {
namespace VSSLAM  {

class Frame;

enum MAP_POINT_TYPE
{
  GLOBAL_EUCLIDEAN = 0,  // Absolute world reference frame
  LOCAL_INV_DEPTH = 1,  // Relative reference camera reference frame
};
enum MAP_CAMERA_TYPE
{
  GLOBAL = 0, // Absolute world reference frame
  RELATIVE = 1  // Relative reference camera reference frame
};

/// Define a collection of Pose (indexed by frame::frameId_)
using MapFrames = Hash_Map<IndexT, std::shared_ptr<Frame> >;

/// Define a collection of IntrinsicParameter (indexed by id_intrinsic)
using Intrinsics = Hash_Map<IndexT, cameras::IntrinsicBase *>;

/// Observation of a landmark in an image (feature)
struct MapObservation
{
  MapObservation()
  : feat_id (UndefinedIndexT),
    frame_ptr(nullptr)
  {}
  MapObservation(IndexT f_id, Frame * frame)
  : feat_id (f_id),
    frame_ptr(frame)
  {}

  IndexT feat_id;
  Frame * frame_ptr;
};
/// Define a collection of observations of a landmark
using LandmarkObservations = Hash_Map<size_t, MapObservation>;

/// A 3D point with it's associated image observations
struct MapLandmark
{
  MapLandmark()
  : id_(UndefinedIndexT),
    X_(-1,-1,-1),
    normal_(0,0,0),
    ref_frame_(nullptr),
    bestDesc_(nullptr),
    active_(false),
    last_obs_step_(0),
    last_local_map_frame_id_(UndefinedIndexT)
   {}

  IndexT id_;
  Vec3 X_;
  Vec3 normal_; //mean unit vector of all viewing directions (ray that  join the point with optical center of keyframes that observe it)
  Frame * ref_frame_;

  LandmarkObservations obs_;  // map of keyframe_ids and map observation objects
  void * bestDesc_; // Pointer to the best descriptor of the point (most average of them all?)

  size_t last_obs_step_;
  bool active_; // True if the point is in global map

  //Local Map  data
  IndexT last_local_map_frame_id_;  // Id of frame for which the point was last added to local map

  // 1 - initialization point; 2- motion model/reference kf; 3-map tracking point; 4- new triangulated point
  size_t association_type_=0; // Through which tye of association the point was added

  const bool & isActive() const
  {
    return active_;
  }

  void setActive()
  {
    active_ = true;
  }

  const size_t getLastObsStep() const
  {
    return last_obs_step_;
  }
  void setObsStep(const size_t & step)
  {
    last_obs_step_ = step;
  }

  // Method which defines what is enough quality for the points to be added to the system
  bool isValidByConnectivityDegree(const size_t & min_degree_landmark) const
  {
    if (obs_.size() < min_degree_landmark)
      return false;
    return true;
  }

  bool hasFrameObservation(const IndexT & frame_id)
  {
    if (obs_.find(frame_id) != obs_.end())
      return true;
    return false;
  }


};

/// Define a collection of Landmarks of the map (3D reconstructed points)
using MapLandmarks = Hash_Map<IndexT,std::unique_ptr<MapLandmark> >;


}
}
