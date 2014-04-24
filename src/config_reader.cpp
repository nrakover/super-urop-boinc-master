//============================================================================
// Name        : SuperUROP.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include "config_reader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>


using namespace std;

namespace alfax {

ConfigReader::ConfigReader(const string &config_file) {
	q = 0;
	log_likelihood_change =  0;
	hidden_states = 0;
	max_iterations = 0;
	string line;
	ifstream myfile(config_file.c_str());
	if (myfile.is_open()) {

		//OBSERVED NODES
		getline(myfile, line);
		istringstream iss(line);
		iss >> q;

		//STATES FOR FEATURES
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		for (int i = 0; i < q; i++) {
			int n;
			iss >> n;
			states_for_nodes.push_back(n);
		}

		//HIDDEN STATES
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		iss >> hidden_states;

		//EM ITERATIONS MAX
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		iss >> max_iterations;

		//DATAFILE LOCATION
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		iss >> data_filepath;

		//CHANGE IN LOG LIKELYHOOD
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		iss >> log_likelihood_change;

		//WHICH FEATURES
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		for (int i = 0; i < q; i++) {
			int n;
			iss >> n;
			features_to_use.push_back(n);
		}

		//INITIAL PARAMETERS FILEPATH
		getline(myfile, line);
		iss.str(line);
		iss.clear();
		iss >> parameters_filepath;
		myfile.close();

//		cout << q << endl;
//		cout << states_for_nodes.size() << endl;
//		cout << hidden_states << endl;
//		cout << max_iterations << endl;
//		cout << data_filepath << endl;
//		cout << log_likelihood_change << endl;
//		cout << features_to_use.size() << endl;
//		cout << parameters_filepath << endl;
	}

	else {
		cout << "Unable to open file";
	}
}

int	ConfigReader::getNumberOfObservations(){
	return q;
}
int	ConfigReader::getHiddenVariableSupport(){
	return hidden_states;
}
vector<int>	ConfigReader::getObservationsSupport(){
	return states_for_nodes;
}
int	ConfigReader::getMaxIterations(){
	return max_iterations;
}
string ConfigReader::getDataFilepath(){
	return data_filepath;
}

vector<vector<Timeslice> > ConfigReader::getDataVector(){

	/*
	 * TODO: Normalize data format
	 *
	 * For now process based on csv we have
	 */

	vector<vector<Timeslice> > data_points = processDataSpecial();

	cout << "Config variables from data file:" << endl;
	cout << "Number of students:" << data_points.size() << endl;
	cout << "Number of weeks:" << data_points.at(0).size() << endl;
	// cout << data_points.size() << endl;
	// cout << data_points[0].size() << endl;
	cout << "Number of features:" << data_points[0][0].observations_.size() << endl;
	return data_points;

}

vector<vector<Timeslice> > ConfigReader::processDataSpecial(){
//	vector<map<int, int> > states_maps;
//	states_maps.resize(q);
//	vector<int> next_available_state;
//	next_available_state.resize(q);
//	for (int i = 0; i < q; i++) {
//		next_available_state[i] = 0;
//		states_maps[i] = map<int, int>();
//	}
	vector<vector<Timeslice> > data_points;
	ifstream myfile(data_filepath.c_str());
	if (myfile.is_open()) {
			string line;
			int count = 0;
			int i=0;
			while(getline(myfile, line)){
				int observation_count = 0;
				stringstream ss(line);
				string observation;
				vector<int> observations;
				while(getline(ss, observation, ';')){
					istringstream iss(observation);
					int n;
					iss >> n;

//					if (states_maps[observation_count].find(n) == states_maps[observation_count].end()) {
//						states_maps[observation_count][n] = next_available_state[observation_count];
//						next_available_state[observation_count]++;
//					}
					observations.push_back(n);
					observation_count++;
				}
				Timeslice t = Timeslice(observations);
				if (data_points.size()==i){
					vector<Timeslice> timeslices;
					timeslices.push_back(t);
					data_points.push_back(timeslices);
				}
				else{
					data_points.at(i).push_back(t);
				}
				count++;
				if (count == 15){
					i++;
					count=0;
				}
			}
		}
	return data_points;
}

int ConfigReader::getLogLikelihoodChange(){
	return log_likelihood_change;
}
vector<int>	ConfigReader::getFeaturesToUse(){
	return features_to_use;
}
string ConfigReader::getParametersFilepath(){
	return parameters_filepath;
}
vector<double> ConfigReader::getParametersVector(){
	/*
	 * TODO: Need to decide format for parameters files,
	 * then fill in.
	 */
	vector<double> params;
	return params;
}



}
