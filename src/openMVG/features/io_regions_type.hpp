// This file is part of OpenMVG, an Open Multiple View Geometry C++ library.

// Copyright (c) 2015 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_FEATURES_IO_REGIONS_TYPE_HPP
#define OPENMVG_FEATURES_IO_REGIONS_TYPE_HPP

#include <fstream>
#include <string>
#include <vector>

#include "openMVG/features/regions_factory.hpp"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <cereal/archives/json.hpp>

namespace openMVG {
namespace features {

// Init the regions_type from an image describer file (used for regions loading)
inline std::unique_ptr<features::Regions> Init_region_type_from_file
(
  const std::string & sImage_describer_file
)
{
  std::unique_ptr<Regions> regions_type;
  if (stlplus::is_file(sImage_describer_file))
  {
    // Dynamically load the regions type from the file
    std::ifstream stream(sImage_describer_file.c_str());
    if (stream.is_open())
    {
      cereal::JSONInputArchive archive(stream);
      archive(cereal::make_nvp("regions_type", regions_type));
    }
  }
  else // By default init a SIFT regions type (keep compatibility)
  {
    regions_type.reset(new features::SIFT_Regions());
  }
  return regions_type;
}

} // namespace features
} // namespace openMVG

#endif // OPENMVG_FEATURES_IO_REGIONS_TYPE_HPP
