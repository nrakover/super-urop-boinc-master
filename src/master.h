/*
 * master.h
 *
 *  Created on: Feb 22, 2014
 *      Author: nrakover
 */

#include <vector>
#include <string>
#include "timeslice.h"

#ifndef MASTER_H_
#define MASTER_H_

namespace alfax {

class Master {
public:
	Master(const string &config_file);

	void Train();

	void DistributeE();

	void M();

	double DistributeLL();

	void ExportParams();

	void LoadECounts(int timestamp, vector<vector<double> > *A_counts,
			vector<vector<vector<double> > > *B_counts);

	double LoadLL(int timestamp);

	void RandInitParams();

	vector<vector<double> > A_;
	vector<vector<double> > A_counts_;
	vector<vector<vector<double> > > B_;
	vector<vector<vector<double> > > B_counts_;
	int N_;
	vector<int> Sigma_;
	vector<vector<Timeslice> > data_;
	int max_iterations_;
	double stopping_condition_;

};

} // namespace alfax


#endif /* MASTER_H_ */
