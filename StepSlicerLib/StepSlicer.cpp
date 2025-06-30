#include "StepSlicer.h"
#include "GeometryContract.h"

// --- OCCT Includes ---
#include <STEPControl_Reader.hxx>
#include <gp_Pln.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <GeomAdaptor_Curve.hxx> // ADDED: The crucial adaptor header
#include <GCPnts_UniformDeflection.hxx>
#include <gp_Pnt.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

namespace geometry {

    StepSlicer::StepSlicer(const std::string& step_file_path)
        : m_file_path(step_file_path) {
    }

    std::vector<geometry_contract::SlicedLayer> StepSlicer::Slice(double layer_height) {

        std::vector<geometry_contract::SlicedLayer> all_layers;

        STEPControl_Reader reader;
        if (reader.ReadFile(m_file_path.c_str()) != IFSelect_RetDone) {
            return all_layers;
        }
        reader.TransferRoots();
        TopoDS_Shape model = reader.OneShape();

        if (model.IsNull()) {
            return all_layers;
        }

        Bnd_Box bounding_box;
        BRepBndLib::Add(model, bounding_box);
        Standard_Real z_min, z_max, x_min, y_min, x_max, y_max;
        bounding_box.Get(x_min, y_min, z_min, x_max, y_max, z_max);

        // A small epsilon to ensure we slice the very top layer
        for (double z = z_min; z <= z_max + 1e-9; z += layer_height) {

            gp_Pln slicing_plane(gp_Pnt(0, 0, z), gp_Dir(0, 0, 1));
            BRepAlgoAPI_Section section(model, slicing_plane);
            section.Build();
            TopoDS_Shape result_section = section.Shape();

            if (result_section.IsNull()) {
                continue;
            }

            geometry_contract::SlicedLayer current_layer;
            current_layer.ZHeight = z;

            TopExp_Explorer explorer(result_section, TopAbs_EDGE);
            while (explorer.More()) {
                TopoDS_Edge edge = TopoDS::Edge(explorer.Current());

                Standard_Real first, last;
                Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

                if (!curve.IsNull()) {
                    // CORRECTED: Create the adaptor for the curve
                    GeomAdaptor_Curve adaptor(curve);

                    GCPnts_UniformDeflection discretizer;
                    // Pass the ADAPTOR to the Initialize method
                    discretizer.Initialize(adaptor, 0.1, first, last); // Also pass curve bounds

                    if (discretizer.IsDone()) {
                        geometry_contract::Contour current_contour;
                        for (int i = 1; i <= discretizer.NbPoints(); ++i) {
                            gp_Pnt point = discretizer.Value(i);
                            current_contour.points.push_back({ point.X(), point.Y() });
                        }
                        current_layer.contours.push_back(current_contour);
                    }
                }
                explorer.Next();
            }

            if (!current_layer.contours.empty()) {
                all_layers.push_back(current_layer);
            }
        }

        return all_layers;
    }
}