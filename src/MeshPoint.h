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
* @file MeshPoint.h
* @author Claudio Lobos, Fabrice Jaillet
* @version 0.1
* @brief
**/

#ifndef MeshPoint_h
#define MeshPoint_h 1

#include <iostream>
#include <math.h>
#include "Point3D.h"
#include <vector>
#include <list>

using Clobscode::Point3D;
using std::list;
using std::vector;

namespace Clobscode
{	
	class MeshPoint{
		
	public:
		
		MeshPoint();
		
        MeshPoint(const Point3D &p);
		
        virtual ~MeshPoint();
		
		virtual void setPoint(Point3D &p);
		
        //acces method:
        virtual Point3D &getPoint();
        virtual const Point3D &getPoint() const;

		virtual void addElement(unsigned int idx);
		
        virtual const list<unsigned int> &getElements() const;
		
        virtual void clearElements();
		
        virtual bool wasOutsideChecked() const;
		
		virtual void outsideChecked();
		
		virtual void setMaxDistance(double md);
		
        virtual double getMaxDistance() const;
		
		virtual void updateMaxDistanceByFactor(const double &per);
		
		virtual void setProjected();
		
        virtual bool wasProjected() const;
		
		//state methods
		virtual void setOutside();
		
		virtual void setInside();
		
		//returns true if node is inside any input mesh
        virtual bool isInside() const;
		
		//returns true if node is outside every input mesh
        virtual bool isOutside() const;
		
		virtual void setIOState(bool state);
		
        virtual bool getIOState() const;
        
        virtual void featureProjected();
        
        virtual bool isFeature() const;
		
	protected:
		
		Point3D point;
		//this flag avoids to re-check if node is inside, 
		//which is an expensive task
		bool outsidechecked, projected, feature;
		//inside is a flag to shrink elements to the surface.
		//it is a vector to know the state w.r.t. every input
		//geometry
		bool inside;//, projected;
		list<unsigned int> elements;
		
		double maxdistance;
		
	};
	
	inline void MeshPoint::outsideChecked(){
		outsidechecked = true;
	}
	
    inline bool MeshPoint::wasOutsideChecked() const{
		return outsidechecked;
	}
	
	inline void MeshPoint::setMaxDistance(double md){
		if (md<maxdistance) {
			return;
		}
		maxdistance = md;
	}
	
    inline double MeshPoint::getMaxDistance() const{
		return maxdistance;
	}
	
	inline void MeshPoint::updateMaxDistanceByFactor(const double &per){
		maxdistance *= per;
	}
	
	inline void MeshPoint::setOutside(){
		inside = false;
	}
	
	inline void MeshPoint::setInside(){
		inside = true;
	}
	
	inline void MeshPoint::setIOState(bool state){
		inside = state;
	}
	
    inline bool MeshPoint::getIOState() const {
		return inside;
	}
	
	//returns true if node is inside any input mesh
    inline bool MeshPoint::isInside() const {
		return inside;
	}
	
	//returns true if node is outside every input mesh
    inline bool MeshPoint::isOutside() const{
		return !inside;
	}
	
	inline void MeshPoint::setProjected(){
		projected = true;
        inside = false;
	}
	
    inline bool MeshPoint::wasProjected() const {
		return projected;
	}
    
    inline void MeshPoint::featureProjected() {
        feature = true;
    }
    
    inline bool MeshPoint::isFeature() const {
        return feature;
    }
	
    inline Point3D &MeshPoint::getPoint(){
        return point;
    }
    inline const Point3D &MeshPoint::getPoint() const {
        return point;
    }

	inline void MeshPoint::setPoint(Point3D &p){
		point = p;
	}
	
	inline void MeshPoint::addElement(unsigned int e){
		elements.push_back(e);
	}
	
	inline void MeshPoint::clearElements(){
		elements.clear();
	}
	
    inline const list<unsigned int> &MeshPoint::getElements() const {
		return elements;
	}
	
    std::ostream& operator<<(std::ostream& o, const MeshPoint &p);
	
}
#endif
