#include "pch.h"
#include "CppUnitTest.h"

#include "OvfWriter.h"
#include "open_vector_format.pb.h"
#include "ovf_lut.pb.h"
#include "TestFixtures.h"

#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace open_vector_format::writer;
using namespace open_vector_format;

namespace CadToOvfConverterTests
{
	TEST_CLASS(OvfWriterTests)
	{
	public:

		TEST_METHOD(JobWriter_WriteSimpleSquare_CreatesNonEmptyFile)
		{
			// ARRANGE
			const std::string filepath = "test_square_smoke.ovf";
			Job job_shell;
			job_shell.mutable_job_meta_data()->set_job_name("TestSquareJob");
			WorkPlane wp_shell;
			wp_shell.set_z_pos_in_mm(0.05);
			VectorBlock square_vb = TestFixtures::CreateSquareVectorBlock();

			// ACT
			{
				JobWriter writer(filepath, job_shell);
				WorkPlaneWriter wp_writer = writer.AppendWorkPlane(wp_shell);
				wp_writer.AppendVectorBlock(square_vb);
			}

			// ASSERT
			std::ifstream outfile(filepath);
			Assert::IsTrue(outfile.good(), L"Output file was not created.");
			outfile.seekg(0, std::ios::end);
			Assert::IsTrue(outfile.tellg() > 0, L"Output file is empty.");
		}

		TEST_METHOD(JobWriter_WriteAndReadSquare_VerifiesContent)
		{
			// ARRANGE
			const std::string filepath = "test_square_verify.ovf";
			Job original_job_shell;
			original_job_shell.mutable_job_meta_data()->set_job_name("TestVerifyJob");
			WorkPlane original_wp_shell;
			original_wp_shell.set_z_pos_in_mm(0.05);
			VectorBlock original_vb = TestFixtures::CreateSquareVectorBlock();

			// ACT
			{
				JobWriter writer(filepath, original_job_shell);
				WorkPlaneWriter wp_writer = writer.AppendWorkPlane(original_wp_shell);
				wp_writer.AppendVectorBlock(original_vb);
			}

			// ASSERT
			uint64_t job_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				Assert::IsTrue(fs.good(), L"Failed to open file for initial offset read.");
				fs.seekg(4); // Skip magic bytes
				Assert::IsTrue(TestFixtures::ReadLittleEndian(fs, job_lut_offset), L"Failed to read JobLUT offset.");
			}

			JobLUT read_job_lut;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, job_lut_offset, read_job_lut), L"Failed to parse JobLUT.");
			Assert::AreEqual(1, read_job_lut.workplanepositions_size(), L"JobLUT should have one workplane position.");

			uint64_t wp_data_block_offset = read_job_lut.workplanepositions(0);
			uint64_t wp_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(wp_data_block_offset);
				Assert::IsTrue(TestFixtures::ReadLittleEndian(fs, wp_lut_offset), L"Failed to read WorkPlaneLUT offset.");
			}

			WorkPlaneLUT read_wp_lut;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, wp_lut_offset, read_wp_lut), L"Failed to parse WorkPlaneLUT.");
			Assert::AreEqual(1, read_wp_lut.vectorblockspositions_size(), L"WorkPlaneLUT should have one VB position.");

			VectorBlock read_vb;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, read_wp_lut.vectorblockspositions(0), read_vb), L"Failed to parse VectorBlock.");
			Assert::AreEqual(original_vb.SerializeAsString(), read_vb.SerializeAsString(), L"Read-back VectorBlock does not match original.");
		}

		TEST_METHOD(JobWriter_WorkPlaneWithNoVectorBlocks_SucceedsAndVerifies)
		{
			// ARRANGE
			const std::string filepath = "test_empty_wp.ovf";
			Job job_shell;
			job_shell.mutable_job_meta_data()->set_job_name("EmptyWorkplaneJob");
			WorkPlane wp_shell;
			wp_shell.set_z_pos_in_mm(0.10);

			// ACT
			{
				JobWriter writer(filepath, job_shell);
				WorkPlaneWriter wp_writer = writer.AppendWorkPlane(wp_shell);
				// No vector blocks appended
			}

			// ASSERT
			uint64_t job_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				Assert::IsTrue(fs.good(), L"Failed to open file for initial offset read.");
				fs.seekg(4);
				Assert::IsTrue(TestFixtures::ReadLittleEndian(fs, job_lut_offset), L"Failed to read JobLUT offset.");
			}

			JobLUT read_job_lut;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, job_lut_offset, read_job_lut), L"Failed to parse JobLUT.");
			Assert::AreEqual(1, read_job_lut.workplanepositions_size(), L"JobLUT should have one workplane position.");

			uint64_t wp_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(read_job_lut.workplanepositions(0));
				Assert::IsTrue(TestFixtures::ReadLittleEndian(fs, wp_lut_offset), L"Failed to read WorkPlaneLUT offset.");
			}

			WorkPlaneLUT read_wp_lut;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, wp_lut_offset, read_wp_lut), L"Failed to parse WorkPlaneLUT.");
			Assert::AreEqual(0, read_wp_lut.vectorblockspositions_size(), L"WorkPlaneLUT should have zero vector block positions.");
		}

		TEST_METHOD(WorkPlaneWriter_MultipleVectorBlocks_SucceedsAndVerifies)
		{
			// ARRANGE
			const std::string filepath = "test_multi_vb.ovf";
			Job job_shell;
			job_shell.mutable_job_meta_data()->set_job_name("MultiVBJob");
			WorkPlane wp_shell;
			wp_shell.set_z_pos_in_mm(0.15);
			VectorBlock original_square_vb = TestFixtures::CreateSquareVectorBlock();
			VectorBlock original_triangle_vb = TestFixtures::CreateTriangleVectorBlock();

			// ACT
			{
				JobWriter writer(filepath, job_shell);
				WorkPlaneWriter wp_writer = writer.AppendWorkPlane(wp_shell);
				wp_writer.AppendVectorBlock(original_square_vb);
				wp_writer.AppendVectorBlock(original_triangle_vb);
			}

			// ASSERT
			uint64_t job_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(4);
				TestFixtures::ReadLittleEndian(fs, job_lut_offset);
			}

			JobLUT read_job_lut;
			TestFixtures::ReadDelimitedFromOffset(filepath, job_lut_offset, read_job_lut);

			uint64_t wp_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(read_job_lut.workplanepositions(0));
				TestFixtures::ReadLittleEndian(fs, wp_lut_offset);
			}

			WorkPlaneLUT read_wp_lut;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, wp_lut_offset, read_wp_lut), L"Failed to parse WorkPlaneLUT.");
			Assert::AreEqual(2, read_wp_lut.vectorblockspositions_size(), L"WorkPlaneLUT should have two vector block positions.");

			VectorBlock read_square_vb;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, read_wp_lut.vectorblockspositions(0), read_square_vb), L"Failed to parse first VB.");
			Assert::AreEqual(original_square_vb.SerializeAsString(), read_square_vb.SerializeAsString(), L"Read-back square VB does not match original.");

			VectorBlock read_triangle_vb;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, read_wp_lut.vectorblockspositions(1), read_triangle_vb), L"Failed to parse second VB.");
			Assert::AreEqual(original_triangle_vb.SerializeAsString(), read_triangle_vb.SerializeAsString(), L"Read-back triangle VB does not match original.");
		}

		TEST_METHOD(JobWriter_MultipleWorkPlanes_SucceedsAndVerifies)
		{
			// ARRANGE
			const std::string filepath = "test_multi_wp.ovf";
			Job job_shell;
			job_shell.mutable_job_meta_data()->set_job_name("MultiWPJob");

			WorkPlane original_wp_1;
			original_wp_1.set_z_pos_in_mm(0.1);
			VectorBlock original_vb_1 = TestFixtures::CreateSquareVectorBlock();

			WorkPlane original_wp_2;
			original_wp_2.set_z_pos_in_mm(0.2);
			VectorBlock original_vb_2 = TestFixtures::CreateTriangleVectorBlock();

			// ACT
			{
				JobWriter writer(filepath, job_shell);
				{
					WorkPlaneWriter wp_writer_1 = writer.AppendWorkPlane(original_wp_1);
					wp_writer_1.AppendVectorBlock(original_vb_1);
				}
				{
					WorkPlaneWriter wp_writer_2 = writer.AppendWorkPlane(original_wp_2);
					wp_writer_2.AppendVectorBlock(original_vb_2);
				}
			}

			// ASSERT
			uint64_t job_lut_offset;
			{
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(4);
				TestFixtures::ReadLittleEndian(fs, job_lut_offset);
			}

			JobLUT read_job_lut;
			Assert::IsTrue(TestFixtures::ReadDelimitedFromOffset(filepath, job_lut_offset, read_job_lut), L"Failed to parse JobLUT.");
			Assert::AreEqual(2, read_job_lut.workplanepositions_size(), L"JobLUT should have two workplane positions.");

			// Verify Workplane 1
			{
				uint64_t wp_lut_offset_1;
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(read_job_lut.workplanepositions(0));
				TestFixtures::ReadLittleEndian(fs, wp_lut_offset_1);

				WorkPlaneLUT read_wp_lut_1;
				TestFixtures::ReadDelimitedFromOffset(filepath, wp_lut_offset_1, read_wp_lut_1);
				Assert::AreEqual(1, read_wp_lut_1.vectorblockspositions_size(), L"WP1 LUT should have one VB.");

				VectorBlock read_vb_1;
				TestFixtures::ReadDelimitedFromOffset(filepath, read_wp_lut_1.vectorblockspositions(0), read_vb_1);
				Assert::AreEqual(original_vb_1.SerializeAsString(), read_vb_1.SerializeAsString(), L"WP1 VectorBlock does not match.");
			}

			// Verify Workplane 2
			{
				uint64_t wp_lut_offset_2;
				std::ifstream fs(filepath, std::ios::binary);
				fs.seekg(read_job_lut.workplanepositions(1));
				TestFixtures::ReadLittleEndian(fs, wp_lut_offset_2);

				WorkPlaneLUT read_wp_lut_2;
				TestFixtures::ReadDelimitedFromOffset(filepath, wp_lut_offset_2, read_wp_lut_2);
				Assert::AreEqual(1, read_wp_lut_2.vectorblockspositions_size(), L"WP2 LUT should have one VB.");

				VectorBlock read_vb_2;
				TestFixtures::ReadDelimitedFromOffset(filepath, read_wp_lut_2.vectorblockspositions(0), read_vb_2);
				Assert::AreEqual(original_vb_2.SerializeAsString(), read_vb_2.SerializeAsString(), L"WP2 VectorBlock does not match.");
			}
		}
	};
}