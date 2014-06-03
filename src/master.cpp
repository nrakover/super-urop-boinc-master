/*
 * master.cpp
 *
 *  Created on: Feb 22, 2014
 *      Author: nrakover
 */

#include "master.h"
#include "timeslice.h"
#include "config_reader.h"
#include "utils.h"
#include "work_generator.h"

#include <unistd.h>
#include <iostream>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

namespace alfax {

// Reports whether the indicated output file is ready.
bool OutputReady(const char *file) {

	char path[256];
	sprintf(path, "cons/%s", file);
	struct stat st;
	int result = stat(path, &st);
	return result == 0;
}

Master::Master(const string &config_file) {
	ConfigReader *config_reader = new ConfigReader(config_file);
	N_ = 1 + config_reader->getHiddenVariableSupport();		// 2 + ...
	Sigma_ = config_reader->getObservationsSupport();
	data_ = config_reader->getDataVector();
	max_iterations_ = config_reader->getMaxIterations();
	stopping_condition_ = config_reader->getLogLikelihoodChange();

	vector<double> params(config_reader->getParametersVector());
	if (params.empty())
		RandInitParams();
	//TODO: else set starting conditions from params
}

void Master::Train() {
	long double old_log_likelihood = DistributeLL();

	cout << "Starting loglikelihood: " << old_log_likelihood << endl;

	DistributeE();
	M();

	long double new_log_likelihood = DistributeLL();
	long double change_in_log_likelihood = new_log_likelihood
			- old_log_likelihood;
	old_log_likelihood = new_log_likelihood;

	cout << ">>>>>> MAX ITERATION: " << max_iterations_ << endl;

	int iteration = 1;
	while (iteration < max_iterations_) {

		cout << "Iteration " << iteration << endl;
		cout << "Log-likelihood: " << old_log_likelihood << endl;

		if (change_in_log_likelihood < stopping_condition_) {
			cout << ">>>>>>>>>> STOPPING CONDITION MET" << endl;
			break;
		}
		DistributeE();
		M();

		new_log_likelihood = DistributeLL();
		change_in_log_likelihood = new_log_likelihood - old_log_likelihood;
		old_log_likelihood = new_log_likelihood;
		iteration++;
	}
	ExportParams();
	return;
}


void Master::DistributeE() {

	const int DATA_POINTS_PER_SHARD = 500;
	int retval;

	// Generates file with latest global params for clients to use
	ExportParams();	// Export to working directory.

	cout << "[MASTER] Initializing WorkGenerator." << endl;

	int ts = time(0);
	WorkGenerator *wg = new WorkGenerator("HMM_EM",
			"estep_in", "estep_out",
			ts, ceil((1.0 * data_.size()) / DATA_POINTS_PER_SHARD));

	cout << "[MASTER] WorkGenerator initialized." << endl;

	// Iterate through data splits and generate jobs
	// to compute partial expected counts.
	vector<vector<Timeslice> > data_shard;
	data_shard.resize(DATA_POINTS_PER_SHARD);
	int num_splits = data_.size() / DATA_POINTS_PER_SHARD;
	for (int split = 0; split < num_splits; split++) {

		for (int i = 0; i < DATA_POINTS_PER_SHARD; i++) {
			data_shard.at(i) = data_.at(split * DATA_POINTS_PER_SHARD + i);
		}
		retval = wg->MakeJob(&data_shard);
		if (retval) {
			cerr << "[MASTER] error in MakeJob(): error code " << retval
					<< endl;
			exit(retval);
		}

	}
	int leftover = data_.size() - num_splits * DATA_POINTS_PER_SHARD;
	if (leftover > 0) {
		data_shard.resize(leftover);
		for (int i = 0; i < leftover; i++) {
			data_shard.at(i) = data_.at(num_splits * DATA_POINTS_PER_SHARD + i);
		}
		retval = wg->MakeJob(&data_shard);
		if (retval) {
			cerr << "[MASTER] error in MakeJob(): error code " << retval
					<< endl;
			exit(retval);
		}
	}

	cout << "[MASTER] Estep work units generated." << endl;

	// Wait for clients to complete
	char fn[256];
	sprintf(fn, "trans_%d", ts);
	while (!OutputReady(fn)) {
		cout << "[MASTER] waiting on trans results" << endl;
		sleep(100);
	}
	sprintf(fn, "emi_%d", ts);
	while (!OutputReady(fn)) {
		cout << "[MASTER] waiting on emi results" << endl;
		sleep(100);
	}
	// Get results
	LoadECounts(ts, &A_counts_, &B_counts_);

}

void Master::M() {

	// Populate the transition probabilities table
	for (int i = 0; i < N_; i++) { //N_-1
		vector<double> row = A_counts_.at(i);
		long double sum = std::accumulate(row.begin(), row.end(), 0.0);
		if (!(sum > 0.0))
			sum = 1.0;
		double smoothing_term = sum / (1000.0*(N_-1));
		double smoothing_sum = sum + (sum / 1000.0);
		for (int j = 1; j < N_; j++) {
			A_.at(i).at(j) = (row.at(j) + smoothing_term) / smoothing_sum;
		}
	}

	// Populate the emission probabilities table
	for (int i = 1; i < N_; i++) { //N_-1
		for (int j = 0; j < Sigma_.size(); j++) {
			vector<double> row = B_counts_.at(i).at(j);
			long double sum = std::accumulate(row.begin(), row.end(), 0.0);
			if (!(sum > 0.0))
				sum = 1.0;
			double smoothing_term = sum / (1000.0*Sigma_.at(j));
			double smoothing_sum = sum + (sum / 1000.0);
			for (int k = 0; k < Sigma_.at(j); k++) {
				B_.at(i).at(j).at(k) = (row.at(k) + smoothing_term) / smoothing_sum;
			}
		}
	}
}


double Master::DistributeLL() {

	const int DATA_POINTS_PER_SHARD = 500;
	int retval;

	// Generates file with latest global params for clients to use
	ExportParams();	// Export to working directory.

	cout << "[MASTER] Initializing WorkGenerator." << endl;

	int ts = time(0);
	WorkGenerator *wg = new WorkGenerator("HMM_EM",
			"loglike_in", "loglike_out",
			ts, ceil((1.0 * data_.size()) / DATA_POINTS_PER_SHARD));

	cout << "[MASTER] WorkGenerator initialized." << endl;

	// Iterate through LL_clients and send them requests
	// to compute partial loglikelihoods.
	vector<vector<Timeslice> > data_shard;
	data_shard.resize(DATA_POINTS_PER_SHARD);
	int num_splits = data_.size() / DATA_POINTS_PER_SHARD;
	for (int split = 0; split < num_splits; split++) {

		for (int i = 0; i < DATA_POINTS_PER_SHARD; i++) {
			data_shard.at(i) = data_.at(split * DATA_POINTS_PER_SHARD + i);
		}
		retval = wg->MakeJob(&data_shard);
		if (retval) {
			cerr << "[MASTER] error in MakeJob(): error code " << retval
					<< endl;
			exit(retval);
		}

	}
	int leftover = data_.size() - num_splits * DATA_POINTS_PER_SHARD;
	if (leftover > 0) {
		data_shard.resize(leftover);
		for (int i = 0; i < leftover; i++) {
			data_shard.at(i) = data_.at(num_splits * DATA_POINTS_PER_SHARD + i);
		}
		retval = wg->MakeJob(&data_shard);
		if (retval) {
			cerr << "[MASTER] error in MakeJob(): error code " << retval
					<< endl;
			exit(retval);
		}
	}

	cout << "[MASTER] LL work units generated." << endl;

	// Wait for clients to complete
	char fn[256];
	sprintf(fn, "loglike_%d", ts);
	while (!OutputReady(fn)) {
		cout << "[MASTER] waiting on LL results" << endl;
		sleep(100);
	}
	// Get results
	return LoadLL(ts);

}

void Master::ExportParams() {

	ofstream output_file;
	output_file.open("transitions.txt");

	for (int i = 0; i < N_; i++) {
		for (int j = 0; j < N_; j++) {
			output_file << A_.at(i).at(j) << ";";
		}
		output_file << "\n";
	}

	output_file.close();
	output_file.clear();

	output_file.open("emissions.txt");

	for (int i = 1; i < N_; i++) {		// N_-1
		for (int j = 0; j < Sigma_.size(); j++) {
			for (int k = 0; k < Sigma_.at(j); k++) {
				output_file << B_.at(i).at(j).at(k) << ";";
			}
			output_file << "|";
		}
		output_file << "\n";
	}

	output_file.close();
}

void Master::LoadECounts(int timestamp, vector<vector<double> > *A_counts,
		vector<vector<vector<double> > > *B_counts) {

	char transitions_path[256];
	sprintf(transitions_path, "cons/trans_%d", timestamp);
	char emissions_path[256];
	sprintf(emissions_path, "cons/emi_%d", timestamp);
	string line;

	ifstream params_file;
	params_file.open(transitions_path);	// First read transition matrix

	int N = 0;
	int start = 0;
	A_counts->resize(1);
	std::getline(params_file, line);
	for (int i = 0; i < line.length(); i++) {
		if (line.substr(i, 1).compare(";") == 0) {
			N++;
			A_counts->at(0).push_back(
					atof(line.substr(start, i - start).c_str()));
			start = i + 1;
		}
	}
	A_counts->resize(N);

	int row = 1;
	while (std::getline(params_file, line)) {
		if (row > N - 1)
			break;
		int start = 0;
		for (int i = 0; i < line.length(); i++) {
			if (line.substr(i, 1).compare(";") == 0) {
				A_counts->at(row).push_back(
						atof(line.substr(start, i - start).c_str()));
				start = i + 1;
			}
		}
		row++;
	}
	params_file.close();

	params_file.open(emissions_path);// Next read the emissions matrix

	B_counts->resize(N);
	row = 1;
	while (std::getline(params_file, line)) {
		if (row > N - 1)
			break;
		B_counts->at(row).resize(1);
		start = 0;
		int obs_support = 0;
		int obs_index = 0;
		for (int i = 0; i < line.length(); i++) {
			if (line.substr(i, 1).compare(";") == 0) {
				B_counts->at(row).at(obs_index).push_back(
						atof(line.substr(start, i - start).c_str()));
				obs_support++;
				start = i + 1;
			} else if (line.substr(i, 1).compare("|") == 0) {
				obs_index++;
				B_counts->at(row).resize(obs_index + 1);
				obs_support = 0;
				start = i + 1;
			}
		}
		B_counts->at(row).resize(obs_index);
		row++;
	}
}

double Master::LoadLL(int timestamp) {

	char file[256];
	sprintf(file, "cons/loglike_%d", timestamp);
	ifstream f;
	f.open(file);
	string content;
	std::getline(f, content);
	f.close();

	return atof(content.c_str());
}

void Master::RandInitParams() {

	srand((unsigned) time(0));  // seed the random number generator

	// #################  Initialize A_   #################
	A_.resize(N_);
	// Initialize the state priors
	A_.at(0).resize(N_);
	A_.at(0).at(0) = 0.0;
	//	A_.at(0).at(N_-1) = 0.0;
	vector<long double> rand_dist = GenerateUniformRandDist(N_ - 1);	// N_-2
	for (int j = 0; j < N_ - 1; j++) {			//  N_-2
		A_.at(0).at(j + 1) = rand_dist.at(j);
	}
	// Initialize the transition probabilities
	for (int i = 1; i < N_; i++) {			// N_-1
		A_.at(i).resize(N_);
		A_.at(i).at(0) = 0.0;

		vector<long double> rand_dist = GenerateUniformRandDist(N_ - 1); // N_-2
		for (int j = 0; j < N_ - 1; j++) {				// N_-2
			A_.at(i).at(j + 1) = rand_dist.at(j);
		}
	}
	// Initialize END state transitions
	//	A_.at(N_-1).resize(N_);
	//	for (int i = 0; i < N_-1; i++) {
	//		A_.at(N_-1).at(i) = 0.0;
	//	}
	//	A_.at(N_-1).at(N_-1) = 1.0;

	// #################  Initialize B_   #################
	B_.resize(N_);
	for (int i = 1; i < N_; i++) {
		B_.at(i).resize(Sigma_.size());
		for (int j = 0; j < Sigma_.size(); j++) {
			B_.at(i).at(j).resize(Sigma_[j]);

			vector<long double> rand_dist = GenerateUniformRandDist(Sigma_[j]);
			for (int k = 0; k < Sigma_[j]; k++) {
				B_.at(i).at(j).at(k) = rand_dist[k];
				//				if (i == 0 || i == N_-1)
				//					B_.at(i).at(j).at(k) = 0.0;
			}
		}
	}
}

} // namespace alfax
