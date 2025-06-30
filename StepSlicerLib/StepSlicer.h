// StepSlicerLib/StepSlicer.h

#pragma once

#include <string>
#include "GeometryContract.h"

// We'll define our own simple namespace for this library
// to keep things cleanly separated.
namespace geometry {

    class StepSlicer {
    public:
        // Constructor takes the path to the STEP file.
        explicit StepSlicer(const std::string& step_file_path);

        // This will be the main function we call to perform the slicing.
        // We will define the SlicedData struct later. For now, a placeholder.
        void Slice();

    private:
        std::string m_file_path;
    };

} // namespace geometry