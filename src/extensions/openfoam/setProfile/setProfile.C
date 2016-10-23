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
 *
 */

#include "fvCFD.H"
#include "OFstream.H"
#include "transformField.H"
#include "transformGeometricField.H"
#include "fixedGradientFvPatchFields.H"

#include "Tuple2.H"
#include "interpolationTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

template<class T>
void setProfileLinear
(
    GeometricField<T, fvPatchField, volMesh>& field,
    point p0, 
    vector ey,
    vector ex,
    Istream& ascii_data
)
{
    List<Tuple2<scalar, T> > data(ascii_data);
    interpolationTable<T> ip(data, interpolationTable<T>::CLAMP, "no_filename");
    
    vector ez=ex^ey;
    ez/=mag(ez);
    
    tensor TT(ex, ey, ez);
    
    const fvMesh& mesh=field.mesh();
    forAll(field, i)
    {
        point p=mesh.C()[i];
        scalar x = ((p-p0)&ey);
        
        T y = ip(x);
        
        field[i]=transform(TT, y);
    }
}

int main(int argc, char *argv[])
{
    argList::validArgs.append("fieldname");    
    argList::validArgs.append("p0");
    argList::validArgs.append("profileDir_y");
    argList::validArgs.append("axialDir_x");
    argList::validArgs.append("data");
    argList::validOptions.insert("cylindrical", "");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    word fieldname(args.additionalArgs()[0]);
    point p0(IStringStream(args.additionalArgs()[1])());
    vector ey(IStringStream(args.additionalArgs()[2])());
    ey/=mag(ey);
    vector ex(IStringStream(args.additionalArgs()[3])());
    ex/=mag(ex);
    
    IOobject header
	(
	    fieldname,
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::AUTO_WRITE
	);

    if (!header.headerOk())
    {
        FatalErrorIn("main")
         << "Could not find field "<<fieldname<<"!"
         <<abort(FatalError);
    }
    
    if (header.headerClassName() == volScalarField::typeName)
    {
        volScalarField field(header, mesh);
        setProfileLinear<scalar>(field, p0, ey, ex, IStringStream(args.additionalArgs()[4])());    
        field.write();
    }
    else if (header.headerClassName() == volVectorField::typeName)
    {
        volVectorField field(header, mesh);
        setProfileLinear<vector>(field, p0, ey, ex, IStringStream(args.additionalArgs()[4])());    
        field.write();
    }
    else
    {
        FatalErrorIn("main")
         << "Unsupported field type "<<header.headerClassName()<<"!"
         <<abort(FatalError);
    }
    
    Info<<"End."<<endl;

    return 0;
}

// ************************************************************************* //