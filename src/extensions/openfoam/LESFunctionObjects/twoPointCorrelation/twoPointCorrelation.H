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

#ifndef twoPointCorrelation_H
#define twoPointCorrelation_H

#include "typeInfo.H"
#include "autoPtr.H"
#include "runTimeSelectionTables.H"

#include "OFstream.H"
#include "cloudSet.H"
#include "sampledSets.H"
#include "meshSearch.H"
#include "coordinateSystem.H"

#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
#include "functionObject.H"
#else
#include "OutputFilterFunctionObject.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#include <boost/concept_check.hpp>

namespace Foam
{

// Forward declaration of classes
class Time;
class polyMesh;
class mapPolyMesh;

/*---------------------------------------------------------------------------*\
                       Class functionObject Declaration
\*---------------------------------------------------------------------------*/


//- Class used for sampling volFields
template<class Type>
class volFieldSampler
: public List<Field<Type> >
{
    //- Name of this collection of values
    const word name_;

public:

    //- Construct interpolating field to the sampleSets
    volFieldSampler
    (
        const word& interpolationScheme,
        const GeometricField<Type, fvPatchField, volMesh>& field,
        const PtrList<sampledSet>& samplers
    )
        :
        List<Field<Type> >(samplers.size()),
        name_(field.name())
    {
        autoPtr<interpolation<Type> > interpolator
        (
            interpolation<Type>::New(interpolationScheme, field)
        );

        forAll(samplers, setI)
        {
            Field<Type>& values = this->operator[](setI);
            const sampledSet& samples = samplers[setI];

            values.setSize(samples.size());
            forAll(samples, sampleI)
            {
                const point& samplePt = samples[sampleI];
                label cellI = samples.cells()[sampleI];
                label faceI = samples.faces()[sampleI];

                if (cellI == -1 && faceI == -1)
                {
                    // Special condition for illegal sampling points
                    values[sampleI] = pTraits<Type>::max;
                }
                else
                {
                    values[sampleI] = interpolator().interpolate
                                      (
                                          samplePt,
                                          cellI,
                                          faceI
                                      );
                }
            }
        }
    }

    volFieldSampler
    (
        const List<Field<Type> >& values,
        const word& name
    )
        :
        List<Field<Type> >(values),
        name_(name)
    {}

    inline const word& name() const {
        return name_;
    }
};

#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
#define RTYPE bool
#else
#define RTYPE void
#endif

class twoPointCorrelation
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
: public functionObject
#endif
{

protected:
    word name_;
	
    const objectRegistry& obr_;

    //- on/off switch
    bool active_;
    
    //- Mesh search engine
    autoPtr<meshSearch> searchEngine_;
    
    autoPtr<OFstream> filePtr_;

    point p0_;
    vector directionSpan_;
    label np_;
    
    vector homogeneousTranslationUnit_;
    label nph_;
    
    autoPtr<coordinateSystem> csys_;
    
    PtrList<sampledSet> lines_;
    
    // Merging structures
    autoPtr<scalarField> x_;
    PtrList<coordSet> masterSampledSets_;
    labelListList indexSets_;

    autoPtr<tensorField> correlationCoeffs_;
    scalar totalTime_;

    //- If the forces file has not been created create it
    void makeFile();

    //- Output file header information
    virtual void writeFileHeader();
    
    void createInterpolators();
    
    void combineSampledSets
    (
	PtrList<coordSet>& masterSampledSets,
	labelListList& indexSets
    );
    
    void resetAveraging();


public:
    //- Runtime type information
    TypeName("twoPointCorrelation");

    // Constructors
    twoPointCorrelation
    (
      const word& name,
      const objectRegistry&,
      const dictionary&,
      const bool loadFromFiles = false
    );
    virtual ~twoPointCorrelation();

    // Member Functions
    virtual const word& name() const { return name_; }

    //- Read and set the function object if its data have changed
    virtual RTYPE read(const dictionary&);

    virtual void start();
    virtual RTYPE execute();
    virtual RTYPE end();

    //- Called when time was set at the end of the Time::operator++
    virtual void timeSet();

    virtual RTYPE write();

    //- Update for changes of mesh
    virtual void updateMesh(const mapPolyMesh& mpm);
    
    virtual void movePoints(    
#if ! (defined(OF16ext) || defined(OF21x))
    const polyMesh& mesh
#else
    const pointField& points
#endif
    );
    
    virtual void readUpdate(const polyMesh::readUpdateState state);
};

typedef 
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    twoPointCorrelation
#else
    OutputFilterFunctionObject<twoPointCorrelation>
#endif
    twoPointCorrelationFunctionObject;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#undef RTYPE
#endif

// ************************************************************************* //
