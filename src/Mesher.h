/*
 <Mix-mesher: region type. This program generates a mixed-elements mesh>
 
 Copyright (C) <2013,2017>  <Claudio Lobos>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/gpl.txt>
 */

#ifndef Mesher_h
#define Mesher_h 1

#include "Polyline.h"
#include "FEMesh.h"
#include "GridMesher.h"
#include "Quadrant.h"
#include "QuadEdge.h"
#include "Services.h"
#include "RefinementRegion.h"
#include "RefinementCubeRegion.h"

#include "Visitors/SplitVisitor.h"
#include "Visitors/IntersectionsVisitor.h"
#include "Visitors/OneIrregularVisitor.h"
#include "Visitors/PointMovedVisitor.h"
#include "Visitors/TransitionPatternVisitor.h"
#include "Visitors/SurfaceTemplatesVisitor.h"
#include "Visitors/RemoveSubElementsVisitor.h"

#include <list>
#include <vector>
#include <set>
#include <cstdlib>
#include <ctime>
#include <string.h>

using std::vector;
using std::list;
using std::set;
using Clobscode::QuadEdge;
using Clobscode::Polyline;
using Clobscode::RefinementRegion;

namespace Clobscode
{
	
	class Mesher{
		
	public:

		Mesher();
		
		virtual ~Mesher();
				
        virtual FEMesh generateMesh(Polyline &input, const unsigned short &rl,
                                    const string &name, list<RefinementRegion *> &all_reg);
		
        virtual FEMesh refineMesh(Polyline &input, const unsigned short &rl,
                                  const string &name, list<unsigned int> &roctli,
                                  list<RefinementRegion *> &all_reg,
                                  GeometricTransform &gt, const unsigned short &minrl,
                                  const unsigned short &omaxrl);

        
        virtual void setInitialState(vector<MeshPoint> &epts, vector<Quadrant> &eocts,
                                     set<QuadEdge> &eedgs);
        
	protected:
        
        virtual void splitQuadrants(const unsigned short &rl, Polyline &input,
                                  list<unsigned int> &roctli,
                                  list<RefinementRegion *> &all_reg, const string &name,
                                  const unsigned short &minrl, const unsigned short &omaxrl);
		
        virtual void generateOctreeMesh(const unsigned short &rl, Polyline &input,
                                        list<RefinementRegion *> &all_reg, const string &name);

        virtual bool isItIn(Polyline &mesh, list<unsigned int> &faces, vector<Point3D> &coords);

        virtual bool rotateGridMesh(Polyline &input,
									list<RefinementRegion *> &all_reg,
									GeometricTransform &gt);
		
		/*virtual void generateGridFromOctree(const unsigned short &rl, 
                                              Polyline &input,
                                              const string &name);*/
		
        virtual void generateGridMesh(Polyline &input);
		
		virtual void linkElementsToNodes();

        virtual void detectInsideNodes(Polyline &input);

		virtual void removeOnSurface();
		
        virtual void applySurfacePatterns(Polyline &input);

        virtual void shrinkToBoundary(Polyline &input);

		virtual unsigned int saveOutputMesh(FEMesh &mesh);
		
		virtual unsigned int saveOutputMesh(FEMesh &mesh,
									vector<MeshPoint> &points, 
									list<Quadrant> &elements);
        
        virtual void projectCloseToBoundaryNodes(Polyline &input);


		
	protected:
		
		vector<MeshPoint> points;
		vector<Quadrant> Quadrants;
		set<QuadEdge> QuadEdges;
		list<RefinementRegion *> regions;



	};
    
    inline void Mesher::setInitialState(vector<MeshPoint> &epts, vector<Quadrant> &eocts,
                                        set<QuadEdge> &eedgs) {
        Quadrants = eocts;
        points = epts;
        QuadEdges = eedgs;
    }
	
	
}
#endif
