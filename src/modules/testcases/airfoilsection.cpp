/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "base/factory.h"
#include "airfoilsection.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/snappyhexmesh.h"

#include "boost/foreach.hpp"
#include "boost/assign.hpp"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
  
defineType(AirfoilSection);
addToFactoryTable(Analysis, AirfoilSection, NoParameters);


AirfoilSection::AirfoilSection(const NoParameters&)
: OpenFOAMAnalysis("Airfoil 2D", "Steady RANS simulation of a 2-D flow over an airfoil section"),
  in_("in"), 
  out_("out"), 
  up_("up"), 
  down_("down"), 
  fb_("frontAndBack"),
  foil_("foil")
{

}

void AirfoilSection::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", c);
  PSDBL(p, "operation", vinf);
  PSINT(p, "fluid", turbulenceModel);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "fluid", rho);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new simpleFoamNumerics(cm, simpleFoamNumerics::Parameters()
    .set_purgeWrite(2)
  )); 
  
  cm.insert(new forces(cm, forces::Parameters()
    .set_name("foilForces")
    .set_patches( list_of(foil_+".*") )
    .set_rhoInf(rho)
    .set_CofR(vec3(0,0,0))
    ));  

  cm.insert(new VelocityInletBC(cm, in_, boundaryDict, VelocityInletBC::Parameters().velocity(vec3(vinf,0,0)) ));
  cm.insert(new PressureOutletBC(cm, out_, boundaryDict, PressureOutletBC::Parameters().pressure(0.0) ));
   
  cm.insert(new SimpleBC(cm, up_, boundaryDict, "symmetryPlane" ));
  cm.insert(new SimpleBC(cm, down_, boundaryDict, "symmetryPlane" ));
  cm.insert(new SimpleBC(cm, fb_, boundaryDict, "empty" ));

  //   cm.insert(new cuttingPlane(cm, cuttingPlane::Parameters()
//     .set_name("plane")
//     .set_basePoint(vec3(0,1e-6,1e-6))
//     .set_normal(vec3(0,0,1))
//     .set_fields(list_of<string>("p")("U")("UMean")("UPrime2Mean"))
//   ));
  
//   cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
//     .set_name("zzzaveraging") // shall be last FO in list
//     .set_fields(list_of<std::string>("p")("U"))
//     .set_timeStart(inittime*T_)
//   ));
  
//   cm.insert(new RadialTPCArray(cm, typename RadialTPCArray::Parameters()
//     .set_name_prefix("tpc_interior")
//     .set_R(0.5*D)
//     .set_x(0.5*L)
//     .set_axSpan(0.5*L)
//     .set_tanSpan(M_PI)
//     .set_timeStart( (inittime+meantime)*T_ )
//   ));
//   
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu) ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());
}

void AirfoilSection::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  PSPATH(p, "geometry", foilfile);
  PSINT(p, "mesh", nlayer);
  PSINT(p, "mesh", lmfoil);
  PSINT(p, "mesh", lxfoil);
  PSINT(p, "mesh", nc);
  PSDBL(p, "geometry", c);
  PSDBL(p, "geometry", alpha);
  PSDBL(p, "geometry", LinByc);
  PSDBL(p, "geometry", LoutByc);
  PSDBL(p, "geometry", HByc);
  
  arma::mat contour;
  contour.load(foilfile.string(), arma::auto_detect);
  cout<<contour<<endl;
  
  path dir = executionPath();
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  double delta=c/double(nc);
  
  std::map<int, Point> pts;
  double z0=0.5;
  pts = boost::assign::map_list_of   
      (0, 	vec3(-(LinByc+0.5)*c, -HByc*c, z0))
      (1, 	vec3((LoutByc+0.5)*c, -HByc*c, z0))
      (2, 	vec3((LoutByc+0.5)*c, HByc*c, z0))
      (3, 	vec3(-(LinByc+0.5)*c, HByc*c, z0))
  ;
  
  arma::mat PiM=vec3(-(LinByc+0.4)*c, 0.01*c, z0+0.0001*c);
  
  int nx=(pts[1][0]-pts[0][0])/delta;
  int ny=(pts[2][1]-pts[1][1])/delta;
  
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  Patch& up= 	bmd->addPatch(up_, new Patch());
  Patch& down= 	bmd->addPatch(down_, new Patch());
  Patch& dummy= bmd->addPatch("dummy", new Patch());
  Patch& fb= 	bmd->addPatch(fb_, new Patch());
  
  arma::mat vH=vec3(0,0,1.);
  
#define PTS(a,b,c,d) \
  P_8(pts[a], pts[b], pts[c], pts[d], \
      pts[a]+vH, pts[b]+vH, pts[c]+vH, pts[d]+vH)
      
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,2,3),
	nx, ny, 1,
	list_of<double>(1.)(1.)(1.)
      )
    );
    in.addFace(bl.face("0473"));
    out.addFace(bl.face("1265"));
    up.addFace(bl.face("2376"));
    down.addFace(bl.face("0154"));
    fb.addFace(bl.face("0321"));
    dummy.addFace(bl.face("4567"));
  }
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  
  path targ_path(dir/"constant"/"triSurface"/"foil.stl");
  create_directories(targ_path.parent_path());
  STLExtruder(contour, 0, z0+2.0, targ_path);
  
  cm.executeCommand(dir, "blockMesh");  

  boost::ptr_vector<snappyHexMeshFeats::Feature> shm_feats;
  
  shm_feats.push_back(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
    .set_name(foil_)
    .set_fileName(targ_path)
    .set_minLevel(lmfoil)
    .set_maxLevel(lxfoil)
    .set_nLayers(nlayer)
    .set_scale(vec3(c, c, 1))
    .set_rollPitchYaw(vec3(0,0,-alpha))
  ));
  
  shm_feats.push_back(new snappyHexMeshFeats::NearSurfaceRefinement( snappyHexMeshFeats::NearSurfaceRefinement::Parameters()
    .set_name(foil_)
    .set_mode("distance")
    .set_level(lmfoil)
    .set_distance(0.1*c)
  ));

    
  snappyHexMesh
  (
    cm, dir,
    OFDictData::vector3(PiM),
    shm_feats,
    snappyHexMeshOpts::Parameters()
      .set_tlayer(0.25)
      .set_relativeSizes(true)
      .set_nLayerIter(10)
  );
  
  extrude2DMesh
  (
    cm, dir,
    fb_,
    fb_,
    1.0
  );
}


insight::ParameterSet AirfoilSection::defaultParameters() const
{
  ParameterSet p(OpenFOAMAnalysis::defaultParameters());
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    
      ("geometry", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("c",		new DoubleParameter(1.0, "[m] chord length"))
	    ("alpha",		new DoubleParameter(0.0, "[deg] angle of attack"))
	    ("LinByc",		new DoubleParameter(1.0, "[-] Upstream extent of the channel"))
	    ("LoutByc",		new DoubleParameter(4.0, "[-] Downstream extent of the channel"))
	    ("HByc",		new DoubleParameter(2.0, "[-] Height of the channel"))
	    ("foilfile",	new PathParameter("foil.csv", "File with tabulated coordinates on the foil surface"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the numerical tunnel"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nc",	new IntParameter(15, "# cells along span"))
	    ("lmfoil",	new IntParameter(5, "min refinement level at foil surface"))
	    ("lxfoil",	new IntParameter(6, "min refinement level at foil surface"))
	    ("nlayer",	new IntParameter(10, "number of prism layers"))
// 	    ("ypluswall", new DoubleParameter(2, "yPlus at the wall grid layer"))
// 	    ("2d",	new BoolParameter(false, "Whether to create a two-dimensional case"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("vinf",		new DoubleParameter(30.8, "[m/s] inflow velocity"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("rho",	new DoubleParameter(1.0, "[kg/m^3] Density of the fluid"))
	    ("nu",	new DoubleParameter(1.5e-5, "[m^2/s] Viscosity of the fluid"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))
      
      //       ("evaluation", new SubsetParameter
// 	(
// 	  ParameterSet
// 	  (
// 	    boost::assign::list_of<ParameterSet::SingleEntry>
// 	    ("inittime",	new DoubleParameter(5, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
// 	    ("meantime",	new DoubleParameter(10, "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"))
// 	    ("mean2time",	new DoubleParameter(10, "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"))
// 	    ("eval2", 		new BoolParameter(true, "Whether to evaluate second order statistics"))
// 	    .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Options for statistical evaluation"
// 	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}

insight::ResultSetPtr AirfoilSection::evaluateResults(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  ResultSetPtr results(new ResultSet(p, "Result Report", "2D-Airfoil simulation"));
  
  return results;
}

arma::mat STLExtruder::tri::normal() const
{
  arma::mat d1=p[2]-p[0], d2=p[1]-p[0];
  arma::mat n=cross(d1, d2);
  n/=norm(n);
  return n;
}

void STLExtruder::addTriPair(const arma::mat& p0, const arma::mat& p1)
{
  tri t;
  t.p[0]=vec3(p0[0], p0[1], z0_);
  t.p[1]=vec3(p1[0], p1[1], z0_);
  t.p[2]=vec3(p0[0], p0[1], z1_);
  tris_.push_back(t);
  t.p[0]=vec3(p1[0], p1[1], z0_);
  t.p[1]=vec3(p1[0], p1[1], z1_);
  t.p[2]=vec3(p0[0], p0[1], z1_);
  tris_.push_back(t);
}

void STLExtruder::writeTris(const boost::filesystem::path& outputfilepath)
{
  std::ofstream f(outputfilepath.c_str());
  std::string name="patch0"; //outputfilepath.stem().string();
  f<<"solid "<<name<<std::endl;
  
  BOOST_FOREACH(const tri& t, tris_)
  {
    arma::mat n=t.normal();
    f<<"facet normal "<<n[0]<<" "<<n[1]<<" "<<n[2]<<endl;
    f<<" outer loop"<<endl;
    for (int j=0; j<3; j++)
      f<<"  vertex "<<t.p[j][0]<<" "<<t.p[j][1]<<" "<<t.p[j][2]<<endl;
    f<<" endloop"<<endl;
    f<<"endfacet"<<endl;
  }
  
  f<<"endsolid "<<name<<std::endl;
}


STLExtruder::STLExtruder
(
  const arma::mat xy_contour,
  double z0, double z1,
  const boost::filesystem::path& outputfilepath
)
: z0_(z0),
  z1_(z1)
{
  addTriPair(xy_contour.row(xy_contour.n_rows-1), xy_contour.row(0));
  for (int i=1; i<xy_contour.n_rows; i++)
  {    
    addTriPair(xy_contour.row(i-1), xy_contour.row(i));
  }
  
  writeTris(outputfilepath);
}

}