/*
 * Copyright (c) 2016-2019 United States Government as represented by
 * the National Aeronautics and Space Administration.  No copyright
 * is claimed in the United States under Title 17, U.S.Code. All Other
 * Rights Reserved.
 */
#include "DensityGridAStarSearch.h"
#include "DensityGrid.h"
#include "DensityGridTimed.h"
#include "Position.h"
#include "Triple.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace larcfm {


double DensityGridAStarSearch::dirWeight = 0.5; //0.5
double DensityGridAStarSearch::distWeight = 1.0; //1.0
double DensityGridAStarSearch::predDistWeight = 2.0; //2.0
bool DensityGridAStarSearch::fourway = false;
bool DensityGridAStarSearch::oldHeuristics = false;
const double DensityGridAStarSearch::diagonalCost = sqrt(2.0);

DensityGridAStarSearch::DensityGridAStarSearch() { }

bool DensityGridAStarSearch::contains(const std::vector<std::pair<int,int> >& gPath, const std::pair<int,int>& pii) const {
	for (int i = 0; i < (int) gPath.size(); i++) {
		if (gPath[i] == pii) {
			return true;
		}
	}
	return false;
}

bool DensityGridAStarSearch::contains(const std::vector<Triple<int,int,int> >& gPath, const Triple<int,int,int>& pii) const {
	for (int i = 0; i < (int) gPath.size(); i++) {
		if (gPath[i] == pii) {
			return true;
		}
	}
	return false;
}




//	/**
//	 * Return true if cell2 is in the "same direction" as the previous search (not more than a 90 degree turn in the search needed)
//	 */
//	bool DensityGridAStarSearch::sameDirection(FringeEntry c, std::pair<int,int> cell2) const {
//		if (c.path.size() == 0) return true; // no history, go anywhere
//		std::pair<int,int> cell1 = c.path[c.path.size()-1];
//		int dx1 = c.x - cell1.first;
//		int dy1 = c.y - cell1.second;
//		int dx2 = cell2.first - c.x;
//		int dy2 = cell2.second - c.y;
//		return ((dx1 == dx2 && abs(dy1-dy2) == 1) || (dy1 == dy2 && abs(dx1-dx2) == 1));
//	}

	// this gives a value of 0 to same direction, 1 to a 45 degree turn, 2 to a 90 degree turn, and 3 to a 135 degree turn, multiplied by the dirWeigh
	double DensityGridAStarSearch::directionCost(FringeEntry c, int x2, int y2, double directionWeight) const {
		if (c.path.size() < 2) return 0.0; // no history, go anywhere
		std::pair<int,int> cell1 = c.path[c.path.size()-2];
		int dx1 = Util::signTriple(c.x - cell1.first);
		int dy1 = Util::signTriple(c.y - cell1.second);
		int dx2 = Util::signTriple(x2 - c.x);
		int dy2 = Util::signTriple(y2 - c.y);
		return (abs(dx2-dx1)+abs(dy2-dy1))*directionWeight;
	}

	// this computes a distance from the end point, in squares, multiplied by the distWeight
	double DensityGridAStarSearch::predictedDistanceCost(std::pair<int,int> cell2, int endx, int endy, double distanceWeight) const {
		double sqdist = (Vect2(endx,endy)).Sub(Vect2(cell2.first, cell2.second)).norm();
		return sqdist*distanceWeight;
	}

	std::vector<std::pair<int,int> > DensityGridAStarSearch::astar(DensityGrid& dg, int endx, int endy, std::vector<FringeEntry>& fringe, std::vector<std::pair<int,int> >& searched, bool fourway_b, double directionWeight, double distanceWeight, double predictedDistanceWeight) const {
		while (fringe.size() > 0) {
			std::stable_sort(fringe.begin(), fringe.end());
			FringeEntry c = fringe[0];
			fringe.erase(fringe.begin());
			if (std::isfinite(c.getTotalCost())) { // ignore infinite cost entries
				if (c.x == endx && c.y == endy) {
					return c.getPath();
				} else {
					for (int x = -1; x <= 1; x++) {
						for (int y = -1; y <= 1 ; y++) {
							// do not check diagonals
							if (fourway_b && x != 0 && y != 0) continue;

							std::pair<int,int> cell2 = std::pair<int,int>(c.x+x, c.y+y);

							if (dg.containsCell(cell2) && !contains(searched,cell2)) {
//								double cost2 = dg.getWeight(cell2) + predictedDistanceCost(cell2,endx,endy,distanceWeight) + directionCost(c,c.x+x,c.y+y,directionWeight);
								double distcost = 1;
								if (x!=0 && y!=0) distcost = 1.414;
								double actualCost2 = dg.getWeight(cell2) + directionCost(c, c.x+x, c.y+y, directionWeight) + distcost*distanceWeight;
								double predictedCost2 = predictedDistanceCost(cell2,endx,endy,predictedDistanceWeight);
								if (oldHeuristics) {
									predictedCost2 = 0;
									actualCost2 = dg.getWeight(cell2) + predictedDistanceCost(cell2,endx,endy,1.0) + directionCost(c,cell2.first, cell2.second, 1.0);
								}

								FringeEntry c2 = FringeEntry(cell2, actualCost2, predictedCost2, c);
								fringe.push_back(c2);
								searched.push_back(cell2);
							}
						}
					}
				}
			}
		}
		return std::vector<std::pair<int,int> >();
	}


	// in this one, searched includes x, y, and source square (as
	std::vector<std::pair<int,int> > DensityGridAStarSearch::astarT(DensityGridTimed& dg, int endx, int endy, double gs, std::vector<FringeEntry>& fringe, std::vector<Triple<int,int,int> >& searched, bool fourway_b, double directionWeight, double distanceWeight, double predictedDistanceWeight) const {
		int cnt = 0;
		Position endPos = dg.center(endx,endy);
		while (fringe.size() > 0) {
			std::stable_sort(fringe.begin(), fringe.end());
			FringeEntry c = fringe[0];
			fringe.erase(fringe.begin());
			cnt++;
//std::cout << "astarT "+Fm0(cnt)+" x=" << (c.x) << " y=" << (c.y) << " cost=" << c.actualCost << " path=" << Fobj(c.path) << std::endl;
			if (std::isfinite(c.getTotalCost())) { // ignore infinite cost entries
				if (c.x == endx && c.y == endy) {
					return c.getPath();
				} else {
					for (int x = -1; x <= 1; x++) {
						for (int y = -1; y <= 1 ; y++) {
							if (fourway_b && x != 0 && y != 0) continue;
							std::pair<int,int> cell2 = std::pair<int,int>(c.x+x, c.y+y);

							Position pos1 = dg.center(c.x,c.y);
							Position pos2 = dg.center(cell2);
							double dist = pos1.distanceH(pos2);
							if (!std::isnan(dist)) { // dist == NaN if either position is invalid (i.e. center cannot be calculated)
								double dt = dist/gs;
								double t = c.t + dt;
								Triple<int,int,int> searchedcell3 = Triple<int,int,int>(c.x+x, c.y+y, 0); // do not allow revisiting cells, eventually change this to a pair?
								if (dg.containsCell(cell2) && !contains(searched,searchedcell3)) {
									double distcost = 1;
									if (x!=0 && y!=0) distcost = 1.414;
									double actualCost2 = dg.getWeightT(c.x+x,c.y+y,t) + directionCost(c,c.x+x,c.y+y,directionWeight) + distcost*distanceWeight;
									double predictedCost2 = predictedDistanceCost(cell2,endx,endy,predictedDistanceWeight);
//									double predictedCost2 = pos2.distanceH(endPos)*predictedDistanceWeight;
									if (oldHeuristics) {
										predictedCost2 = 0;
										actualCost2 = dg.getWeightT(c.x+x,c.y+y,t) + predictedDistanceCost(cell2,endx,endy,1.0) + directionCost(c,c.x+x,c.y+y, 1.0);
									}
									FringeEntry c2 = FringeEntry(t,cell2,actualCost2,predictedCost2,c);
//fpln("astarT new entry "+c2.toString());
									fringe.push_back(c2);
									searched.push_back(searchedcell3);
								}
							}
						}
					}
				}
			}
		}
		return std::vector<std::pair<int,int> >();
	}


	/**
	 * Perform an astar search through the given DensitGrid, starting at startPos and ending at endPos.
	 * This assumes the grid has a set of static polygons
	 * Returns a std::vector of grid coordinates defining a path, or null if no path found.
	 * This has the side effect of setting the searchedWeights values in the DensityGrid for display or analysis purposes.
	 */
	std::vector<std::pair<int,int> > DensityGridAStarSearch::search(DensityGrid& dg, const Position& startPos, const Position& endPos) const {
		std::pair<int,int> start = dg.gridPosition(startPos);
		std::pair<int,int> end = dg.gridPosition(endPos);
		std::vector<FringeEntry> fringe = std::vector<FringeEntry>();
		std::vector<std::pair<int,int> > searched = std::vector<std::pair<int,int> >();
		searched.push_back(start);
		double firstWeight = dg.getWeight(start);
		if (!std::isfinite(firstWeight)) return std::vector<std::pair<int,int> >(); // first cell is invalid, abort search
		fringe.push_back(FringeEntry(1, start, dg.getWeight(start), 0.0));
		bool fourway_b = fourway;
		double directionWeight = dirWeight;
		double distanceWeight = distWeight;
		double predictedDistanceWeight = predDistWeight;
		return astar(dg, end.first, end.second, fringe, searched, fourway_b, directionWeight, distanceWeight, predictedDistanceWeight);
	}

	/**
	 * Perform a search on the given densitygrid, assuming a constant groundspeed.  Polygons may be static or moving.
	 * @param dg Timed density grid with polygon and initial weight information.  Polygons may be mosing or static.
	 * @param startPos Start position for search.
	 * @param endPos Ending Position for search.
	 * @param startTime Absolute time to start the search at startPos
	 * @param gs ground speed of aircraft
	 * @return std::vector of grid coordinates for a successful path, or null if no path found.
	 */
	std::vector<std::pair<int,int> > DensityGridAStarSearch::searchT(DensityGridTimed& dg, const Position& startPos, const Position& endPos, double startTime, double gs) const {
		std::pair<int,int> start = dg.gridPosition(startPos);
		std::pair<int,int> end = dg.gridPosition(endPos);
		std::vector<FringeEntry> fringe = std::vector<FringeEntry>();
		std::vector<Triple<int,int,int> > searched = std::vector<Triple<int,int,int> >();
		searched.push_back(Triple<int,int,int>(start.first, start.second, 0));
		double firstWeight = dg.getWeight(start);
		if (!std::isfinite(firstWeight)) return std::vector<std::pair<int,int> >(); // first cell is invalid, abort search
		fringe.push_back(FringeEntry(startTime, start, dg.getWeight(start), 0.0));
		bool fourway_b = fourway;
		double directionWeight = dirWeight;
		double distanceWeight = distWeight;
		double predictedDistanceWeight = predDistWeight;
		return astarT(dg, end.first, end.second, gs, fringe, searched, fourway_b, directionWeight, distanceWeight, predictedDistanceWeight);
	}


	std::vector<std::pair<int,int> > DensityGridAStarSearch::optimalPath(DensityGrid& dg) const {
		return search(dg, dg.startPoint(), dg.endPoint());
	}

	std::vector<std::pair<int,int> > DensityGridAStarSearch::optimalPathT(DensityGridTimed& dg) const {
		return searchT(dg, dg.startPoint(), dg.endPoint(), dg.startTime(), dg.getGroundSpeed());
	}

}
