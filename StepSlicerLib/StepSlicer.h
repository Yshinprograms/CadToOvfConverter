#pragma once
#include <string>
#include <vector> // We need this for the return type
#include "GeometryContract.h" // And our contract

namespace geometry {
    class StepSlicer {
    public:
        explicit StepSlicer(const std::string& step_file_path);

        // CORRECTED: Update the signature to match the implementation
        std::vector<geometry_contract::SlicedLayer> Slice(double layer_height);

    private:
        std::string m_file_path;
    };
}