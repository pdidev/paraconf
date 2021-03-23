#include <iostream>

#include <paraconf/PC_node.h>


/* This file contains an example on how to use Paraconf 
 */


int main(int argc, char *argv[])
{
	/// Creates a tree that contains all the parsed data of the parameter file
	if (argc != 2 ) {
		fprintf(stderr, "Error: expected 1 argument!\n");
		exit(1);
	}
	PC_node conf = PC_load_file(argv[1]);

	/*For the sake of the illustration a tree is 
	 *           .                // origin of the tree
	 *       /   |     \
	 * .a_int   .a_float  ...       // sub-trees/node/map
	 */
	
	/// conf["a_int"] return the sub-tree under the node name ".a_int" if it is found.
	/// and .as<long>() return the leaf assuming it's an int.
	long a_int = conf["a_int"].as<long>();
	/// Same for double
	double a_float = conf["a_float"].as<double>();
	/// Same for string
	std::string a_string = conf["a_string"].as<std::string>();
	/// Same for bool
	int a_yes = conf["a_yes"].as<bool>();
	std::cout << "a_int=" << a_int << 
	             " a_float=" << a_float <<
				 " a_string=\"" << a_string <<
				 "\" a_yes=" << (a_yes?"true":"false") << std::endl;
	

	/// Element of a list are accessed using the iterator
	std::cout << "a_list=[ ";
	for (auto it = conf["a_list"].begin(); it != conf["a_list"].end(); ++it) {
		std::cout << it->as<std::string>() << " ";
	}
	std::cout << "]" << std::endl;
	
	/// Or ranged-based for loop
	std::cout << "a_list=[ ";
	for (auto node : conf["a_list"]) {
		std::cout << node.as<std::string>() << " ";
	}
	std::cout << "]" << std::endl;

	// /// Sub-tree/node of a tree are accessed using the iterator and key/value function
	std::cout << "a_map={" << std::endl;
	for (auto it = conf["a_map"].begin(); it != conf["a_map"].end(); ++it) {
		std::cout << "  " << it.key() << " => " << it.value().as<std::string>() << std::endl;
	}
	std::cout << "}" << std::endl;

	try {
		conf["some_key"];
		std::cout << "config contains 'some_key'" << std::endl;
	} catch (const PC::Error& e) {
		std::cout << "config does not contain 'some_key'" << std::endl;
	}
	
	return 0;
}
