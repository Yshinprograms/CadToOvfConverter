#include "pch.h"
#include "CppUnitTest.h"

#include "OvfWriter.h"
#include "open_vector_format.pb.h"
#include "TestFixtures.h" // <-- Include our new helper

#include <fstream>
#include "google/protobuf/util/delimited_message_util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace open_vector_format::writer;
using namespace open_vector_format;

namespace CadToOvfConverterTests
{
    TEST_CLASS(OvfWriterTests)
    {
    public:

        TEST_METHOD(JobWriter_WriteSimpleSquare_CreatesNonEmptyFile) // <-- New name
        {
            // --- ARRANGE ---
            const std::string filepath = "test_square.ovf";
            Job job_shell;
            job_shell.mutable_job_meta_data()->set_job_name("TestSquareJob");
            WorkPlane wp_shell;
            wp_shell.set_z_pos_in_mm(0.05);

            // Get the geometry from our new fixture
            VectorBlock square_vb = TestFixtures::CreateSquareVectorBlock();

            // --- ACT ---
            {
                JobWriter writer(filepath, job_shell);
                WorkPlaneWriter wp_writer = writer.AppendWorkPlane(wp_shell);
                wp_writer.AppendVectorBlock(square_vb);
            }

            // --- ASSERT ---
            std::ifstream outfile(filepath);
            Assert::IsTrue(outfile.good(), L"Output file was not created.");

            outfile.seekg(0, std::ios::end);
            long long file_size = outfile.tellg();
            Assert::IsTrue(file_size > 0, L"Output file is empty.");
        }

        // ... includes ...

        TEST_METHOD(JobWriter_WriteAndReadSquare_VerifiesContent)
        {
            // --- ARRANGE --- (Unchanged)
            const std::string filepath = "test_square_final.ovf";
            Job original_job_shell;
            original_job_shell.mutable_job_meta_data()->set_job_name("TestFinalJob");
            WorkPlane original_wp_shell;
            original_wp_shell.set_z_pos_in_mm(0.05);
            VectorBlock original_vb = TestFixtures::CreateSquareVectorBlock();

            // --- ACT (WRITE) --- (Unchanged)
            {
                JobWriter writer(filepath, original_job_shell);
                WorkPlaneWriter wp_writer = writer.AppendWorkPlane(original_wp_shell);
                wp_writer.AppendVectorBlock(original_vb);
            }

            // --- ASSERT (READ BACK & VERIFY) ---
            // 1. First, we need the offset of the JobLUT. We still read this directly.
            uint64_t job_lut_offset;
            {
                std::ifstream fs(filepath, std::ios::binary);
                Assert::IsTrue(fs.good(), L"Failed to open file for initial offset read.");
                fs.seekg(4); // Skip magic bytes
                Assert::IsTrue(TestFixtures::ReadLittleEndian(fs, job_lut_offset), L"Failed to read JobLUT offset.");
            }

            // 2. Now use our robust helper to read the JobLUT
            JobLUT read_job_lut;
            Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, job_lut_offset, read_job_lut), L"Failed to parse JobLUT.");
            Assert::AreEqual(1, read_job_lut.workplanepositions_size(), L"JobLUT should have one workplane position.");
            uint64_t wp_data_block_offset = read_job_lut.workplanepositions(0);

            // 3. Read the WP LUT offset (which is at the start of the WP data block)
            uint64_t wp_lut_offset;
            {
                std::ifstream fs(filepath, std::ios::binary);
                fs.seekg(wp_data_block_offset);
                Assert::IsTrue(TestFixtures::ReadLittleEndian(fs, wp_lut_offset), L"Failed to read WorkPlaneLUT offset.");
            }

            // 4. Use our helper to read the WorkPlaneLUT
            WorkPlaneLUT read_wp_lut;
            Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, wp_lut_offset, read_wp_lut), L"Failed to parse WorkPlaneLUT.");
            Assert::AreEqual(1, read_wp_lut.vectorblockspositions_size(), L"WorkPlaneLUT should have one VB position.");
            uint64_t vb_offset = read_wp_lut.vectorblockspositions(0);

            // 5. Use our helper to read the VectorBlock
            VectorBlock read_vb;
            Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, vb_offset, read_vb), L"Failed to parse VectorBlock.");

            // 6. Final comparison
            Assert::AreEqual(original_vb.SerializeAsString(), read_vb.SerializeAsString(), L"Read-back VectorBlock does not match original.");
        }
    };
}