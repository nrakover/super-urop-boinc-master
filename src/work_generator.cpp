/*
 * work_generator.cpp
 *
 *  Created on: Apr 6, 2014
 *      Author: nrakover
 */

#include "work_generator.h"

#include <sys/param.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>

#include "backend_lib.h"
#include "boinc_db.h"
#include "error_numbers.h"
#include "filesys.h"
#include "parse.h"
#include "str_replace.h"
#include "str_util.h"
#include "svn_version.h"
#include "util.h"

#include "sched_config.h"
#include "sched_util.h"
#include "sched_msgs.h"

#define REPLICATION_FACTOR  1	 // number of instances of each job

namespace alfax {

WorkGenerator::WorkGenerator(const string &app_name,
		const string &in_file_template, const string &out_file_template,
		int timestamp, int total_shards) :
		seq_num_(0), total_shards_(total_shards), timestamp_(timestamp), app_name_(
				app_name), in_template_file_(in_file_template), out_template_file_(
				out_file_template) {

	char buf[256];
	int retval = config_.parse_file();
	if (retval) {
		log_messages.printf(MSG_CRITICAL, "Can't parse config.xml: %s\n",
				boincerror(retval));
		exit(1);
	}

	retval = boinc_db.open(config_.db_name, config_.db_host, config_.db_user,
			config_.db_passwd);
	if (retval) {
		log_messages.printf(MSG_CRITICAL, "can't open db\n");
		exit(1);
	}

	sprintf(buf, "where name='%s'", app_name_.c_str());
	if (app_.lookup(buf)) {
		log_messages.printf(MSG_CRITICAL, "can't find app %s\n", app_name_.c_str());
		exit(1);
	}

	char* in_template;
	sprintf(buf, "templates/%s", in_template_file_.c_str());
	if (read_file_malloc(config_.project_path(buf), in_template)) {
		log_messages.printf(MSG_CRITICAL, "can't read input template %s\n",
				buf);
		exit(1);
	}
	in_template_ = string(in_template);

	cout << "[WORKGENERATOR] app_name = " << app_name << endl;
	cout << "[WORKGENERATOR] app_name_ = " << app_name_ << endl;
}

int WorkGenerator::MakeJob(const vector<vector<Timeslice> > *data) {

	cout << "[WORKGENERATOR] starting MakeJob for app: " << app_name_ << endl;

	DB_WORKUNIT wu;
	char name[256], path[MAXPATHLEN], prepath[MAXPATHLEN];
	const char* infiles[3];
	int retval;

	// make a unique name (for the job)
	//
	sprintf(name, "%s_%d_%d_of_%d", app_name_.c_str(), timestamp_, seq_num_++, total_shards_);

	cout << "[WORKGENERATOR] work unit name: " << name << endl;

	// Create the data file.
	// Put it at the right place in the download dir hierarchy
	//

	char data_path[MAXPATHLEN];
	sprintf(data_path, "%s_%s", app_name_.c_str(), "data");

	cout << "[WORKGENERATOR] creating data file: " << data_path << endl;

	retval = config_.download_path(data_path, path);

	cout << "[WORKGENERATOR] filepath determined: " << path << endl;

	if (retval)
		boincerror(retval);

	FILE* f = fopen(path, "w");
	if (!f)
		return ERR_FOPEN;

	cout << "[WORKGENERATOR] data file opened" << endl;

	string data_string = "";
	for (int data_index = 0; data_index < data->size(); data_index++) {
		for (int time_index = 0; time_index < data->at(data_index).size();
				time_index++) {
			for (int obs_index = 0;
					obs_index
							< data->at(data_index).at(time_index).observations_.size();
					obs_index++) {
				char append_val[256];
				sprintf(append_val, "%d", data->at(data_index).at(time_index).observations_.at(
						obs_index));
				data_string += std::string(append_val);

				if (obs_index
						!= data->at(data_index).at(time_index).observations_.size()
								- 1)
					data_string += ";";
			}
			data_string += "\n";
		}
	}

	fprintf(f, "%s", data_string.c_str());
	fclose(f);

	cout << "[WORKGENERATOR] data file generated" << endl;

	// Create the transition params file.
	// Put it at the right place in the download dir hierarchy
	//

	cout << "[WORKGENERATOR] creating trans file" << endl;

	char trans_path[MAXPATHLEN];
	sprintf(trans_path, "%s_%s", app_name_.c_str(), "transitions");
	retval = config_.download_path(trans_path, path);
	if (retval)
		boincerror(retval);

	f = fopen(path, "w");
	if (!f)
		return ERR_FOPEN;

	ifstream params_file;
	string params;
	params_file.open("transitions.txt");
	if (!params_file)
		return ERR_FOPEN;
	params.assign( (std::istreambuf_iterator<char>(params_file) ),
            (std::istreambuf_iterator<char>()    ) );
	params_file.close();

	fprintf(f, "%s", params.c_str());
	fclose(f);

	cout << "[WORKGENERATOR] trans file generated" << endl;

	// Create the emissions params file.
	// Put it at the right place in the download dir hierarchy
	//

	cout << "[WORKGENERATOR] creating emi file" << endl;

	char emi_path[MAXPATHLEN];
	sprintf(emi_path, "%s_%s", app_name_.c_str(), "emissions");
	retval = config_.download_path(emi_path, path);

	if (retval)
		boincerror(retval);

	f = fopen(path, "w");
	if (!f)
		return ERR_FOPEN;

	params_file.open("emissions.txt");
		if (!params_file)
			return ERR_FOPEN;
		params.assign( (std::istreambuf_iterator<char>(params_file) ),
	            (std::istreambuf_iterator<char>()    ) );
		params_file.close();

		fprintf(f, "%s", params.c_str());
	fclose(f);

	cout << "[WORKGENERATOR] emi file generated" << endl;

	// Fill in the job parameters
	//
	cout << "[WORKGENERATOR] filling in job parameters" << endl;

	wu.clear();
	wu.appid = app_.id;
	safe_strcpy(wu.name, name);
	wu.rsc_fpops_est = 1e12;
	wu.rsc_fpops_bound = 1e14;
	wu.rsc_memory_bound = 1e8;
	wu.rsc_disk_bound = 1e8;
	wu.delay_bound = 86400;
	wu.min_quorum = REPLICATION_FACTOR;
	wu.target_nresults = REPLICATION_FACTOR;
	wu.max_error_results = REPLICATION_FACTOR * 4;
	wu.max_total_results = REPLICATION_FACTOR * 8;
	wu.max_success_results = REPLICATION_FACTOR * 4;
	infiles[0] = data_path;
	infiles[1] = trans_path;
	infiles[2] = emi_path;

	// Register the job with BOINC
	//
	cout << "[WORKGENERATOR] registering the job with BOINC" << endl;

	sprintf(path, "templates/%s", out_template_file_.c_str());
	return create_work(wu, in_template_.c_str(), path, config_.project_path(path),
			infiles, 3, config_);

}

} // namespace alfax
