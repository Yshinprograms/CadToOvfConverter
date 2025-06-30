#pragma once

#include "open_vector_format.pb.h" // We need the protobuf message definitions
#include "google/protobuf/util/delimited_message_util.h"

namespace TestFixtures {

    inline open_vector_format::VectorBlock CreateSquareVectorBlock()
    {
        using namespace open_vector_format;

        VectorBlock square_vb;
        auto* line_seq = square_vb.mutable_line_sequence();
        line_seq->add_points(0.0f); line_seq->add_points(0.0f);   // P1 (0,0)
        line_seq->add_points(10.0f); line_seq->add_points(0.0f);  // P2 (10,0)
        line_seq->add_points(10.0f); line_seq->add_points(10.0f); // P3 (10,10)
        line_seq->add_points(0.0f); line_seq->add_points(10.0f);  // P4 (0,10)
        line_seq->add_points(0.0f); line_seq->add_points(0.0f);   // Close loop back to P1

        return square_vb;
    }

    inline bool ReadLittleEndian(std::ifstream& fs, uint64_t& value) {
        char buffer[sizeof(uint64_t)];
        if (!fs.read(buffer, sizeof(uint64_t))) {
            return false;
        }

        // This assumes the machine running the test is little-endian, which is true
        // for any modern Windows/Intel/AMD machine. A more robust solution would
        // handle byte swapping, but this is fine for our unit tests.
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

        // Read the relevant part of the file into a memory buffer
        std::vector<char> buffer(size - offset);
        if (!fs.read(buffer.data(), buffer.size())) return false;

        // Use ArrayInputStream to parse our in-memory buffer, just like the reference reader
        google::protobuf::io::ArrayInputStream ais(buffer.data(), static_cast<int>(buffer.size()));
        return google::protobuf::util::ParseDelimitedFromZeroCopyStream(&message, &ais, nullptr);
    }
}