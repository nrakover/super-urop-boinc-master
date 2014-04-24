/*
 * utils.h
 *
 *  Created on: Feb 26, 2014
 *      Author: nrakover
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <vector>

using namespace std;

namespace alfax {

vector<long double> GenerateUniformRandDist(int n);

void InitializeA_counts(vector<vector<long double> > *A_counts, int N);

void InitializeB_counts(vector<vector<vector<long double> > > *B_counts,
		int N, vector<int> *Sigma);

//void PrintA(vector<vector<double> > *A, fstream *out);
//
//void PrintB(vector<vector<vector<double> > > *B, fstream *out);

} // namespace alfax


#endif /* UTILS_H_ */
