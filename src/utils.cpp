/*
 * utils.cpp
 *
 *  Created on: Feb 26, 2014
 *      Author: nrakover
 */

#include "utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <cstdlib>

using namespace std;

namespace alfax {

vector<long double> GenerateUniformRandDist(int n) {
	vector<int> rand_ints;
	rand_ints.resize(n);
	vector<long double> dist;
	dist.resize(n);

	long double sum = 0.0;
	for (int i = 0; i < n; i++) {
		int random_num = (rand() % 1024) + 1;
		sum += random_num;
		rand_ints[i] = random_num;
	}

	for (int i = 0; i < n; i++) {
		dist[i] = rand_ints[i] / sum;
	}

	return dist;
}

void InitializeA_counts(vector<vector<long double> > *A_counts, int N) {
	A_counts->resize(N);
	for (int i = 0; i < N; i++) {
		A_counts->at(i).resize(N);
		for (int j = 0; j < N; j++)
			A_counts->at(i).at(j) = 0.0;
	}
}

void InitializeB_counts(vector<vector<vector<long double> > > *B_counts,
		int N, vector<int> *Sigma) {
	B_counts->resize(N);
	for (int i = 0; i < N; i++) {
		B_counts->at(i).resize(Sigma->size());
		for (int j = 0; j < Sigma->size(); j++) {
			B_counts->at(i).at(j).resize(Sigma->at(j));
			for (int k = 0; k < Sigma->at(j); k++) {
				B_counts->at(i).at(j).at(k) = 0.0;
			}
		}
	}
}

//void PrintA(vector<vector<double> > *A, fstream *out) {
//	for (int i = 0; i < A->size(); i++) {
//		for (int j = 0; j < A->at(i).size(); j++) {
//			out << A->at(i).at(j);
//			out << ":";
//		}
//		out << "\n";
//	}
//}
//
//void PrintB(vector<vector<vector<double> > > *B, fstream *out) {
//	for (int i = 0; i < B->size(); i++) {
//		for (int j = 0; j < B->at(i).size(); j++) {
//			for (int k = 0; k < B->at(i).at(j).size(); k++) {
//				out << B->at(i).at(j).at(k) << ":";
//			}
//			out << "|";
//		}
//		out << "\n";
//	}
//}

} // namespace alfax
