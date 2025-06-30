#pragma once

#include "open_vector_format.pb.h"
#include <fstream>
#include <vector>
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/util/delimited_message_util.h"

namespace TestFixtures {

    inline open_vector_format::VectorBlock CreateSquareVectorBlock()
    {
        using namespace open_vector_format;
        VectorBlock square_vb;
        auto* line_seq = square_vb.mutable_line_sequence();
        line_seq->add_points(0.0f); line_seq->add_points(0.0f);
        line_seq->add_points(10.0f); line_seq->add_points(0.0f);
        line_seq->add_points(10.0f); line_seq->add_points(10.0f);
        line_seq->add_points(0.0f); line_seq->add_points(10.0f);
        line_seq->add_points(0.0f); line_seq->add_points(0.0f);
        return square_vb;
    }

    inline open_vector_format::VectorBlock CreateTriangleVectorBlock()
    {
        using namespace open_vector_format;
        VectorBlock triangle_vb;
        auto* line_seq = triangle_vb.mutable_line_sequence();
        line_seq->add_points(0.0f); line_seq->add_points(0.0f);
        line_seq->add_points(5.0f); line_seq->add_points(10.0f);
        line_seq->add_points(10.0f); line_seq->add_points(0.0f);
        line_seq->add_points(0.0f); line_seq->add_points(0.0f);
        return triangle_vb;
    }

    inline bool ReadLittleEndian(std::ifstream& fs, uint64_t& value)
    {
        char buffer[sizeof(uint64_t)];
        if (!fs.read(buffer, sizeof(uint64_t))) return false;
        value = *reinterpret_cast<uint64_t*>(buffer);
        return true;
    }

    template<typename T>
    inline bool ReadDelimitedFromOffset(const std::string& filepath, uint64_t offset, T& message)
    {
        std::ifstream fs(filepath, std::ios::binary | std::ios::ate);
        if (!fs.good()) return false;
        std::streamsize size = fs.tellg();
        fs.seekg(offset);
        if (offset >= size) return false;
        std::vector<char> buffer(size - offset);
        if (!fs.read(buffer.data(), buffer.size())) return false;
        google::protobuf::io::ArrayInputStream ais(buffer.data(), static_cast<int>(buffer.size()));
        return google::protobuf::util::ParseDelimitedFromZeroCopyStream(&message, &ais, nullptr);
    }
}