#include "pch.h"
#include "CppUnitTest.h"

// This is the public interface for our slicer library
#include "StepSlicer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace geometry;

namespace CadToOvfConverterTests
{
    TEST_CLASS(StepSlicerTests)
    {
    public:

        TEST_METHOD(StepSlicer_Construction_DoesNotThrow)
        {
            // --- ARRANGE ---
            // The goal of this test is not to run logic, but to prove
            // that the project can link against OCCT.
            // We just need a dummy file path.
            const std::string dummy_path = "dummy.stp";

            // --- ACT & ASSERT ---
            // The "Act" is simply calling the constructor. If this line
            // compiles and links, the test has succeeded. If there are
            // configuration errors (bad include path, missing lib),
            // the build itself will fail.
            try
            {
                StepSlicer slicer(dummy_path);
                // If we get here without an exception, the basic object
                // was created successfully.
                Assert::IsTrue(true);
            }
            catch (...)
            {
                Assert::Fail(L"StepSlicer constructor threw an unexpected exception.");
            }
        }
    };
}