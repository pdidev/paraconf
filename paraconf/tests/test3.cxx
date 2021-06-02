/*******************************************************************************
 * Copyright (C) 2021 Institute of Bioorganic Chemistry Polish Academy of Science (PSNC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of CEA nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <iostream>
#include <string>

#include <yaml-cpp/yaml.h>

#include <paraconf/node.h>

int main(int argc, char *argv[])
{
	if (argc != 2 ) {
		fprintf(stderr, "Error: expected 1 argument!\n");
		exit(1);
	}
	PC::Node conf = PC::Load_file(argv[1]);

	YAML::Node node = conf.yaml_node();

	int a_int = conf["a_int"].as<int>();
	if (a_int != 100) {
		std::cerr << "Error, expected: a_int = 100, is: "<< a_int << std::endl;
		exit(1);
	}

	if (conf["a_map"].type() != PC_tree_type_t::PC_MAP) {
		std::cerr << "Error, expected: conf[\"a_map\"].type() = PC_MAP, is: "<< conf["a_map"].type() << std::endl;
		exit(1);
	}

	if (node["a_int"].as<int>() !=  conf["a_int"].as<int>()) {
		std::cerr << "Error, expected: node.as<int>() = conf.as<int>(), is: "<< node["a_int"].as<int>() << std::endl;
		exit(1);
	}
	
	return 0;
}
