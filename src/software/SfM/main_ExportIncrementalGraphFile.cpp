
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdlib>

#include "openMVG/sfm/sfm.hpp"

#include "openMVG/sfm/pipelines/incremental/incremental_SfM.hpp"
#include "openMVG/system/timer.hpp"
#include "openMVG/cameras/Cameras_Common_command_line_helper.hpp"

#include "third_party/cmdLine/cmdLine.h"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

using namespace openMVG;
using namespace openMVG::cameras;
using namespace openMVG::sfm;

/// From 2 given image file-names, find the two corresponding index in the View list
bool computeIndexFromImageNames(
  const SfM_Data & sfm_data,
  const std::pair<std::string,std::string>& initialPairName,
  Pair& initialPairIndex)
{
  if (initialPairName.first == initialPairName.second)
  {
    std::cerr << "\nInvalid image names. You cannot use the same image to initialize a pair." << std::endl;
    return false;
  }

  initialPairIndex = Pair(UndefinedIndexT, UndefinedIndexT);

  /// List views filenames and find the one that correspond to the user ones:
  for (Views::const_iterator it = sfm_data.GetViews().begin();
    it != sfm_data.GetViews().end(); ++it)
  {
    const View * v = it->second.get();
    const std::string filename = stlplus::filename_part(v->s_Img_path);
    if (filename == initialPairName.first)
    {
      initialPairIndex.first = v->id_view;
    }
    else{
      if (filename == initialPairName.second)
      {
        initialPairIndex.second = v->id_view;
      }
    }
  }
  return (initialPairIndex.first != UndefinedIndexT &&
      initialPairIndex.second != UndefinedIndexT);
}


int main(int argc, char **argv)
{
  using namespace std;
  std::cout << "Export incremental graph file" << std::endl
            << " " << std::endl
            << std::endl;

  CmdLine cmd;

  std::string sSfM_Data_Filename;
  std::string sMatchesDir;
  std::string sOutDir = "";
  std::pair<std::string,std::string> initialPairString("","");
  std::string sIntrinsic_refinement_options = "ADJUST_ALL";
  int i_User_camera_model = PINHOLE_CAMERA_RADIAL3;

  // Ordered incremental reconstruction settings
  size_t iWindowSize = 5;
  bool bOrderedProcessing = true;
  bool bTryAllViewsAtEnd = true;
  
  

  // Graph settings
  std::string graphFileDir;
  bool bPerformGlobalBA = false;
  bool bPerformLocalPoseBA = false;
  bool bPerformGlobalOutlierRemoval = false;
  bool bPerformLocalOutlierRemoval = false;
  bool bPerformConsistencyCheck = false;

  int iCamVertexType = 0;
  int iLandmarkVertexType = 0;
  bool bExportTwoFoldGraphFile = true;

  cmd.add( make_option('i', sSfM_Data_Filename, "input_file") );
  cmd.add( make_option('m', sMatchesDir, "matchdir") );
  cmd.add( make_option('o', sOutDir, "outdir") );
  cmd.add( make_option('a', initialPairString.first, "initialPairA") );
  cmd.add( make_option('b', initialPairString.second, "initialPairB") );
  cmd.add( make_option('c', i_User_camera_model, "camera_model") );
  cmd.add( make_option('f', sIntrinsic_refinement_options, "refineIntrinsics") );

  cmd.add( make_option('y', bOrderedProcessing, "ordered_processing") );
  cmd.add( make_option('w', iWindowSize, "order_window_size") );
  cmd.add( make_option('q', bTryAllViewsAtEnd, "try_all_views") );

  cmd.add( make_option('s', graphFileDir, "graph_file") );
  cmd.add( make_option('t', bExportTwoFoldGraphFile, "twofold_graph_file") );

  cmd.add( make_option('u', iCamVertexType, "camera_vertex_type") );
  cmd.add( make_option('v', iLandmarkVertexType, "landmark_vertex_type") );

  cmd.add( make_option('g', bPerformGlobalBA, "globalBA") );
  cmd.add( make_option('l', bPerformLocalPoseBA, "localPoseBA") );
  cmd.add( make_option('e', bPerformGlobalOutlierRemoval, "global_outlier_removal") );
  cmd.add( make_option('h', bPerformLocalOutlierRemoval, "local_outlier_removal") );
  cmd.add( make_option('z', bPerformConsistencyCheck, "consistency_check") );



  try {
    if (argc == 1) throw std::string("Invalid parameter.");
    cmd.process(argc, argv);
  } catch(const std::string& s) {
    std::cerr << "Usage: " << argv[0] << '\n'
    << "[-i|--input_file] path to a SfM_Data scene\n"
    << "[-m|--matchdir] path to the matches that corresponds to the provided SfM_Data scene\n"
    << "[-o|--outdir] path where the output data will be stored\n"
    << "[-a|--initialPairA] filename of the first image (without path)\n"
    << "[-b|--initialPairB] filename of the second image (without path)\n"
    << "[-c|--camera_model] Camera model type for view with unknown intrinsic:\n"
      << "\t 1: Pinhole \n"
      << "\t 2: Pinhole radial 1\n"
      << "\t 3: Pinhole radial 3 (default)\n"
      << "\t 4: Pinhole radial 3 + tangential 2\n"
      << "\t 5: Pinhole fisheye\n"
    << "[-f|--refineIntrinsics] Intrinsic parameters refinement option\n"
    << "[-y|--ordered_processing] Follow the list of views or always select the best one (better but slower) (default: true)\n"
    << "[-w|--order_window_size] Size of window from which the next views are considered (default: 5)\n"
    << "[-q|--try_all_views] After ordered processing we try again with all the remaining views (default: false)\n"
      << "\t ADJUST_ALL -> refine all existing parameters (default) \n"
      << "\t NONE -> intrinsic parameters are held as constant\n"
      << "\t ADJUST_FOCAL_LENGTH -> refine only the focal length\n"
      << "\t ADJUST_PRINCIPAL_POINT -> refine only the principal point position\n"
      << "\t ADJUST_DISTORTION -> refine only the distortion coefficient(s) (if any)\n"
      << "\t -> NOTE: options can be combined thanks to '|'\n"
      << "\t ADJUST_FOCAL_LENGTH|ADJUST_PRINCIPAL_POINT\n"
      <<      "\t\t-> refine the focal length & the principal point position\n"
      << "\t ADJUST_FOCAL_LENGTH|ADJUST_DISTORTION\n"
      <<      "\t\t-> refine the focal length & the distortion coefficient(s) (if any)\n"
      << "\t ADJUST_PRINCIPAL_POINT|ADJUST_DISTORTION\n"
      <<      "\t\t-> refine the principal point position & the distortion coefficient(s) (if any)\n"
    << "[-s|--graphdir] path where incremental graphfile and other related files will be stored\n"
    << "[-u|--camera_vertex_type] Type of exported camera vertex (in graph file):\n"
      << "\t 0: SE(3) in global reference frame (default)\n"
      << "\t 1: Sim(3) in global reference frame \n"
    << "[-v|--landmark_vertex_type] Type of exported landmark vertex (in graph file):\n"
      << "\t 0: XYZ in global reference frame (default) \n"
      << "\t 1: Inverse depth in relative reference frame (second observing camera is reference) \n"
    << "[-g|--globalBA] Perform global BA after each iteration (default: false)]\n"
    << "[-l|--localPoseBA] Perform local BA of each camera pose added (default: false)]\n"
    << "[-e|--global_outlier_removal] Perform global outlier removal after global BA (default: false)]\n"
    << "[-h|--local_outlier_removal] Perform local outlier removal of measurements (if false all measurements are added) (default: false)]\n"
    << "[-z|--consistency_check] After each interation perform check if loggind is correct (default: false)]\n"
    << std::endl;

    std::cerr << s << std::endl;
    return EXIT_FAILURE;
  }

  if (i_User_camera_model < PINHOLE_CAMERA ||
      i_User_camera_model > PINHOLE_CAMERA_FISHEYE )  {
    std::cerr << "\n Invalid camera type" << std::endl;
    return EXIT_FAILURE;
  }

  const cameras::Intrinsic_Parameter_Type intrinsic_refinement_options =
    cameras::StringTo_Intrinsic_Parameter_Type(sIntrinsic_refinement_options);

  // Load input SfM_Data scene
  SfM_Data sfm_data;
  if (!Load(sfm_data, sSfM_Data_Filename, ESfM_Data(VIEWS|INTRINSICS))) {
    std::cerr << std::endl
      << "The input SfM_Data file \""<< sSfM_Data_Filename << "\" cannot be read." << std::endl;
    return EXIT_FAILURE;
  }

  // Init the regions_type from the image describer file (used for image regions extraction)
  using namespace openMVG::features;
  const std::string sImage_describer = stlplus::create_filespec(sMatchesDir, "image_describer", "json");
  std::unique_ptr<Regions> regions_type = Init_region_type_from_file(sImage_describer);
  if (!regions_type)
  {
    std::cerr << "Invalid: "
      << sImage_describer << " regions type file." << std::endl;
    return EXIT_FAILURE;
  }

  // Features reading
  std::shared_ptr<Features_Provider> feats_provider = std::make_shared<Features_Provider>();
  if (!feats_provider->load(sfm_data, sMatchesDir, regions_type)) {
    std::cerr << std::endl
      << "Invalid features." << std::endl;
    return EXIT_FAILURE;
  }
  // Matches reading
  std::shared_ptr<Matches_Provider> matches_provider = std::make_shared<Matches_Provider>();
  if // Try to read the two matches file formats
  (
    !(matches_provider->load(sfm_data, stlplus::create_filespec(sMatchesDir, "matches.f.txt")) ||
      matches_provider->load(sfm_data, stlplus::create_filespec(sMatchesDir, "matches.f.bin")))
  )
  {
    std::cerr << std::endl
      << "Invalid matches file." << std::endl;
    return EXIT_FAILURE;
  }

  if (sOutDir.empty())  {
    std::cerr << "\nIt is an invalid output directory" << std::endl;
    return EXIT_FAILURE;
  }

  if (!stlplus::folder_exists(sOutDir))
  {
    if (!stlplus::folder_create(sOutDir))
    {
      std::cerr << "\nCannot create the output directory" << std::endl;
    }
  }

  //---------------------------------------
  // Sequential reconstruction process
  //---------------------------------------

  openMVG::system::Timer timer;
  IncrementalSfMReconstructionEngine sfmEngine(
    sfm_data,
    sOutDir,
    stlplus::create_filespec(sOutDir, "Reconstruction_Report.html"));

  // Configure the features_provider & the matches_provider
  sfmEngine.SetFeaturesProvider(feats_provider.get());
  sfmEngine.SetMatchesProvider(matches_provider.get());

  // Configure reconstruction parameters
  sfmEngine.Set_Intrinsics_Refinement_Type(intrinsic_refinement_options);
  sfmEngine.SetUnknownCameraType(EINTRINSIC(i_User_camera_model));


  // Set SlamPP logging data
  if (graphFileDir.empty())
    sfmEngine.setGraphFileOutputFolder(sOutDir);
  else
    sfmEngine.setGraphFileOutputFolder(graphFileDir);
  
  sfmEngine.setBAOptimizations(bPerformGlobalBA, bPerformLocalPoseBA, bPerformLocalPoseBA);
  sfmEngine.setConsistencyCheck(bPerformConsistencyCheck);
  sfmEngine.setOutlierRemoval(bPerformGlobalOutlierRemoval,bPerformLocalOutlierRemoval);
  sfmEngine.setGraphVertexOutputTypes(iCamVertexType,iLandmarkVertexType);
  sfmEngine.setExportTwoFoldGraphFile(bExportTwoFoldGraphFile);
  
  sfmEngine.setProcessingOrder(bOrderedProcessing, iWindowSize, bTryAllViewsAtEnd);


  // Handle Initial pair parameter
  if (!initialPairString.first.empty() && !initialPairString.second.empty())
  {
    Pair initialPairIndex;
    if(!computeIndexFromImageNames(sfm_data, initialPairString, initialPairIndex))
    {
        std::cerr << "Could not find the initial pairs <" << initialPairString.first
          <<  ", " << initialPairString.second << ">!\n";
      return EXIT_FAILURE;
    }
    sfmEngine.setInitialPair(initialPairIndex);
  }

  if (sfmEngine.Process())
  {
    std::cout << std::endl << " Total Ac-Sfm took (s): " << timer.elapsed() << std::endl;

    std::cout << "...Generating SfM_Report.html" << std::endl;
    Generate_SfM_Report(sfmEngine.Get_SfM_Data(),
      stlplus::create_filespec(sOutDir, "SfMReconstruction_Report.html"));

    //-- Export to disk computed scene (data & visualizable results)
    std::cout << "...Export SfM_Data to disk." << std::endl;
    Save(sfmEngine.Get_SfM_Data(),
      stlplus::create_filespec(sOutDir, "sfm_data", ".bin"),
      ESfM_Data(ALL));

    Save(sfmEngine.Get_SfM_Data(),
      stlplus::create_filespec(sOutDir, "cloud_and_poses", ".ply"),
      ESfM_Data(ALL));

    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}