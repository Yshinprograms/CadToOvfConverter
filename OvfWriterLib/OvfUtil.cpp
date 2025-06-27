// OvfWriterLib/OvfUtil.cpp

#include "OvfUtil.h"
#include "google/protobuf/util/delimited_message_util.h"

namespace open_vector_format {
    namespace writer {
        namespace util {

            // A simple check for the host system's endianness.
            bool IsSystemBigEndian() {
                uint16_t num = 0x0100;
                return (*(uint8_t*)&num == 0x01);
            }

            void WriteLittleEndian(uint64_t value, std::ofstream& os) {
                if (IsSystemBigEndian()) {
                    uint8_t buf[sizeof(uint64_t)];
                    for (size_t i = 0; i < sizeof(uint64_t); ++i) {
                        buf[i] = (value >> (i * 8)) & 0xFF;
                    }
                    os.write(reinterpret_cast<const char*>(buf), sizeof(uint64_t));
                }
                else {
                    os.write(reinterpret_cast<const char*>(&value), sizeof(uint64_t));
                }
            }

            Job CreateJobShell(const Job& full_job) {
                Job shell;
                if (full_job.has_job_meta_data()) {
                    shell.mutable_job_meta_data()->CopyFrom(full_job.job_meta_data());
                }
                if (full_job.has_job_parameters()) {
                    shell.mutable_job_parameters()->CopyFrom(full_job.job_parameters());
                }
                shell.mutable_marking_params_map()->insert(
                    full_job.marking_params_map().begin(),
                    full_job.marking_params_map().end()
                );
                shell.mutable_parts_map()->insert(
                    full_job.parts_map().begin(),
                    full_job.parts_map().end()
                );
                // Note: We intentionally DO NOT copy work_planes.
                // We also initialize num_work_planes to 0, as we will count them during writing.
                shell.set_num_work_planes(0);
                return shell;
            }

            WorkPlane CreateWorkPlaneShell(const WorkPlane& full_wp) {
                WorkPlane shell;
                shell.set_x_pos_in_mm(full_wp.x_pos_in_mm());
                shell.set_y_pos_in_mm(full_wp.y_pos_in_mm());
                shell.set_z_pos_in_mm(full_wp.z_pos_in_mm());
                shell.set_x_rot_in_deg(full_wp.x_rot_in_deg());
                shell.set_y_rot_in_deg(full_wp.y_rot_in_deg());
                shell.set_z_rot_in_deg(full_wp.z_rot_in_deg());
                shell.set_repeats(full_wp.repeats());
                shell.set_work_plane_number(full_wp.work_plane_number());
                shell.set_machine_type(full_wp.machine_type());
                shell.mutable_additional_axis_positions()->CopyFrom(full_wp.additional_axis_positions());
                if (full_wp.has_meta_data()) {
                    shell.mutable_meta_data()->CopyFrom(full_wp.meta_data());
                }
                // Note: We intentionally DO NOT copy vector_blocks.
                // We also initialize num_blocks to 0, as we will count them.
                shell.set_num_blocks(0);
                return shell;
            }
        }
    } // namespace util
} // namespace open_vector_format::writer// OvfWriterLib/OvfUtil.cpp
