/*
 * work_generator.h
 *
 *  Created on: Apr 6, 2014
 *      Author: nrakover
 */

#ifndef WORK_GENERATOR_H_
#define WORK_GENERATOR_H_

#include <vector>
#include "boinc_db.h"
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
	const char* app_name_;
	const char* in_template_file_;
	const char* out_template_file_;

	char* in_template_;
	DB_APP app_;

};

} // namespace alfax


#endif /* WORK_GENERATOR_H_ */
