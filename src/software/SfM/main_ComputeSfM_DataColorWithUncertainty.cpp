// Copyright (c) 2015 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "openMVG/sfm/sfm.hpp"
#include "openMVG/sfm/sfm_uncertainty.hpp"
#include "openMVG/stl/stl.hpp"
#include "software/SfM/SfMPlyHelper.hpp"

#include "third_party/cmdLine/cmdLine.h"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <string>
#include <vector>

using namespace openMVG;
using namespace openMVG::image;
using namespace openMVG::sfm;
using namespace openMVG::cameras;
using namespace openMVG::geometry;

/// Find the color of the SfM_Data Landmarks/structure
bool ColorizeTracks(
  const SfM_Data & sfm_data,
  std::vector<Vec3> & vec_3dPoints,
  std::vector<Vec3> & vec_tracksColor)
{
  // Colorize each track
  //  Start with the most representative image
  //    and iterate to provide a color to each 3D point

  {
    C_Progress_display my_progress_bar(sfm_data.GetLandmarks().size(),
                                       std::cout,
                                       "\nCompute scene structure color\n");

    vec_tracksColor.resize(sfm_data.GetLandmarks().size());
    vec_3dPoints.resize(sfm_data.GetLandmarks().size());

    //Build a list of contiguous index for the trackIds
    std::map<IndexT, IndexT> trackIds_to_contiguousIndexes;
    IndexT cpt = 0;
    for (Landmarks::const_iterator it = sfm_data.GetLandmarks().begin();
      it != sfm_data.GetLandmarks().end(); ++it, ++cpt)
    {
      trackIds_to_contiguousIndexes[it->first] = cpt;
      vec_3dPoints[cpt] = it->second.X;
    }

    // The track list that will be colored (point removed during the process)
    std::set<IndexT> remainingTrackToColor;
    std::transform(sfm_data.GetLandmarks().begin(), sfm_data.GetLandmarks().end(),
      std::inserter(remainingTrackToColor, remainingTrackToColor.begin()),
      stl::RetrieveKey());

    while( !remainingTrackToColor.empty() )
    {
      // Find the most representative image (for the remaining 3D points)
      //  a. Count the number of observation per view for each 3Dpoint Index
      //  b. Sort to find the most representative view index

      std::map<IndexT, IndexT> map_IndexCardinal; // ViewId, Cardinal
      for (std::set<IndexT>::const_iterator
        iterT = remainingTrackToColor.begin();
        iterT != remainingTrackToColor.end();
        ++iterT)
      {
        const size_t trackId = *iterT;
        const Observations & obs = sfm_data.GetLandmarks().at(trackId).obs;
        for( Observations::const_iterator iterObs = obs.begin();
          iterObs != obs.end(); ++iterObs)
        {
          const size_t viewId = iterObs->first;
          if (map_IndexCardinal.find(viewId) == map_IndexCardinal.end())
            map_IndexCardinal[viewId] = 1;
          else
            ++map_IndexCardinal[viewId];
        }
      }

      // Find the View index that is the most represented
      std::vector<IndexT> vec_cardinal;
      std::transform(map_IndexCardinal.begin(),
        map_IndexCardinal.end(),
        std::back_inserter(vec_cardinal),
        stl::RetrieveValue());
      using namespace stl::indexed_sort;
      std::vector< sort_index_packet_descend< IndexT, IndexT> > packet_vec(vec_cardinal.size());
      sort_index_helper(packet_vec, &vec_cardinal[0], 1);

      // First image index with the most of occurence
      std::map<IndexT, IndexT>::const_iterator iterTT = map_IndexCardinal.begin();
      std::advance(iterTT, packet_vec[0].index);
      const size_t view_index = iterTT->first;
      const View * view = sfm_data.GetViews().at(view_index).get();
      const std::string sView_filename = stlplus::create_filespec(sfm_data.s_root_path,
        view->s_Img_path);
      Image<RGBColor> image_rgb;
      Image<unsigned char> image_gray;
      const bool b_rgb_image = ReadImage(sView_filename.c_str(), &image_rgb);
      if (!b_rgb_image) //try Gray level
      {
        const bool b_gray_image = ReadImage(sView_filename.c_str(), &image_gray);
        if (!b_gray_image)
        {
          std::cerr << "Cannot open provided the image." << std::endl;
          return false;
        }
      }

      // Iterate through the remaining track to color
      // - look if the current view is present to color the track
      std::set<IndexT> set_toRemove;
      for (std::set<IndexT>::const_iterator
        iterT = remainingTrackToColor.begin();
        iterT != remainingTrackToColor.end();
        ++iterT)
      {
        const size_t trackId = *iterT;
        const Observations & obs = sfm_data.GetLandmarks().at(trackId).obs;
        Observations::const_iterator it = obs.find(view_index);

        if (it != obs.end())
        {
          // Color the track
          const Vec2 & pt = it->second.x;
          const RGBColor color = b_rgb_image ? image_rgb(pt.y(), pt.x()) : RGBColor(image_gray(pt.y(), pt.x()));

          vec_tracksColor[ trackIds_to_contiguousIndexes[trackId] ] = Vec3(color.r(), color.g(), color.b());
          set_toRemove.insert(trackId);
          ++my_progress_bar;
        }
      }
      // Remove colored track
      for (std::set<IndexT>::const_iterator iter = set_toRemove.begin();
        iter != set_toRemove.end(); ++iter)
      {
        remainingTrackToColor.erase(*iter);
      }
    }
  }
  return true;
}

/// Export camera poses positions as a Vec3 vector
void GetCameraPositions(const SfM_Data & sfm_data, std::vector<Vec3> & vec_camPosition)
{
  for (const auto & view : sfm_data.GetViews())
  {
    if (sfm_data.IsPoseAndIntrinsicDefined(view.second.get()))
    {
      const geometry::Pose3 pose = sfm_data.GetPoseOrDie(view.second.get());
      vec_camPosition.push_back(pose.center());
    }
  }
}
/*
/// Export uncertainty of points as double vector
void GetUncertaintyStructure(const SfM_Data & sfm_data, std::vector<double> & vec_structureUncertainty)
{
  IndexT cpt = 0;
  for (Landmarks::const_iterator it = sfm_data.GetLandmarks().begin();
    it != sfm_data.GetLandmarks().end(); ++it, ++cpt)
  {
    // Compute the confidence (95%) ellipsoid and compute its volume
    Eigen::Vector3d eigen_values = (it->second.covariance).eigenvalues().real();
    // Parameters of ellipsoid
    double a_ellipsoid = sqrt(eigen_values(0)*7.815)*2;
    double b_ellipsoid = sqrt(eigen_values(1)*7.815)*2;
    double c_ellipsoid = sqrt(eigen_values(2)*7.815)*2;
    // Volume
    double quality = (1.33333f * M_PI * a_ellipsoid * b_ellipsoid  * c_ellipsoid);
    
    /*
    // Trace of covariance matrix
    double quality = (it->second.covariance).trace();
    //double qualityB = sfm_data.structure.at(cpt).covariance.trace();
    std::cout<<"TT: "<<quality<<" :: "<<"\n";
        //* (it->second.meanReprojError);
    */
/*
    double meanReprojError = 0.0;
    const Observations & obs = it->second.obs;
    for (Observations::const_iterator itObs = obs.begin();
      itObs != obs.end(); ++itObs)
    {
      const Observation obs = itObs->second;
      const IndexT view_id = itObs->first;
      const View * view = sfm_data.GetViews().at(view_id).get();
      const IntrinsicBase * cam = sfm_data.GetIntrinsics().at(view->id_intrinsic).get();
      const Pose3 pose = sfm_data.GetPoseOrDie(view);
      meanReprojError += cam->residual(pose,it->second.X,cam->get_ud_pixel(itObs->second.x)).squaredNorm();
    }
    meanReprojError /=obs.size();
  
    quality = quality * meanReprojError;

    vec_structureUncertainty.push_back(quality);



  }
}
*/
// Convert from a SfM_Data format to another
int main(int argc, char **argv)
{
  CmdLine cmd;

  std::string
    sSfM_Data_Filename_In,
    sOutputPLY_Out;

  cmd.add(make_option('i', sSfM_Data_Filename_In, "input_file"));
  cmd.add(make_option('o', sOutputPLY_Out, "output_file"));

  try {
      if (argc == 1) throw std::string("Invalid command line parameter.");
      cmd.process(argc, argv);
  } catch(const std::string& s) {
      std::cerr << "Usage: " << argv[0] << '\n'
        << "[-i|--input_file] path to the input SfM_Data scene\n"
        << "[-o|--output_file] path to the output PLY file\n"
        << std::endl;

      std::cerr << s << std::endl;
      return EXIT_FAILURE;
  }

  if (sOutputPLY_Out.empty())
  {
    std::cerr << std::endl
      << "No output PLY filename specified." << std::endl;
    return EXIT_FAILURE;
  }

  // Load input SfM_Data scene
  SfM_Data sfm_data;
  if (!Load(sfm_data, sSfM_Data_Filename_In, ESfM_Data(ALL|UNCERTAINTY)))
  {
    std::cerr << std::endl
      << "The input SfM_Data file \"" << sSfM_Data_Filename_In << "\" cannot be read." << std::endl;
    return EXIT_FAILURE;
  }

  // Compute the scene structure color
  std::vector<Vec3> vec_3dPoints, vec_tracksColor, vec_camPosition;
  std::vector<double> vec_structureQuality;
  if (ColorizeTracks(sfm_data, vec_3dPoints, vec_tracksColor))
  {
    GetCameraPositions(sfm_data, vec_camPosition);
    EstimateQualityOfStructure(sfm_data,vec_structureQuality);

    // Export the SfM_Data scene in the expected format
    if (plyHelper::exportToPly(vec_3dPoints, vec_camPosition, sOutputPLY_Out, &vec_tracksColor,&vec_structureQuality))
    {
      return EXIT_SUCCESS;
    }
    else
    {
      return EXIT_FAILURE;
    }
  }
  else
  {
    return EXIT_FAILURE;
  }
}
