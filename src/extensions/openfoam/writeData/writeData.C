/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "writeData.H"
#include "dictionary.H"
#include "error.H"
#ifdef Fx40
#include "foamTime.H"
#else
#include "Time.H"
#endif
#include "OSspecific.H"
#include "PstreamReduceOps.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

#if not (defined(OFdev)||defined(OFplus)||defined(OFesi1806))
namespace Foam
{
defineTypeNameAndDebug(writeData, 0);
}
#endif

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::writeData::removeFile() const
{
    bool hasWrite = isFile(writeFile_);
    reduce(hasWrite, orOp<bool>());
    
    bool hasAbort = isFile(abortFile_);
    reduce(hasAbort, orOp<bool>());

    if (hasWrite && Pstream::master())
    {
        // cleanup ABORT file (on master only)
        rm(writeFile_);
    }
    
    if (hasAbort && Pstream::master())
    {
        // cleanup ABORT file (on master only)
        rm(abortFile_);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::writeData::writeData
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict,
    const bool loadFromFiles
)
:
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    functionObject(name),
#endif
    name_(name),
    obr_(obr),
    writeFile_("$FOAM_CASE/" + name),
    abortFile_("$FOAM_CASE/" + name+"Abort")
{
    writeFile_.expand();
    abortFile_.expand();
    read(dict);

    // remove any old files from previous runs
    removeFile();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::writeData::~writeData()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
bool
#else
void 
#endif
Foam::writeData::read(const dictionary& dict)
{
    if (dict.readIfPresent("fileName", writeFile_))
    {
        writeFile_.expand();
    }
    
    if (dict.readIfPresent("fileNameAbort", abortFile_))
    {
        abortFile_.expand();
    }
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    return true;
#endif
}


#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
bool
#else
void 
#endif
Foam::writeData::execute()
{
    bool write = isFile(writeFile_);
    reduce(write, orOp<bool>());
    bool abort = isFile(abortFile_);
    reduce(abort, orOp<bool>());

    if (write||abort)
    {    
        removeFile();
    }

    if (write)
    {
#if defined(OF16ext) || defined(OF21x)
        const_cast<Time&>(obr_.time()).writeNow();
#else
        const_cast<Time&>(obr_.time()).writeOnce();
#endif
        Info<< "USER REQUESTED DATA WRITE AT (timeIndex="
            << obr_.time().timeIndex()
            << ")"
            << endl;
    }
    
    if (abort)
    {
#if defined(OF16ext) //defined(OF21x)
//        const_cast<Time&>(obr_.time()).setStopAt(Time::saWriteNow);
#else
        const_cast<Time&>(obr_.time()).stopAt(Time::saWriteNow);
	Info<< "USER REQUESTED ABORT (timeIndex="
	    << obr_.time().timeIndex()
	    << "): stop+write data"
	    << endl;
#endif
    }
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    return true;
#endif    
}

void Foam::writeData::updateMesh(const mapPolyMesh& mpm)
{
}

#if defined(OF16ext) || defined(OF21x)
void Foam::writeData::movePoints(const pointField& pf)
#else
void Foam::writeData::movePoints(const polyMesh& mesh)
#endif
{
}



#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
bool
#else
void 
#endif
Foam::writeData::end()
{
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    return false;
#endif    //removeFile();
}


void Foam::writeData::timeSet()
{
    // Do nothing - only valid on execute
}


#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
bool
#else
void 
#endif
Foam::writeData::write()
{
    // Do nothing - only valid on execute
#if defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    return false;
#endif
}


// ************************************************************************* //
