/*
 * config_reader.h
 *
 *  Created on: Feb 26, 2014
 *      Author: nrakover
 */

#include <string>
#include <vector>
#include "timeslice.h"

using namespace std;
namespace alfax{
class ConfigReader{

	int q;
	int hidden_states;
	vector<int> states_for_nodes;
	int max_iterations;
	string data_filepath;
	int log_likelihood_change;
	vector<int> features_to_use;
	string parameters_filepath;
	vector<vector<Timeslice> > processDataSpecial();

public:
	ConfigReader(const string &config_file);
	int getNumberOfObservations();
	int getHiddenVariableSupport();
	vector<int>	getObservationsSupport();
	int getMaxIterations();
	string getDataFilepath();
	vector<vector<Timeslice> > getDataVector();
	int getLogLikelihoodChange();
	vector<int>	getFeaturesToUse();
	string getParametersFilepath();
	vector<double> getParametersVector();

};
} // namespace alfax

