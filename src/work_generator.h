/*
 * work_generator.h
 *
 *  Created on: Apr 6, 2014
 *      Author: nrakover
 */

#ifndef WORK_GENERATOR_H_
#define WORK_GENERATOR_H_

#include <vector>
#include <string>
#include "boinc_db.h"
#include "sched_config.h"

#include "timeslice.h"

namespace alfax {

class WorkGenerator {
public:
	WorkGenerator(const string &app_name, const string &in_file_template,
			const string &out_file_template, int timestamp, int total_shards);

	int MakeJob(const vector<vector<Timeslice> > *data);

	int seq_num_;
	const int total_shards_;
	const int timestamp_;
	const string app_name_;
	const string in_template_file_;
	const string out_template_file_;

	string in_template_;
	DB_APP app_;
	SCHED_CONFIG config_;

};

} // namespace alfax


#endif /* WORK_GENERATOR_H_ */
