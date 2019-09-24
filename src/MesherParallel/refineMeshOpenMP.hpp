#include "../Mesher.h"
#include <omp.h>

#include <iostream>
#include "ReductionOpenMP/SplitVisitorOpenMP.h"
#include "ReductionOpenMP/SplitVisitorReductionOpenMP.h"

#include <unordered_map>
#include <tr1/unordered_map>
#include <string>

using namespace std;

namespace Clobscode {

	void make_reduceV1(	vector<Point3D> *threads_new_pts, set<QuadEdge> *threads_new_edges, vector<Quadrant> *threads_new_quadrants,
					unsigned int number_of_threads, const unsigned int total_nb_points_before_reduce, unsigned int rl,
					vector<MeshPoint> &new_pts, set<QuadEdge> &new_edges, vector<Quadrant> &new_quadrants);

	void make_reduceV2(	vector<Point3D> *threads_new_pts, set<QuadEdge> *threads_new_edges, vector<Quadrant> *threads_new_quadrants,
					unsigned int number_of_threads, const unsigned int total_nb_points_before_reduce, unsigned int rl,
					vector<MeshPoint> &new_pts, set<QuadEdge> &new_edges, vector<Quadrant> &new_quadrants);

	string Mesher::refineMeshParallelOpenMP(int nbThread, list<Quadrant> Quadrants, vector<MeshPoint> points,
                                        set<QuadEdge> QuadEdges,
                                        const list<RefinementRegion *> &all_reg, const unsigned short &rl,
                                        Polyline &input) {
		string result;

            int MAX_THREAD = omp_get_max_threads();

            if (nbThread > MAX_THREAD || nbThread < 0) {
                std::cout << "Invalid number of threads or not supported by computer" << std::endl;
                return result;
            }
            omp_set_num_threads(nbThread);


            std::cout << "Start refine mesh parallel Open MP with " << nbThread << " threads." << std::endl;

            //Convert list into vector of temp Quadrants
            vector<Quadrant> tmp_Quadrants;
            tmp_Quadrants.assign(make_move_iterator(Quadrants.begin()), make_move_iterator(Quadrants.end()));


            //Shared variables
            vector<Quadrant> new_Quadrants;
            vector<Point3D> new_pts;
            //Also QuadEdges

            unsigned int nb_points;
            for (unsigned short i = 0; i < rl; i++) {
                auto start_refine_rl_time = chrono::high_resolution_clock::now();

                //atomic<long> time_split_visitor;
                //atomic<long> time_inside_block;
                //time_inside_block = 0;
                //time_split_visitor = 0;

                //the new_pts is a list that holds the coordinates of
                //new points inserted at this iteration. At the end of
                //this bucle, they are inserted in the point vector
                new_pts.clear();

                nb_points = points.size();

                //auto start_outside_block_time = chrono::high_resolution_clock::now();

                //split the Quadrants as needed
				  //auto start_block_time = chrono::high_resolution_clock::now();




			  #pragma omp parallel shared(nb_points, new_Quadrants, new_pts, QuadEdges, points)
			  	{
				//create visitors and give them variables
				SplitVisitorOpenMP sv(nb_points);
				sv.setPoints(points); // READ
				sv.setEdges(QuadEdges); // INSERT / REMOVE / READ
				sv.setNewPts(new_pts); // INSERT / READ

				//schedule(dynamic)
				#pragma omp for
			  	for (unsigned int j = 0; j < tmp_Quadrants.size(); ++j) {
			  		Quadrant &iter = tmp_Quadrants[j];

					//Only check, can not modify after treatment
					bool to_refine = false;
					list<RefinementRegion *>::const_iterator reg_iter;

					for (reg_iter = all_reg.begin(), reg_iter++; reg_iter != all_reg.end(); ++reg_iter) {

						unsigned short region_rl = (*reg_iter)->getRefinementLevel();
						if (region_rl < i) {
						  continue;
						}

						//If the Quadrant has a greater RL than the region needs, continue
						if (region_rl <= iter.getRefinementLevel()) {
							continue;
						}

						//Get the two extreme nodes of the Quadrant to test intersection with
						//this RefinementRegion. If not, conserve it as it is.
						//unsigned int n_idx1 = (*iter).getPoints()[0];
						//unsigned int n_idx2 = (*iter).getPoints()[2];


						// intersectQaudrant can modify the quadrant with
						// function Polyline::getNbFeatures in RefinementboundaryRegion
						// maybe no problem for parallelisation, as this information is
						// not used by other thread
						if ((*reg_iter)->intersectsQuadrant(points, iter)) {
							to_refine = true;
							//counterRefine.fetch_and_increment();
							break;
						}
					}


			      	//now if refinement is not needed, we add the Quadrant as it was.
			      	if (!to_refine) {
			      		#pragma omp critical(new_quad)
			        	new_Quadrants.push_back(iter);
			        	//End of for loop
			      } else {
			          //(paul) Idea : add a task here (only if to refined, check if faster..)

			          list<unsigned int> &inter_edges = iter.getIntersectedEdges();
			          unsigned short qrl = iter.getRefinementLevel();

			          vector<vector<Point3D> > clipping_coords;
			          sv.setClipping(clipping_coords);

			          vector<vector<unsigned int> > split_elements;
			          sv.setNewEles(split_elements);

			          //auto start_sv_time = chrono::high_resolution_clock::now();
			          iter.accept(&sv);
			          //auto end_sv_time = chrono::high_resolution_clock::now();
			          //time_split_visitor += std::chrono::duration_cast<chrono::milliseconds>(end_sv_time - start_sv_time).count();

			          if (inter_edges.empty()) {
			              for (unsigned int j = 0; j < split_elements.size(); j++) {
			                  Quadrant o(split_elements[j], qrl + 1);
			                  #pragma omp critical(new_quad)
			                  new_Quadrants.push_back(o);
			              }
			          } else {
			              for (unsigned int j = 0; j < split_elements.size(); j++) {
			                  Quadrant o(split_elements[j], qrl + 1);

			                  //the new points are inserted in bash at the end of this
			                  //iteration. For this reason, the coordinates must be passed
			                  //"manually" at this point (clipping_coords).

			                  IntersectionsVisitor iv(true);
			                  iv.setPolyline(input);
			                  iv.setEdges(inter_edges);
			                  iv.setCoords(clipping_coords[j]);

			                  if (o.accept(&iv)) {
			                  		#pragma omp critical(new_quad)
			                    	new_Quadrants.push_back(o);
			                  } else {
			                      //The element doesn't intersect any input face.
			                      //It must be checked if it's inside or outside.
			                      //Only in the first case add it to new_Quadrants.
			                      //Test this with parent Quadrant faces only.

			                      //Comment the following lines of this 'else' if
			                      //only intersecting Quadrants are meant to be
			                      //displayed.

			                      //note: inter_edges is quite enough to check if
			                      //element is inside input, no Quadrant needed,
			                      //so i moved the method to mesher  --setriva

			                      if (isItIn(input, inter_edges, clipping_coords[j])) {
			                      		#pragma omp critical(new_quad)
			                        	new_Quadrants.push_back(o);
			                      }
			                  }
			              }
			          }
			      }
			  	} //END FOR QUADRANTS


				  //auto end_block_time = chrono::high_resolution_clock::now();
				  //time_inside_block += std::chrono::duration_cast<chrono::milliseconds>(end_block_time - start_block_time).count();
				  //cout << time << " / " << std::chrono::duration_cast<chrono::milliseconds>(end_block_time - start_block_time).count();
				  //cout << " ms for " << range.size() << " quadrants" << endl;

			  	}//END PARALLEL

                //auto end_outside_block_time = chrono::high_resolution_clock::now();

                // don't forget to update list
                tmp_Quadrants.assign(make_move_iterator(new_Quadrants.begin()), make_move_iterator(new_Quadrants.end()));
                new_Quadrants.clear();

                //if no points were added at this iteration, it is no longer
                //necessary to continue the refinement.
                if (new_pts.empty()) {
                    cout << "warning at Mesher::generateQuadtreeMesh no new points!!!\n";
                    break;
                }

                //add the new points to the vector
                points.reserve(points.size() + new_pts.size());
                points.insert(points.end(), new_pts.begin(), new_pts.end());

                auto end_refine_rl_time = chrono::high_resolution_clock::now();
                long total = std::chrono::duration_cast<chrono::milliseconds>(end_refine_rl_time - start_refine_rl_time).count();

             	cout << "         * level " << i << " in "
                     << total;
                cout << " ms" << endl;

                std::cout << "           ---- Points : " << points.size() << std::endl;
	            std::cout << "           ---- QuadEdge : " <<  QuadEdges.size() << std::endl;
	            std::cout << "           ---- Quadrants : " << tmp_Quadrants.size() << std::endl;

                // inside / split / outside stats

                //long outside = std::chrono::duration_cast<chrono::milliseconds>(end_outside_block_time - start_outside_block_time).count();
                //cout << "TBB for outside " << outside << " ms (" << (outside * 100.0 / total) << "%) ";
                //cout << "TBB for inside " << time_inside_block << " ms (all threads cumulated) ";
                //cout << " split visitor " << time_split_visitor << " ms (" << (time_split_visitor * 100.0 / time_inside_block) << "% of time inside) ";
                //cout << endl;





            } //END FOR REFINEMENT LEVEL

            // output result to mesher ! comment if not needed
            //Quadrants.clear();
            //Quadrants.assign(tmp_Quadrants.begin(), tmp_Quadrants.end());

            //QuadEdges.clear();
            //QuadEdges.insert(quadEdges.begin(), quadEdges.end());

            // std::cout << "----------------------------------------------------------" << std::endl;
            // std::cout << "----------------------------------------------------------" << std::endl;
            // std::cout << "---------------------END OF OPENMP------------------------" << std::endl;
            // std::cout << "----------------------------------------------------------" << std::endl;
            // std::cout << "----------------------------------------------------------" << std::endl;

            return result;
        }


        string Mesher::refineMeshReductionOpenMP(const int nbThread, list<Quadrant> Quadrants, vector<MeshPoint> points,
                                        set<QuadEdge> QuadEdges,
                                        const list<RefinementRegion *> &all_reg, const unsigned short &rl,
                                        Polyline &input, bool V1) {
			string result = "";

            int MAX_THREAD = omp_get_max_threads();

            if (nbThread > MAX_THREAD || nbThread < 0) {
                result = "Invalid number of threads or not supported by computer\n";
                return result;
            }
            omp_set_num_threads(nbThread);

            string version = V1?"V1":"V2";

            //std::cout << "Start refine mesh reduction " << version << " Open MP with " << nbThread << " threads." << std::endl;

            //Convert list into vector of temp Quadrants
            vector<Quadrant> tmp_Quadrants;
            tmp_Quadrants.assign(make_move_iterator(Quadrants.begin()), make_move_iterator(Quadrants.end()));


            //Each thread has it's own data set
            vector<Quadrant> thread_new_Quadrants[nbThread];
            vector<Point3D> thread_new_pts[nbThread];
            set<QuadEdge> thread_new_edges[nbThread];

            //Result of reduce :
            vector<Quadrant> reduced_new_Quadrants;

            //Debugging
            vector<int> nbQuadPerThread(nbThread, 0);


            for (unsigned short i = 0; i < rl; i++) {
                auto start_refine_rl_time = chrono::high_resolution_clock::now();


				// std::cout << "Start Parallelisation : " << std::endl;
				// std::cout << "nbPoints   " << points.size() << std::endl;
				// std::cout << "nbEdges   " <<QuadEdges.size() <<  std::endl;
				// std::cout << "nbQuad   " << tmp_Quadrants.size() << std::endl;

                auto start_accumulation_time = chrono::high_resolution_clock::now();
				//Begin of parallel region
				#pragma omp parallel shared(thread_new_Quadrants, thread_new_pts, thread_new_edges)
				{

					const int THREAD_NUMBER = omp_get_thread_num();

					//std::cout << "THREAD NUMBER : " <<  << std::endl;

					//Create reference to the dataset of actual thread
					vector<Quadrant> &new_Quadrants = thread_new_Quadrants[THREAD_NUMBER];
					vector<Point3D> &new_pts = thread_new_pts[THREAD_NUMBER];
					set<QuadEdge> &new_edges = thread_new_edges[THREAD_NUMBER];

					new_Quadrants.clear();
					new_pts.clear();
					new_edges.clear();

					//create visitors and give them variables
					//Each thread link its own dataset
					SplitVisitorReductionOpenMP sv;
					sv.setPoints(points); // READ only

					//sv.setEdges(new_edges); // INSERT / REMOVE / READ
					//Split the method to have a dataset for reading only (current_edges)
					//And one only for new edges -> empty at the beginning, filled by sv
					sv.setCurrentEdges(QuadEdges);
					sv.setNewEdges(new_edges);

					sv.setNewPts(new_pts); // INSERT / READ
					sv.setThreadNum(THREAD_NUMBER);

					//nbQuadPerThread[THREAD_NUMBER] = 0;

					//split the Quadrants as needed
			  		// schedule(dynamic)
			  		#pragma omp for
			  		for (unsigned int j = 0; j < tmp_Quadrants.size(); ++j) {
			  			Quadrant &iter = tmp_Quadrants[j];

			  			//nbQuadPerThread[THREAD_NUMBER]++;

			  		//Check if to refine
						bool to_refine = false;
						list<RefinementRegion *>::const_iterator reg_iter;

						for (reg_iter = all_reg.begin(), reg_iter++; reg_iter != all_reg.end(); ++reg_iter) {

							unsigned short region_rl = (*reg_iter)->getRefinementLevel();
							if (region_rl < i) {
							  continue;
							}

							//If the Quadrant has a greater RL than the region needs, continue
							if (region_rl <= iter.getRefinementLevel()) {
								continue;
							}

							//Get the two extreme nodes of the Quadrant to test intersection with
							//this RefinementRegion. If not, conserve it as it is.
							//unsigned int n_idx1 = (*iter).getPoints()[0];
							//unsigned int n_idx2 = (*iter).getPoints()[2];


							// intersectQaudrant can modify the quadrant with
							// function Polyline::getNbFeatures in RefinementboundaryRegion
							// maybe no problem for parallelisation, as this information is
							// not used by other thread
							if ((*reg_iter)->intersectsQuadrant(points, iter)) {
								to_refine = true;
								break;
							}
						}

					//END check if to refine


						//now if refinement is not needed, we add the Quadrant as it was.
				      	if (!to_refine) {
				      		
				        	new_Quadrants.push_back(iter);
				        	//End of for loop
				      	} else {

				      		list<unsigned int> &inter_edges = iter.getIntersectedEdges();
				          	unsigned short qrl = iter.getRefinementLevel();

				          	vector<vector<Point3D> > clipping_coords;
				          	sv.setClipping(clipping_coords);

				          	vector<vector<unsigned int> > split_elements;
				          	sv.setNewEles(split_elements);

				          	iter.accept(&sv);

				          	if (inter_edges.empty()) {
			              		for (unsigned int j = 0; j < split_elements.size(); j++) {
			                  		Quadrant o(split_elements[j], qrl + 1);
			                  	
			                  		new_Quadrants.push_back(o);
			              		}
			              	} else {


			              		for (unsigned int j = 0; j < split_elements.size(); j++) {
									Quadrant o(split_elements[j], qrl + 1);

									//the new points are inserted in bash at the end of this
									//iteration. For this reason, the coordinates must be passed
									//"manually" at this point (clipping_coords).

									IntersectionsVisitor iv(true);
									iv.setPolyline(input);
									iv.setEdges(inter_edges);
									iv.setCoords(clipping_coords[j]);

									if (o.accept(&iv)) {
										new_Quadrants.push_back(o);
			                  		} else {
			                  			//The element doesn't intersect any input face.
										//It must be checked if it's inside or outside.
										//Only in the first case add it to new_Quadrants.
										//Test this with parent Quadrant faces only.

										//Comment the following lines of this 'else' if
										//only intersecting Quadrants are meant to be
										//displayed.

										//note: inter_edges is quite enough to check if
										//element is inside input, no Quadrant needed,
										//so i moved the method to mesher  --setriva

										if (isItIn(input, inter_edges, clipping_coords[j])) {
											new_Quadrants.push_back(o);
										}
									} // END ELSE ACCEPT
								} // END FOR SPLIT ELEMENTS
							} //END ELSE INTER_EDGES EMPTY
						} //END ELSE TO REFINE
					} //END FOR QUADRANTS

				} // END PARALLEL REGION

				auto end_accumulation_time = chrono::high_resolution_clock::now();
				long totalAccumulationTime = std::chrono::duration_cast<chrono::milliseconds>(end_accumulation_time - start_accumulation_time).count();
                

				//vector<Point3D> new_pts;
				//set<QuadEdge> new_edges;
				vector<Quadrant> new_quadrants;

				// Fill index 0 in arrays thread_new_pts, new_edges, new_Quadrants
				// with a size=nbThread the result of the merge of all thread's new elements.
				// points.size() is needed to know at which index threads have started
				auto start_reduce_time = chrono::high_resolution_clock::now();



				if (V1) {
					make_reduceV1(thread_new_pts, thread_new_edges, thread_new_Quadrants,
							nbThread, points.size(), i,
							points, QuadEdges, new_quadrants);	
				}
				else {
					make_reduceV2(thread_new_pts, thread_new_edges, thread_new_Quadrants,
							nbThread, points.size(), i,
							points, QuadEdges, new_quadrants);
				}
				

				auto end_reduce_time = chrono::high_resolution_clock::now();
				long totalReduceTime = std::chrono::duration_cast<chrono::milliseconds>(end_reduce_time - start_reduce_time).count();


				//TODO fill new_edges directly in reduce....

				//Update edges
				// for (set<QuadEdge>::iterator i = new_edges.begin(); i != new_edges.end(); ++i)
				// {
				// 	QuadEdges.insert(*i);
				// }

				// don't forget to update list
				// tmp_Quadrants.reserve(tmp_Quadrants.size() + new_quadrants.size());
				// tmp_Quadrants.insert(tmp_Quadrants.end(), new_quadrants.begin(), new_quadrants.end());
                
                //tmp_Quadrants.assign(make_move_iterator(new_quadrants.begin()), make_move_iterator(new_quadrants.end()));
                
				tmp_Quadrants.swap(new_quadrants);

                new_quadrants.clear();

                //if no points were added at this iteration, it is no longer
                //necessary to continue the refinement.
                // if (new_pts.empty()) {
                //     cout << "warning at Mesher::generateQuadtreeMesh no new points!!!\n";
                //     break;
                // }

                // //add the new points to the vector
                // points.reserve(points.size() + new_pts.size());
                // points.insert(points.end(), new_pts.begin(), new_pts.end());

                auto end_refine_rl_time = chrono::high_resolution_clock::now();
                long total = std::chrono::duration_cast<chrono::milliseconds>(end_refine_rl_time - start_refine_rl_time).count();
                
             	result += "Level " + std::to_string(i) + " in " + std::to_string(total) + " ms\n";
             	//result += "Time in accumulation = " + std::to_string(totalAccumulationTime) + "ms\n";
             	//result += "Time in reduce = " + std::to_string(totalReduceTime) + "ms\n";

                result += "Points " + std::to_string(points.size()) + "\n";
	            result += "QuadEdge " +  std::to_string(QuadEdges.size()) + "\n";
	            result += "Quadrants " + std::to_string(tmp_Quadrants.size()) + "\n";

			} // END FOR REFINEMENT LEVEL

			return result;

            // std::cout << "----------------------------------------------------------" << std::endl;
            // std::cout << "----------------------------------------------------------" << std::endl;
            // std::cout << "---------------------END OF OPENMP REDUCTION " << version << "-----------" << std::endl;
            // std::cout << "----------------------------------------------------------" << std::endl;
            // std::cout << "----------------------------------------------------------" << std::endl;
        }


    /**
     * @brief Reduction of number_of_threads thread into new_pts, new_edges and new_quadrants.
     * @details V1 of the algorithm of reduction, with a loop in points for the creation of
     * the map that link local index of points to global
     * 
     * @param thread_new_pts array that contains the new_pts created by all threads during accumulation.
     * @param thread_new_edges array that contains the new_edges created by all threads during accumulation.
     * @param thread_new_quadrants array that contains the new_quadrants created by all threads during accumulation.
     * @param number_of_threads number of threads
     * @param total_nb_points_before_reduce Constant
     * @param rl refinement level
     * @param new_pts The resulting new_pts (start from old points)
     * @param new_edges The resulting new_edges (start from old edges)
     * @param new_quadrants The resulting new_quadrants (empty at the beginning)
     */
	void make_reduceV1(	vector<Point3D> *threads_new_pts, set<QuadEdge> *threads_new_edges, vector<Quadrant> *threads_new_quadrants, 
						unsigned int number_of_threads, const unsigned int total_nb_points_before_reduce, unsigned int rl,
						vector<MeshPoint> &new_pts, set<QuadEdge> &new_edges, vector<Quadrant> &new_quadrants) {

		new_quadrants.clear();

		auto start_reduce_time = chrono::high_resolution_clock::now();

		long totalPointTime = 0;
		long totalEdgesTime = 0;
		long totalQuadrantTime = 0;

		//The current number of points
		unsigned int total_nb_points = total_nb_points_before_reduce;
		// The map to link real index of new_pts
		unordered_map<unsigned int, unsigned int> map_new_pts;
		// The task to global
		unordered_map<unsigned int, unsigned int> taskToGlobal;
		for (int thread_number = 0; thread_number < number_of_threads; ++thread_number)
		{
			//std::cout << "Points start" << std::endl;
			auto start_point_time = chrono::high_resolution_clock::now();

			const vector<Point3D> &thread_new_pts = threads_new_pts[thread_number];
			for (unsigned int i = 0; i < thread_new_pts.size(); ++i) {
				const unsigned int indexNewPt = total_nb_points_before_reduce + i;
				const Point3D& point = thread_new_pts[i];

				size_t hashPoint = point.operator()(point);
		        auto found = map_new_pts.find(hashPoint);

		        if (found == map_new_pts.end()) {
		            //We did not found the point
		            //We add it in the global new_pts
		            new_pts.push_back(point);
		            //We add it to the global map
		            map_new_pts[hashPoint] = total_nb_points;
		    		total_nb_points++;        
		        }
		        
		        //Else we found it, we link it
		        taskToGlobal[indexNewPt] = map_new_pts[hashPoint];
			}

			auto end_point_time = chrono::high_resolution_clock::now();

			totalPointTime += std::chrono::duration_cast<chrono::milliseconds>(end_point_time - start_point_time).count();

			//std::cout << "Points end" << std::endl;

			//std::cout << "Edge start" << std::endl;

			auto start_edges_time = chrono::high_resolution_clock::now();

			set<QuadEdge> &thread_new_edges = threads_new_edges[thread_number];
			for (const QuadEdge &local_edge : thread_new_edges) {

				// build new edge with right index
		        vector<unsigned long> index(3, 0);

		        //bool all_created = true;
		        for (unsigned int j = 0; j < 3; j++) {
		            if (local_edge[j] < total_nb_points_before_reduce || local_edge[j] == 0) {
		            	//Don't forget midpoint = 0
		                // index refer point not created during this refinement level
		                index[j] = local_edge[j];
		            } else {
		                // point created, need to update the point with correct index
		                index[j] = taskToGlobal[local_edge[j]];
		                //all_created = false;
		            }
		        }
		        QuadEdge edge(index[0], index[1], index[2]);


		        new_edges.insert(edge);
				
				//	Can't modify..		        
		        // for (unsigned int j = 0; j < 3; j++) {
		        // 	if (local_edge[j] >= total_nb_points_before_reduce || local_edge[j] != 0) {
		        //         // point created, need to update the point with correct index
		        //         local_edge[j] = taskToGlobal[local_edge[j]];
		        //     }
		        // }
		        // new_edges.insert(local_edge);

			}

			auto end_edges_time = chrono::high_resolution_clock::now();
			totalEdgesTime += std::chrono::duration_cast<chrono::milliseconds>(end_edges_time - start_edges_time).count();
			//std::cout << "Edge end" << std::endl;

			//std::cout << "Quad start" << std::endl;
			auto start_quadrant_time = chrono::high_resolution_clock::now();

			vector<Quadrant> &thread_new_quadrants = threads_new_quadrants[thread_number];
			for(Quadrant &local_quad : thread_new_quadrants) {
				// update new quad with right index
				//Keep intersected edges
		        unsigned int pointIndex;
		        for (unsigned int j = 0; j < 4; j++) {
		        	pointIndex = local_quad.getPointIndex(j);
		            if (pointIndex < total_nb_points_before_reduce) {
		                // index refer point not created during this refinement level
		                local_quad.setPointIndexAt(j, pointIndex);
		            } else {
		                // point created, need to update the point with correct index
		                local_quad.setPointIndexAt(j, taskToGlobal[pointIndex]);
		            }
		        }
				new_quadrants.push_back(local_quad);
			}	

			auto end_quadrant_time = chrono::high_resolution_clock::now();
			totalQuadrantTime += std::chrono::duration_cast<chrono::milliseconds>(end_quadrant_time - start_quadrant_time).count();

	    	//std::cout << "Quad end" << std::endl;

		} // END FOR ALL THREAD

		auto end_reduce_time = chrono::high_resolution_clock::now();		
		long totalReduceTime = std::chrono::duration_cast<chrono::milliseconds>(end_reduce_time - start_reduce_time).count();             

		// std::cout << "Reduce V1 at level rl=" << rl << std::endl;
		// std::cout << "\t* Point time = " << totalPointTime << " ms" << std::endl;
		// std::cout << "\t* Edges time = " << totalEdgesTime << " ms" << std::endl;
		// std::cout << "\t* Quadrant time = " << totalQuadrantTime << " ms" << std::endl;
		// std::cout << "\t* Total time = " << totalReduceTime << " ms" << std::endl;
	}

	/**
     * @brief Reduction of number_of_threads thread into new_pts, new_edges and new_quadrants.
     * @details V1 of the algorithm of reduction, with a loop in points for the creation of
     * the map that link local index of points to global
     * 
     * @param thread_new_pts array that contains the new_pts created by all threads during accumulation.
     * @param thread_new_edges array that contains the new_edges created by all threads during accumulation.
     * @param thread_new_quadrants array that contains the new_quadrants created by all threads during accumulation.
     * @param number_of_threads number of threads
     * @param total_nb_points_before_reduce Constant
     * @param rl refinement level
     * @param new_pts The resulting new_pts, start from old points (filled)
     * @param new_edges The resulting new_edges. Must contains old edges at the beginning
     * @param new_quadrants The resulting new_quadrants
     */
	void make_reduceV2(	vector<Point3D> *threads_new_pts, set<QuadEdge> *threads_new_edges, vector<Quadrant> *threads_new_quadrants, 
						unsigned int number_of_threads, const unsigned int total_nb_points_before_reduce, unsigned int rl,
						vector<MeshPoint> &new_pts, set<QuadEdge> &new_edges, vector<Quadrant> &new_quadrants) {
		new_quadrants.clear();
		auto start_reduce_time = chrono::high_resolution_clock::now();

		long totalEdgesAndPointTime = 0;
		long totalQuadrantTime = 0;

		std::tr1::unordered_map<unsigned int, unsigned int> threadMap;
		for (int thread_number = 0; thread_number < number_of_threads; ++thread_number)
		{
			//std::cout << "Edge start" << std::endl;
			auto start_edges_time = chrono::high_resolution_clock::now();

			set<QuadEdge> &thread_new_edges = threads_new_edges[thread_number];
			const vector<Point3D> &thread_new_pts = threads_new_pts[thread_number];
			threadMap.clear();
            for (const QuadEdge &local_edge : thread_new_edges) {

                if (local_edge[0] < total_nb_points_before_reduce && local_edge[1] < total_nb_points_before_reduce) {
                    // midpoint created on existing thread
                    auto found = new_edges.find(local_edge);

                    if ((*found)[2] != 0) {
                        // midpoint already created by another thread
                        threadMap.insert(pair<unsigned long, unsigned long>(local_edge[2], (*found)[2]));
                    } else {
                        // midpoint created only by this thread for now
                        // add midpoint in point and update edge midpoint
                        //(*found).updateMidPoint(tmp_points.size());
                        QuadEdge quadEdge(local_edge);
                        quadEdge.updateMidPoint(new_pts.size());
                        auto pos = new_edges.erase(found);
                        new_edges.insert(pos, quadEdge);

                        threadMap.insert(pair<unsigned long, unsigned long>(local_edge[2], new_pts.size()));
                        new_pts.emplace_back(thread_new_pts[local_edge[2] - total_nb_points_before_reduce]);
                    }
                } else {
                    // it not about midpoint created on "old" edge but totally new edge
                    // check if index need to be created or change

                    // build new edge with correct index
                    vector<unsigned int> index(3, 0);

                    for (unsigned int j = 0; j < 3; j++) {
                        if (local_edge[j] < total_nb_points_before_reduce) {
                            // index refer point not created during this refinement level
                            index[j] = local_edge[j];
                        } else {
                            // point created locally, need to update the point with correct index
                            if (threadMap.find(local_edge[j]) != threadMap.end()) {
                                index[j] = threadMap[local_edge[j]];
                            } else {
                                index[j] = new_pts.size();
                                threadMap.insert(
                                        pair<unsigned int, unsigned int>(local_edge[j], new_pts.size()));
                                new_pts.emplace_back(thread_new_pts[local_edge[j] - total_nb_points_before_reduce]);
                            }
                        }
                    }

                    QuadEdge edge(index[0], index[1], index[2]);

                    new_edges.insert(edge); // try insert
                }
            }

			auto end_edges_time = chrono::high_resolution_clock::now();
			totalEdgesAndPointTime += std::chrono::duration_cast<chrono::milliseconds>(end_edges_time - start_edges_time).count();
			//std::cout << "Edge end" << std::endl;

			//std::cout << "Quad start" << std::endl;
			auto start_quadrant_time = chrono::high_resolution_clock::now();

			vector<Quadrant> &thread_new_quadrants = threads_new_quadrants[thread_number];
			for (Quadrant &local_quad : thread_new_quadrants) {
                // build new quad with right index
                vector<unsigned int> new_pointindex(4, 0);

                for (unsigned int j = 0; j < 4; j++) {
                    if (local_quad.getPointIndex(j) < total_nb_points_before_reduce) {
                        // index refer point not created during this refinement level
                        new_pointindex[j] = local_quad.getPointIndex(j);
                    } else {
                        // point created, need to update the point with correct index
                        new_pointindex[j] = threadMap[local_quad.getPointIndex(j)];
                    }
                }

                new_quadrants.emplace_back(new_pointindex, local_quad);
            }

			auto end_quadrant_time = chrono::high_resolution_clock::now();
			totalQuadrantTime += std::chrono::duration_cast<chrono::milliseconds>(end_quadrant_time - start_quadrant_time).count();

	    	//std::cout << "Quad end" << std::endl;

		} // END FOR ALL THREAD

		auto end_reduce_time = chrono::high_resolution_clock::now();		
		long totalReduceTime = std::chrono::duration_cast<chrono::milliseconds>(end_reduce_time - start_reduce_time).count();             

		// std::cout << "Reduce V2 at level rl=" << rl << std::endl;
		// std::cout << "\t* Edges And Point time = " << totalEdgesAndPointTime << " ms" << std::endl;
		// std::cout << "\t* Quadrant time = " << totalQuadrantTime << " ms" << std::endl;
		// std::cout << "\t* Total time = " << totalReduceTime << " ms" << std::endl;
	}

}