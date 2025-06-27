// OvfWriterLib/OvfWriter.h

#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <stdexcept>
#include "open_vector_format.pb.h"
#include "ovf_lut.pb.h"

namespace open_vector_format {
    namespace writer {

        // Forward-declare WorkPlaneWriter so JobWriter can use it.
        class WorkPlaneWriter;

        /**
         * @brief Manages the top-level scope of writing an OVF file.
         *
         * Follows the RAII principle: the constructor opens the file and writes the
         * header, and the destructor finalizes the file by writing the Job shell
         * and Look-Up-Table, then closing the stream.
         */
        class JobWriter {
        public:
            /**
             * @brief Constructs a JobWriter and begins the file writing process.
             * @param path The path to the output .ovf file.
             * @param job_shell A Job protobuf message containing all metadata. Any
             *                  work_planes within this message will be ignored.
             */
            JobWriter(const std::string& path, const Job& job_shell);

            /**
             * @brief Finalizes and closes the OVF file.
             */
            ~JobWriter();

            /**
             * @brief Appends a new WorkPlane to the job.
             * @param work_plane_shell A WorkPlane message with its metadata.
             *                         Any vector_blocks will be ignored.
             * @return A WorkPlaneWriter object to manage the scope of this new workplane.
             */
            WorkPlaneWriter AppendWorkPlane(const WorkPlane& work_plane_shell);

            // This class manages a file handle, so it should not be copied or moved.
            JobWriter(const JobWriter&) = delete;
            JobWriter& operator=(const JobWriter&) = delete;

        private:
            // Grant WorkPlaneWriter access to our private members, like the stream.
            friend class WorkPlaneWriter;

            void Finalize();

            std::ofstream m_stream;
            Job m_job_shell_state;
            JobLUT m_job_lut;
            std::streampos m_job_lut_offset_pos; // The position where the offset to the JobLUT is stored.
            bool m_is_finalized = false;
        };


        /**
         * @brief Manages the scope of writing a single WorkPlane within a Job.
         *
         * Follows the RAII principle: its lifetime is tied to a single workplane.
         * The destructor writes the WorkPlane's shell and its Look-Up-Table.
         * This class is MOVE-ONLY to ensure clear ownership.
         */
        class WorkPlaneWriter {
        public:
            /**
             * @brief Appends a VectorBlock to the current WorkPlane.
             * @param vb The VectorBlock to write.
             */
            void AppendVectorBlock(const VectorBlock& vb);

            // --- RAII and Move Semantics ---
            // The destructor is where the magic happens for finalizing the WorkPlane.
            ~WorkPlaneWriter();

            // This class is movable, allowing it to be returned from functions.
            WorkPlaneWriter(WorkPlaneWriter&& other) noexcept;
            WorkPlaneWriter& operator=(WorkPlaneWriter&& other) noexcept;

            // It is not copyable, as that would imply multiple writers for the same scope.
            WorkPlaneWriter(const WorkPlaneWriter&) = delete;
            WorkPlaneWriter& operator=(const WorkPlaneWriter&) = delete;

        private:
            friend class JobWriter; // Allow JobWriter to construct this class.

            // Private constructor to ensure it can only be created by JobWriter.
            WorkPlaneWriter(JobWriter& parent_writer, const WorkPlane& work_plane_shell);

            void Finalize();

            JobWriter* m_parent_writer; // Pointer to the parent, does not own it.
            WorkPlane m_work_plane_shell_state;
            WorkPlaneLUT m_wp_lut;
            std::streampos m_wp_lut_offset_pos; // Position where the offset to the WorkPlaneLUT is stored.
            bool m_is_finalized = false;
        };

    }
} // namespace open_vector_format::writer