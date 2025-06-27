// OvfWriterLib/OvfUtil.h

#pragma once

#include <fstream>
#include <cstdint>
#include "open_vector_format.pb.h"

namespace open_vector_format {
    namespace writer {
    // Forward declaration
    class JobWriter;

    namespace util {

        /**
         * @brief Writes a 64-bit integer to a stream in little-endian byte order.
         * @param value The integer to write.
         * @param os The output stream to write to.
         */
        void WriteLittleEndian(uint64_t value, std::ofstream& os);

        /**
         * @brief Creates a "shell" of a Job message, copying all fields except the work_planes.
         * @param full_job The source Job object.
         * @return A new Job object containing only the shell data.
         */
        Job CreateJobShell(const Job& full_job);

        /**
         * @brief Creates a "shell" of a WorkPlane message, copying all fields except the vector_blocks.
         * @param full_wp The source WorkPlane object.
         * @return A new WorkPlane object containing only the shell data.
         */
        WorkPlane CreateWorkPlaneShell(const WorkPlane& full_wp);
    }
    } // namespace util
} // namespace open_vector_format::writer