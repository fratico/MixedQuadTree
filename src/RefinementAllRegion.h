/*
 <Mix-mesher: region type. This program generates a mixed-elements 2D mesh>

 Copyright (C) <2013,2018>  <Claudio Lobos> All rights reserved.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/lgpl.txt>
 */
/**
* @file RefinementAllRegion.h
* @author Claudio Lobos, Fabrice Jaillet
* @version 0.1
* @brief
**/

#ifndef RefinementAllRegion_h
#define RefinementAllRegion_h 1

#include <vector>
#include <set>
#include <iostream>
#include "RefinementRegion.h"
#include "Polyline.h"
#include "Quadrant.h"

using Clobscode::Point3D;
using std::vector;
using std::cout;
using std::set;
using Clobscode::RefinementRegion;
using Clobscode::Polyline;
using Clobscode::Quadrant;

namespace Clobscode
{
	class RefinementAllRegion : public RefinementRegion
	{
	public:
		
		// Construction / destruction
		RefinementAllRegion(const unsigned short &level);
		
		virtual ~RefinementAllRegion();
        
        virtual GeometricTransform rotateWithinYou(Polyline &input) override;
        
        virtual void rotate(GeometricTransform &gt) override;
		
        virtual bool intersectsQuadrant(const vector<MeshPoint> &points, Quadrant &q) const override;

	};
}

#endif
