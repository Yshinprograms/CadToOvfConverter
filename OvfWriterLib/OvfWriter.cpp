// OvfWriterLib/OvfWriter.cpp

#include "OvfWriter.h"
#include "OvfUtil.h"
#include "google/protobuf/util/delimited_message_util.h"

namespace open_vector_format {
    namespace writer {

        // --- JobWriter Implementation ---

        JobWriter::JobWriter(const std::string& path, const Job& job_shell) {
            m_stream.open(path, std::ios::binary);
            if (!m_stream.is_open() || !m_stream.good()) {
                throw std::runtime_error("Failed to open file for writing: " + path);
            }

            // 1. Write file identifier (magic bytes).
            const char magic[] = { 0x4f, 0x56, 0x46, 0x21 }; // OVF!
            m_stream.write(magic, sizeof(magic));

            // 2. Reserve 8 bytes for the JobLUT offset, which we'll write at the end.
            m_job_lut_offset_pos = m_stream.tellp();
            util::WriteLittleEndian(0, m_stream); // Placeholder

            // 3. Initialize internal state from the provided shell.
            m_job_shell_state = util::CreateJobShell(job_shell);
        }

        JobWriter::~JobWriter() {
            // The destructor ensures finalization, even if an exception occurs.
            if (!m_is_finalized) {
                Finalize();
            }
        }

        void JobWriter::Finalize() {
            // 1. Write the job shell itself.
            m_job_lut.set_jobshellposition(m_stream.tellp());
            google::protobuf::util::SerializeDelimitedToOstream(m_job_shell_state, &m_stream);

            // 2. Write the JobLUT.
            uint64_t job_lut_offset = m_stream.tellp();
            google::protobuf::util::SerializeDelimitedToOstream(m_job_lut, &m_stream);

            // 3. Go back and write the actual offset of the JobLUT.
            m_stream.seekp(m_job_lut_offset_pos);
            util::WriteLittleEndian(job_lut_offset, m_stream);

            m_stream.close();
            m_is_finalized = true;
        }

        WorkPlaneWriter JobWriter::AppendWorkPlane(const WorkPlane& work_plane_shell) {
            if (m_is_finalized) {
                throw std::runtime_error("Cannot append WorkPlane to a finalized JobWriter.");
            }
            // The private constructor of WorkPlaneWriter creates the object correctly.
            return WorkPlaneWriter(*this, work_plane_shell);
        }


        // --- WorkPlaneWriter Implementation ---

        WorkPlaneWriter::WorkPlaneWriter(JobWriter& parent_writer, const WorkPlane& work_plane_shell)
            : m_parent_writer(&parent_writer)
        {
            auto& stream = m_parent_writer->m_stream;

            // 1. Record the start position of this WorkPlane block in the JobLUT.
            m_parent_writer->m_job_lut.add_workplanepositions(stream.tellp());

            // 2. Reserve 8 bytes for the WorkPlaneLUT offset.
            m_wp_lut_offset_pos = stream.tellp();
            util::WriteLittleEndian(0, stream); // Placeholder

            // 3. Initialize internal state.
            m_work_plane_shell_state = util::CreateWorkPlaneShell(work_plane_shell);
            // The work plane number should be sequential, managed by the JobWriter.
            m_work_plane_shell_state.set_work_plane_number(m_parent_writer->m_job_shell_state.num_work_planes());
        }

        WorkPlaneWriter::~WorkPlaneWriter() {
            // Destructor ensures finalization. If the writer is moved, this logic won't run
            // on the moved-from object because m_parent_writer will be null.
            if (!m_is_finalized && m_parent_writer) {
                Finalize();
            }
        }

        void WorkPlaneWriter::AppendVectorBlock(const VectorBlock& vb) {
            if (m_is_finalized) {
                throw std::runtime_error("Cannot append VectorBlock to a finalized WorkPlaneWriter.");
            }
            auto& stream = m_parent_writer->m_stream;

            m_wp_lut.add_vectorblockspositions(stream.tellp());
            google::protobuf::util::SerializeDelimitedToOstream(vb, &stream);

            m_work_plane_shell_state.set_num_blocks(m_work_plane_shell_state.num_blocks() + 1);
        }

        void WorkPlaneWriter::Finalize() {
            auto& stream = m_parent_writer->m_stream;

            // 1. Write the WorkPlane shell.
            m_wp_lut.set_workplaneshellposition(stream.tellp());
            google::protobuf::util::SerializeDelimitedToOstream(m_work_plane_shell_state, &stream);

            // 2. Write the WorkPlaneLUT.
            uint64_t wp_lut_offset = stream.tellp();
            google::protobuf::util::SerializeDelimitedToOstream(m_wp_lut, &stream);

            // 3. Go back and write the actual offset of the WorkPlaneLUT.
            stream.seekp(m_wp_lut_offset_pos);
            util::WriteLittleEndian(wp_lut_offset, stream);

            // 4. Seek back to the end of the file to be ready for the next WorkPlane.
            stream.seekp(0, std::ios::end);

            // 5. Update the parent's work plane count.
            m_parent_writer->m_job_shell_state.set_num_work_planes(
                m_parent_writer->m_job_shell_state.num_work_planes() + 1
            );

            m_is_finalized = true;
        }


        // --- Move Semantics for WorkPlaneWriter ---

        WorkPlaneWriter::WorkPlaneWriter(WorkPlaneWriter&& other) noexcept
            : m_parent_writer(other.m_parent_writer),
            m_work_plane_shell_state(std::move(other.m_work_plane_shell_state)),
            m_wp_lut(std::move(other.m_wp_lut)),
            m_wp_lut_offset_pos(other.m_wp_lut_offset_pos),
            m_is_finalized(other.m_is_finalized)
        {
            // The moved-from object is now inert and its destructor will do nothing.
            other.m_parent_writer = nullptr;
            other.m_is_finalized = true;
        }

        WorkPlaneWriter& WorkPlaneWriter::operator=(WorkPlaneWriter&& other) noexcept {
            if (this != &other) {
                // Finalize the current object before overwriting it, if necessary.
                if (!m_is_finalized && m_parent_writer) {
                    Finalize();
                }

                m_parent_writer = other.m_parent_writer;
                m_work_plane_shell_state = std::move(other.m_work_plane_shell_state);
                m_wp_lut = std::move(other.m_wp_lut);
                m_wp_lut_offset_pos = other.m_wp_lut_offset_pos;
                m_is_finalized = other.m_is_finalized;

                other.m_parent_writer = nullptr;
                other.m_is_finalized = true;
            }
            return *this;
        }
    }
} // namespace open_vector_format::writer